/*
 * tcpserver.cpp
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

// see https://gridconnect.com/media/documentation/grid_connect/CAN_USB232_UM.pdf for details of the can-ascii message format

#include "string.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.hpp"
#include "lwip/sockets.h"
#include "tcpserver.h"
#include "mcp_can.hpp"
#include "canmessage.h"

//#define os_zalloc(s)                        pvPortZalloc((s))
#define os_zalloc(s)  calloc(1, (s))

#define SERVER_PORT 23
#define MAX_CONN 2

// Global variables

int32_t listenfd;
int32_t client_sock;

MCP_CAN mcpcan;
extern struct queueHandles qh;

tcpProcessor tcpprocessor;
tcpListener tcplistener;
canProcessor canprocessor;

/*
 * Establish a TCP server, and bind it with the local port.
 * Establish TCP server interception
 * Start the TCP listener and processor tasks
 * Set up the can interface 
 *   - if successful, start the can processor task.
 * 
*/ 
void tcpServer::task() {
    // 1. Establish a TCP server, and bind it with the local port.

    printf("starting Telnet Server\n");


    int32_t ret;

    struct sockaddr_in server_addr; //,remote_addr;

    /* Construct local address structure */

    memset(&server_addr, 0, sizeof(server_addr)); /* Zero out structure */

    server_addr.sin_family = AF_INET;            /* Internet address family */
    server_addr.sin_addr.s_addr = INADDR_ANY;   /* Any incoming interface */
    server_addr.sin_len = sizeof(server_addr);
    server_addr.sin_port = htons(SERVER_PORT); /* Local port */

    /* Create socket for incoming connections */

    do {
        listenfd = socket(AF_INET, SOCK_STREAM, 0);
        if (listenfd == -1) {
            printf("ESP8266 TCP server task > socket error\n");
            vTaskDelay(1000/portTICK_PERIOD_MS);
        }
    } while(listenfd == -1);

    printf("ESP8266 TCP server task > create socket: %d\n", listenfd);

    /* Bind to the local port */

    do {
        ret = bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (ret != 0) {
            printf("ESP8266 TCP server task > bind fail\n");
            vTaskDelay(1000/portTICK_PERIOD_MS);
        }
    } while(ret != 0);

    printf("ESP8266 TCP server task > port:%d\n", ntohs(server_addr.sin_port));

    // Establish TCP server interception:

    do {
        /* Listen to the local connection */
        ret = listen(listenfd, MAX_CONN);
        if (ret != 0) {
            printf("ESP8266 TCP server task > failed to set listen queue!\n");
            vTaskDelay(1000/portTICK_PERIOD_MS);
        }
    } while(ret != 0);

    printf("TCP listener OK, starting listener task\n");
    tcplistener.task_create("TCP listener", configMINIMAL_STACK_SIZE * 2, 3);

    //printf("Setting up the can interface.\n");
    if(mcpcan.begin(CAN_CS, CAN_INT, MCP_ANY, CAN_125KBPS, MCP_8MHZ) == CAN_OK) {
        printf("Can Init OK, starting Procesor tasks\n");
        mcpcan.setMode(MCP_NORMAL);
        canprocessor.task_create("CAN Processor", configMINIMAL_STACK_SIZE * 2, 2);
        tcpprocessor.task_create("TCP Processor", configMINIMAL_STACK_SIZE * 2, 2);
    } else {
        printf("Can init failed\n");
    }
    
    /*
    // test: queue a can message
    CAN_MESSAGE canMsg;
    
    for (uint8_t i = 0; i<10; i++){
		canMsg.id = 0x2000;
		canMsg.ext = CAN_EXTID;
		canMsg.len = 1;
		canMsg.dataBytes[0] = i;
		if(xQueueSendToBackFromISR(qh.xQueueCanToWiFi, &canMsg, NULL)!= pdPASS ) {
			printf("Queue xQueueWiFiToCan full, discarding message\n");
		}
	}
    */
    while(1) {
        vTaskDelay(100000 / portTICK_PERIOD_MS);
    }

    // prevent bad things happening if we drop off the bottom (which we shouldn't)
    printf("Deleting task %s.\n", __func__);
    vTaskDelete(NULL);
}

