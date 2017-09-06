/* A very simple OTA (over the air) example. Demos OTA flashing, no further logic.
 * 
 * Uses the esp open rtos sdk, not the expressif sdk.
 * 
 *
 * Runs a TFTP server which will accept a firmware image, update it and boot into it.
 *
 * For more information about esp-open-rtos OTA see https://github.com/SuperHouse/esp-open-rtos/wiki/OTA-Update-Configuration
 *
 * NOT SUITABLE TO PUT ON THE INTERNET OR INTO A PRODUCTION ENVIRONMENT!!!! TFTP has no authentication mechanism
 * 
 * To use:
 *    cd whereever/ota
 *    make -j4 all
 *   Initial flash:
 *   # rboot.bin and blank_config.bin from esp-open-rtos/bootloader/bootloader
 *   # ota_basic from firmware/ota_basic.bin
 *    sudo esptool.py -p /dev/ttyAMA0 --baud 115200 write_flash -fs 32m -fm qio -ff 40m 0x0 rboot.bin 0x1000 blank_config.bin 0x2000 ota_basic.bin 
 * 
 *   Subsequent ota flash
 *    make -j4 all 
 *    tftp <ip address of the ESP8266> 69
 *    >binary
 *    >put firmware/ota_basic.bin firmware.bin
 *    >quit
 *   # look for messages on the serial port
 *   # N. B. the first ota boot will fail. simply reset the ESP8266. Subsequent ota boots should succeed. 
 */
#include <string.h>
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "esp8266.h"
#include "ssid_config.h"

#include "ota-tftp.h"
#include "rboot-api.h"

bool check_wifi_connection() {
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


void heartbeat_task(void *pvParameters) {
	static int i = 0;
    while(1) {
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        printf("Heartbeat %d\n", ++i);
        //check_wifi_connection();
    }
}

void start_ota_ftp_task(void *pvParameters) {

    // wait until we have an ip address
    while(!check_wifi_connection()) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    printf("Getting IP address\n");

    struct ip_info ipconfig;
    sdk_wifi_get_ip_info(STATION_IF, &ipconfig);
    // check the IP address or net connection state
    while (ipconfig.ip.addr == 0) {
        printf("should not be here: no ip address returned\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        sdk_wifi_get_ip_info(STATION_IF, &ipconfig);
    }
    //printf("Ip addr: %d.%d.%d.%d\n", ipconfig.ip.addr >>24, (ipconfig.ip.addr >>12) & 0xFF, (ipconfig.ip.addr >>8) & 0xFF, ipconfig.ip.addr & 0xFF);

    printf("Starting TFTP server. Listening on %d.%d.%d.%d:%d\n\n", ipconfig.ip.addr & 0xFF, (ipconfig.ip.addr >>8) & 0xFF, (ipconfig.ip.addr >>16) & 0xFF, ipconfig.ip.addr >>24, TFTP_PORT);
    ota_tftp_init_server(TFTP_PORT);

    printf("Terminating task 'start_ota_ftp_task' (me)\n");
    vTaskDelete(NULL);

}

void user_init(void) {
    uart_set_baud(0, 115200);

    printf("\r\n\r\nOTA Basic demo.\r\n");
    printf("SDK version:%s\n", sdk_system_get_sdk_version());

    rboot_config conf = rboot_get_config();
    printf("\r\nCurrently running on flash slot %d / %d.\r\n\r\n",
           conf.current_rom, conf.count);

    printf("Image addresses in flash:\r\n");
    for(int i = 0; i <conf.count; i++) {
        printf("%c%d: offset 0x%08x\r\n", i == conf.current_rom ? '*':' ', i, conf.roms[i]);
    }


    printf("starting heartbeat task\n");
    xTaskCreate(&heartbeat_task, "heartbeat", configMINIMAL_STACK_SIZE, NULL, 2, NULL);

    struct sdk_station_config config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASS,
        .bssid_set = 0
    };

    printf("Connecting to %s, pwd: %s\n", config.ssid, config.password);
    
    //sdk_wifi_station_set_auto_connect(true);
    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&config);

    printf("starting task to initiate the tftp server\n");
    xTaskCreate(&start_ota_ftp_task, "TFTP starter", configMINIMAL_STACK_SIZE, NULL, 2, NULL); 
    
    printf("user_init complete\n");
}
