/*
 * wifi_can.cpp
 * 
 * Copyright 2017 Otto Schreibke <oschreibke@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * The license can be found at https://www.gnu.org/licenses/gpl-3.0.txt
 * 
 * 
 */
 
/* A Telnet to CAN gateway.
 * also includes a very simple OTA (over the air) function.
 *
 * The gateway reads CANASCII format messages from the Telnet interface, and sends them to a MCP2515 can interface module, and vice versa.
 * The gateway was concieved to communicate between JMRI and an OpenLCB installation, as such only CAN-Extended messages are passed. 
 * Credit where its due: the MCP2515 driver was originally developed by Cory J Fowler for Arduino.
 * 
 * The OTA facility runs a TFTP server, it will accept a TTP firmware and update it.
 * For more information about esp-open-rtos OTA see https://github.com/SuperHouse/esp-open-rtos/wiki/OTA-Update-Configuration
 *
 * N.B. TFTP HAS NO AUTHENTICATION. THEREFORE IS NOT SUITABLE TO PUT ON THE INTERNET OR INTO A PRODUCTION ENVIRONMENT!!!!
 * 
 * To perform an OTA flash:
 * 
 *    CD <outerfolders>/wifi_can
 *    make -j4 rebuild
 *    tftp <ESP8266's ip address>
 *    bin
 *    put firmware/wifi_can.bin firmware.bin
 *    quit
 * 
 * Connections:
 *    ESP8266 (ESP-12E)     MCP2515
 *    -----------------     -------
 *    GPIO04                INT
 *    GPIO05                CS        -- not using GPIO15 as CS; it messes up the booting
 *    GPIO12  (MISO)        SO
 *    GPIO13  (MOSI)        SI
 *    GPIO14  (CLK)         SCK
 * 
 */

#include <string.h>
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"


#include "task.hpp"
#include "esp8266.h"
#include "ssid_config.h"
#include <lwip/api.h>
#include "lwip/inet.h"

#include "ota-tftp.h"
#include "rboot-api.h"

#include "wifi_can.h"
#include "tcpserver.h"
#include "mcp_can.hpp"
#include "canmessage.h"

#define TELNET_PORT 23

// global variables
struct queueHandles qh;
heartbeat_task ht;
start_listeners_task slt;
tcpServer tcpserver;

/*
 * Heartbeat task. writes to the serial console every 10 seconds 
*/ 
void heartbeat_task::task() {
    static uint i = 0;

    while(1) {

        printf("Heartbeat %d\n", ++i);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}


/*
 * Check the status of the wifi connection. The ESP8266 will take some time to connect.
 * Without it we can't do much. 
*/ 
bool start_listeners_task::check_wifi_connection() {
    uint8_t status = sdk_wifi_station_get_connect_status();

    switch (status) {
    case STATION_IDLE:
        printf("WiFi: station idle\n\r");
        break;
    case STATION_CONNECTING:
        printf("WiFi: station connecting\n\r");
        break;
    case STATION_WRONG_PASSWORD:
        printf("WiFi: wrong password\n\r");
        break;
    case STATION_NO_AP_FOUND:
        printf("WiFi: AP not found\n\r");
        break;
    case STATION_CONNECT_FAIL:
        printf("WiFi: connection failed\r\n");
        break;
    case STATION_GOT_IP:
        printf("WiFi: got ip\r\n");
        break;
    default:
        printf("%s: status = %d\n\r", __func__, status);
        break;
    }

    return (status == STATION_GOT_IP);
}


/*
 * Wait for an IP connection
 * Once we have it, start the OTA TFTP server and the Telnet task, then terminate gracefully
 * The spawned tasks will live on. 
*/ 

void start_listeners_task::task() {

    // wait until we have an ip address
    while(!check_wifi_connection()) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    printf("Getting IP address\n");

    struct ip_info ipconfig;
    sdk_wifi_get_ip_info(STATION_IF, &ipconfig);
    // check the IP address or net connection state
    while (ipconfig.ip.addr == 0) {
        printf("should not be here: no ip address returned despite status STATION_GOT_IP in check_wifi_connection()\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        sdk_wifi_get_ip_info(STATION_IF, &ipconfig);
    }

    printf("Starting TFTP server. Listening on %s, port %d.\n\n", inet_ntoa(ipconfig.ip.addr), TFTP_PORT);
    ota_tftp_init_server(TFTP_PORT);
    printf("TFTP server started\n");

    // start the telnet task if the queues have been successfully created
    if (qh.xQueueWiFiToCan != NULL && qh.xQueueCanToWiFi != NULL) {
        printf("starting telnet task\n");
        tcpserver.task_create("telnetTask", 512, 3);
    }

    printf("Terminating task 'start_ota_ftp_task' (me)\n");
    vTaskDelete(NULL);
}

/*
 * Application initialisation
 * 
 * Start the heartbeat task
 * connect to the local WiFi network
 * create the queues:
 *      xQueueWiFiToCan - holds CANASCII messages from the Telnet client
 *      xQueueCanToWiFi - holds CAN messages from the MCP2515
 * create the start listeners task 
*/ 

extern "C" void user_init(void) {
    uart_set_baud(0, 115200);

    printf("\r\n\r\nWiFi -> can bridge.\r\n");
    printf("SDK version:%s\n", sdk_system_get_sdk_version());

    rboot_config conf = rboot_get_config();
    printf("\r\nCurrently running on flash slot %d / %d.\r\n\r\n",
           conf.current_rom, conf.count);

    printf("Image addresses in flash:\r\n");
    for(int i = 0; i <conf.count; i++) {
        printf("%c%d: offset 0x%08x\r\n", i == conf.current_rom ? '*':' ', i, conf.roms[i]);
    }

    printf("starting heartbeat task\n");
    ht.task_create("heartbeat", configMINIMAL_STACK_SIZE, 2);

    struct sdk_station_config config;
    strcpy((char*)config.ssid, WIFI_SSID);
    strcpy((char*)config.password, WIFI_PASS);
    config.bssid_set = 0;

    printf("Connecting to %s.\n", config.ssid);

    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&config);

    printf("Creating queues\n");

    qh.xQueueWiFiToCan = xQueueCreate(20, CAN_ASCII_MESSAGE_LENGTH);

    if( qh.xQueueWiFiToCan == NULL ) {
        printf("Could not create xQueueWiFiToCan\n");
    }

    qh.xQueueCanToWiFi = xQueueCreate(20, sizeof(CAN_MESSAGE));

    if( qh.xQueueCanToWiFi == NULL ) {
        printf("Could not create xQueueCanToWiFi\n");
    }

    printf("starting listener (TFTP and Telnet) tasks.\n");
    slt.task_create("listener starter", configMINIMAL_STACK_SIZE, 2);
    
    printf("user_init complete\n");
}