/*
 * Wait for a client to connect
 * Once a message arrives, check it is a valid CANASCII message. Format :X[0-9A-F]{1:8}N{[0-9A-F]{0:16}};
 * If OK, queue for processing 
 *  
*/ 
void tcpListener::task() {
    printf("ESP8266 TCP server task > listen ok\n");

    struct sockaddr_in remote_addr;

    // Wait until the TCP client is connected to the server; then start receiving data packets
    // when the TCP communication is established:

    int32_t len = sizeof(struct sockaddr_in);
    int32_t recbytes;

    char recv_buf[TCP_MESSAGE_LENGTH];
    char canAsciiMessage[CAN_ASCII_MESSAGE_LENGTH];  

    for (;;) {
        //printf("ESP8266 TCP server task > wait client\n");
        /*block here waiting remote connect request*/
        if ((client_sock = accept(listenfd, (struct sockaddr *)&remote_addr, (socklen_t *)&len)) < 0) {
            printf("ESP8266 TCP server task > accept fail\n");
            continue;
        }

        printf("ESP8266 TCP server task > Client from %s %d\n", inet_ntoa(remote_addr.sin_addr), htons(remote_addr.sin_port));

        while ((recbytes = read(client_sock, &recv_buf, TCP_MESSAGE_LENGTH)) > 0) {
            recv_buf[recbytes] = 0;
            //printf("ESP8266 TCP server task > read data success %d!\nESP8266 TCP server task > %s\n", recbytes, recv_buf);

            // preliminary validation:
            // only pass on messages with format :X[0-9A-F]{1:8}N{[0-9A-F]{0:16}};
            // no regex so do this by hand

            char* startPos = &recv_buf[0];
            char* colonPos = NULL;
            char* nPos = NULL;
            char* semicolonPos = NULL;

            while ((uint8_t)(startPos - &recv_buf[0]) < strlen(recv_buf) && (colonPos = strchr(startPos, ':')) != NULL) {  // found a colon
                //printf("got :\n");
                if(strchr(colonPos + 1, 'X') !=  NULL) {                                                    // followed immediately by an X
                    //printf("Got X\n");
                    if ((nPos = strchr(colonPos + 2, 'N')) != NULL) {                                       // followed by an N
                        //printf("Found N\n");
                        if ((semicolonPos = strchr(colonPos + 2, ';')) != NULL) {                           // and a ;
                            //printf("Found ;\n");
                            if (semicolonPos > nPos) {                                                      // semicolon occurs after the N
                                if (nPos - colonPos -2 > 0 && nPos - colonPos -2 <= 8) {                    // with 1-8 characters between X and N
                                    if (semicolonPos - nPos - 1 <= 16 && ((semicolonPos - nPos - 1) % 2) == 0) {  // max 16 data nybbles, in pairs
                                        // looks good, queue the message
                                        memset(&canAsciiMessage, '\0', CAN_ASCII_MESSAGE_LENGTH);
                                        char* pCanMsg = &canAsciiMessage[0];
                                        for (char* p = colonPos; p <= semicolonPos; p++) {
                                            *pCanMsg++ = *p;
                                        }
                                        //printf("queueing message %s\n", canAsciiMessage);
                                        if(xQueueSendToBack(qh.xQueueWiFiToCan, &canAsciiMessage, 500 / portTICK_PERIOD_MS)!= pdPASS ) {
                                            printf("Queue xQueueWiFiToCan full, discarding message\n");
                                        }
                                    } else {
                                        printf("Data error: more than 16 data nybbles, or not in pairs. %s\n", recv_buf);
                                    }
                                }
                            }
                        } else {
                            break;            // haven't found a semicolon, the message is not complete
                        }
                    }
                }
                startPos = semicolonPos + 1;  // if we found a ;, reposition the scanner to after it
            }
        }

        if (recbytes <= 0) {
            printf("ESP8266 TCP server task > read data fail!\n");
            close(client_sock);
        }
    }

    // prevent bad things happening if we drop off the bottom (which we shouldn't)
    printf("Deleting task %s.\n", __func__);
    vTaskDelete(NULL);
}

/*
 * Convert from a hex character to binary 
*/
uint8_t tcpProcessor::hex2int(char* hexChar) {
    const char hexDigits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    int i;

    for(i = 0; i < 16; i++) {
        if(*hexChar == hexDigits[i]) {
            break;
        }
    }
    return i;
};

/*
 * decode a queued CANASCII message into a CAN message. The format is valid, otherwise it wouldn't have been queued
 * pass the CAN message to the MCP2515 
*/ 

