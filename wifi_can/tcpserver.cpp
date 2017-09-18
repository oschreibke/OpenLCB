

// from the espressif programming guide (3.3.3)

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

int32_t listenfd;
int32_t client_sock;

MCP_CAN mcpcan;
extern struct queueHandles qh;

tcpProcessor tcpprocessor;
tcpListener tcplistener;
canProcessor canprocessor;

//void tcpServer(void *pvParameters) {
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

    printf("ESP8266 TCP server task > port:%d\n",ntohs(server_addr.sin_port));

    // Establish TCP server interception:

    do {
        /* Listen to the local connection */
        ret = listen(listenfd, MAX_CONN);
        if (ret != 0) {
            printf("ESP8266 TCP server task > failed to set listen queue!\n");
            vTaskDelay(1000/portTICK_PERIOD_MS);
        }
    } while(ret != 0);

    //xTaskCreate(&tcpListener, "TCP listener", configMINIMAL_STACK_SIZE, NULL, 3, NULL);
    //xTaskCreate(&tcpProcessor, "TCP Processor", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    tcplistener.task_create("TCP listener", configMINIMAL_STACK_SIZE, 3);
    tcpprocessor.task_create("TCP Processor", configMINIMAL_STACK_SIZE, 2);

    printf("Setting up the can interface.\n");
    if(mcpcan.begin(15, 5, MCP_ANY, CAN_125KBPS, MCP_8MHZ) == CAN_OK) {
        printf("Can Init OK, starting can procesor task\n");
        canprocessor.task_create("CAN Processor", configMINIMAL_STACK_SIZE, 2);
    } else {
        printf("Can init failed\n");
    }
    while(1) {
        vTaskDelay(100000 / portTICK_PERIOD_MS);
    }

    // prevent bad things happening if we drop off the bottom
    printf("Deleting task %s.\n", __func__);
    vTaskDelete(NULL);
}

//void tcpListener(void * pvParameters) {
void tcpListener::task() {
    printf("ESP8266 TCP server task > listen ok\n");

    struct sockaddr_in remote_addr;

    // Wait until the TCP client is connected to the server; then start receiving data packets
    // when the TCP communication is established:

    int32_t len = sizeof(struct sockaddr_in);
    int32_t recbytes;

    char *recv_buf = (char *)os_zalloc(TCP_MESSAGE_LENGTH);

    for (;;) {
        printf("ESP8266 TCP server task > wait client\n");
        /*block here waiting remote connect request*/
        if ((client_sock = accept(listenfd, (struct sockaddr *)&remote_addr, (socklen_t *)&len)) < 0) {
            printf("ESP8266 TCP server task > accept fail\n");
            continue;
        }

        printf("ESP8266 TCP server task > Client from %s %d\n", inet_ntoa(remote_addr.sin_addr), htons(remote_addr.sin_port));

        while ((recbytes = read(client_sock, recv_buf, TCP_MESSAGE_LENGTH)) > 0) {
            recv_buf[recbytes] = 0;
            printf("ESP8266 TCP server task > read data success %d!\nESP8266 TCP server task > %s\n", recbytes, recv_buf);

            // preliminary validation:
            // only pass on messages with format :X\[0-9A-F]{1:8}N{[0-9A-F]{0:16}};
            // no regex so do this by hand

            char* startPos = recv_buf;
            char* colonPos = NULL;
            char* nPos = NULL;
            char* semicolonPos = NULL;

            while ((uint8_t)(startPos - recv_buf) < strlen(recv_buf) && (colonPos = strchr(startPos, ':')) != NULL) {                                            // found a colon
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
                                        char* canAsciiMessage = (char *)os_zalloc(CAN_ASCII_MESSAGE_LENGTH);
                                        char* pCanMsg = canAsciiMessage;
                                        for (char* p = colonPos; p <= semicolonPos; p++) {
                                            *pCanMsg++ = *p;
                                        }
                                        printf("queueing message %s\n", canAsciiMessage);
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

    free(recv_buf);
    // prevent bad things happening if we drop off the bottom
    printf("Deleting task %s.\n", __func__);
    vTaskDelete(NULL);
}

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

//void tcpProcessor(void * pvParameters) {
void tcpProcessor::task() {
    char* pCanAsciiMessage;
    int hx;
    bool fail = false;
    int32_t canId;
    int32_t dataLen;
    char canData[8];

    while(1) {
        if (xQueueReceive(qh.xQueueWiFiToCan, &pCanAsciiMessage, portMAX_DELAY) == pdPASS) {
            printf("Received message\n");
            printf("Content %s\n", pCanAsciiMessage);
            canId = 0;
            dataLen = 0;
            fail = false;

            // build the can id
            for(char* p = pCanAsciiMessage + 2; p < strchr(pCanAsciiMessage + 2, 'N'); p++) {
                hx = hex2int(p);

                if (hx < 16) {
                    canId = (canId * 16) + hx;
                } else { // Not a hex digit - discard the message
                    printf("Non hex character encountered - discarding message (%s)\n", pCanAsciiMessage);
                    fail = true;
                    break;
                }
            }

            if (!fail) {
                // add the data
                dataLen = 0;
                for (char* p = strchr(pCanAsciiMessage + 2, 'N') +1; p < strchr(pCanAsciiMessage + 2, ';'); p += 2) {
                    canData[dataLen] = '\0';
                    for (int i = 0; i < 2; i++) {
                        hx = hex2int(p);
                        if (hx < 16) {
                            canData[dataLen] = (canData[dataLen] << 4) + hx;
                        } else { // Not a hex digit - discard the message
                            printf("Non hex character encountered - discarding message (%s)\n", pCanAsciiMessage);
                            fail = true;
                            break;
                        }
                    }
                    dataLen++;
                }
            }

            if (!fail) {
                // put the message to CAN
                printf("Writing message to the CAN interface. ID = %d, data length = %d.\n", canId, dataLen);
            }

            free(pCanAsciiMessage);
        }
    }

    // prevent bad things happening if we drop off the bottom
    printf("Deleting task %s.\n", __func__);
    vTaskDelete(NULL);
}

void canProcessor::byte2hex(uint8_t byte, char* ptr) {
    const char hexDigits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    *ptr = hexDigits[byte >> 4];
    *(ptr + 1) = hexDigits[byte & 0x0F];
    return;
}

void canProcessor::task() {
    CAN_MESSAGE* pCanMessage;
    char canAsciiMessage[CAN_ASCII_MESSAGE_LENGTH];
    char* p;
    while (1) {
        if (xQueueReceive(qh.xQueueCanToWiFi, &pCanMessage, portMAX_DELAY) == pdPASS) {
            strcpy(canAsciiMessage, ":X");
            p = &canAsciiMessage[2];
            for (int i = 3; i >= 0; i--) {
                byte2hex(pCanMessage->id / (8 * i), p);
                p += 2;
            }
            *p++ = 'N';
            for (int i = 0; i < pCanMessage->len; i++) {
                byte2hex(pCanMessage->dataBytes[i], p);
                p += 2;
            }
            *p++ = ';';
            *p = '\0';
        }
        free(pCanMessage);
        if	(write(client_sock, &canAsciiMessage, strlen(canAsciiMessage) + 1) < 0){
			printf("Write to TCP failed\n");
}
    }
    // prevent bad things happening if we drop off the bottom
    printf("Deleting task %s.\n", __func__);
    vTaskDelete(NULL);
}