void tcpProcessor::task() {
    char canAsciiMessage[CAN_ASCII_MESSAGE_LENGTH];
    int hx;
    bool fail = false;
    int32_t canId;
    int32_t dataLen;
    uint8_t canData[8];

    while(1) {
        if (xQueueReceive(qh.xQueueWiFiToCan, &canAsciiMessage, portMAX_DELAY) == pdPASS) {
            //printf("Dequeued CANASCII message %s\n", canAsciiMessage);
            canId = 0;
            dataLen = 0;
            fail = false;

            // build the can id
            for(char* p = &canAsciiMessage[2]; p < strchr(&canAsciiMessage[2], 'N'); p++) {
                hx = hex2int(p);

                if (hx < 16) {
                    canId = (canId * 16) + hx;
                } else { // Not a hex digit - discard the message
                    printf("Non hex character encountered - discarding message (%s)\n", canAsciiMessage);
                    fail = true;
                    break;
                }
            }

            if (!fail) {
                // add the data
                dataLen = 0;
                memset(&canData, '\0', 8);
                for (char* p = strchr(&canAsciiMessage[2], 'N') +1; p < strchr(&canAsciiMessage[2], ';');) {
                    for (int i = 0; i < 2; i++) {
                        hx = hex2int(p++);
                        //printf("Encoding %c = %d\n", *p-1, hx);
                        if (hx < 16) {
                            canData[dataLen] = (canData[dataLen] << 4) + hx;
                        } else { // Not a hex digit - discard the message
                            printf("Non hex character encountered - discarding message (%s)\n", canAsciiMessage);
                            fail = true;
                            break;
                        }
                    }
                    dataLen++;
                }
            }

            if (!fail) {
                // put the message to CAN
                //printf("Writing message to the CAN interface. ID = %d, data length = %d, dataBytes ", canId, dataLen);
                for(int i = 0; i < dataLen; i++){
					printf("%0X ", canData[i]);
					}
                printf("\n");					
                INT8U res = mcpcan.sendMsgBuf(canId, CAN_EXTID, dataLen, &canData[0]);
                if (res == CAN_OK) {
                    ; //printf("sendMsgBuf OK\r\n");
                } else {
                    printf("%s: sendMsgBuf failed rc=%d\n", __func__, res);
                }
            }
        }
    }

    // prevent bad things happening if we drop off the bottom (which we shouldn't)
    printf("Deleting task %s.\n", __func__);
    vTaskDelete(NULL);
}

/*
 * encode a byte to two Hex Characters 
 * Parameters:
 *    byte: the octet to encode
 *    ptr:  the buffer location of the first result character
*/ 
void canProcessor::byte2hex(uint8_t byte, char* ptr) {
    const char hexDigits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    //printf("byte2hex %0X \n", byte);
    *ptr = hexDigits[byte >> 4];
    *(ptr + 1) = hexDigits[byte & 0x0F];
    return;
}

/*
 * Encode a CAN message to the corresponding CANASCII format, and pass to the telnet client
*/ 

void canProcessor::task() {
    CAN_MESSAGE canMessage;
    char* p;
    char canAsciiMessage[CAN_ASCII_MESSAGE_LENGTH];

    while (1) {
		//printf("Check queue CanToWiFi\n");
		//printf("%u messages waiting\n", uxQueueMessagesWaiting(qh.xQueueCanToWiFi));
        if (xQueueReceive(qh.xQueueCanToWiFi, &canMessage, portMAX_DELAY) == pdPASS) {
            printf("Dequeued CAN message.\n");
            printf("id = %d, length = %d\n", canMessage.id, canMessage.len);
            memset(&canAsciiMessage, '\0', CAN_ASCII_MESSAGE_LENGTH);
            strcpy(canAsciiMessage, ":X");
            p = &canAsciiMessage[2];
            //printf("Encoding id\n");
            for (int i = 3; i >= 0; i--) {
                byte2hex(canMessage.id >> (8 * i), p);
                p += 2;
            }
            *p++ = 'N';
            //printf("Encoding data bytes\n");
            for (int i = 0; i < canMessage.len; i++) {
                byte2hex(canMessage.dataBytes[i], p);
                p += 2;
            }
            *p++ = ';';
            *p = '\n';

            //printf("Writing CanASCII message %s to network\n", canAsciiMessage);
            if	(write(client_sock, canAsciiMessage, strlen(canAsciiMessage) + 1) < 0) {
                printf("Write to TCP failed\n");
            } else {
                printf("Write to TCP OK\n");
            }
        }
    }
    
    // prevent bad things happening if we drop off the bottom  (which we shouldn't)
    printf("Deleting task %s.\n", __func__);
    vTaskDelete(NULL);
}
