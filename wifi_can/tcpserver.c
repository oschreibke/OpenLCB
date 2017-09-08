// from the espressif programming guide (3.3.3)

#include "string.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "lwip/sockets.h"
#include "tcpserver.h"

//#define os_zalloc(s)                        pvPortZalloc((s))
#define os_zalloc(s)  calloc(1, (s))

#define SERVER_PORT 23
#define MAX_CONN 2

int32_t listenfd;

void tcpServer(void *pvParameters) {
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

    xTaskCreate(&tcpListener, "TCP listener", configMINIMAL_STACK_SIZE, pvParameters, 3, NULL);
    xTaskCreate(&tcpProcessor, "TCP Processor", configMINIMAL_STACK_SIZE, pvParameters, 2, NULL);

    while(1){
		vTaskDelay(100000 / portTICK_PERIOD_MS);
		}
		
    // prevent bad things happening if we drop off the bottom
    printf("Deleting task %s.\n", __func__);
    vTaskDelete(NULL);
}

void tcpListener(void * pvParameters) {
    printf("ESP8266 TCP server task > listen ok\n");

    struct sockaddr_in remote_addr;

    // Wait until the TCP client is connected to the server; then start receiving data packets
    // when the TCP communication is established:
    int32_t client_sock;
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

            while ((colonPos = strchr(startPos, ':')) != NULL) {                          // found a colon
                if(strchr(colonPos + 1, 'X') !=  NULL) {                                  // followed immediately by an X
                    if ((nPos = strchr(colonPos + 2, 'N')) != NULL) {                     // followed by an N
                        if ((semicolonPos = strchr(colonPos + 2, ';')) != NULL) {         // and a ;
                            if (semicolonPos > nPos) {                                    // semicolon occurs after the N
                                if (nPos - colonPos -2 > 0 && nPos - colonPos -2 <= 8) {  // with 1-8 characters between X and N
                                    if (semicolonPos - nPos <= 16) {                      // max 16 data nybbles (not checking for pairs here)
                                        // looks good, queue the message
                                        char* canAsciiMessage = (char *)os_zalloc(CAN_ASCII_MESSAGE_LENGTH);
                                        char* pCanMsg = canAsciiMessage;
                                        for (char* p = colonPos; p <= semicolonPos; p++) {
                                            *pCanMsg = *p;
                                        }
                                        printf("queueing message %s", canAsciiMessage);
                                        if(xQueueSendToBack(((struct queueHandles *) pvParameters)->xQueue1, canAsciiMessage, 500 / portTICK_PERIOD_MS)!= pdPASS ) {
                                            printf("Queue xQueue1 full, discarding message\n");
                                        }
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

void tcpProcessor(void * pvParameters) {
	char* pCanAsciiMessage; 
	int32_t canId;
	int32_t dataLength;
	char canData[8];
	
	while(1){
		if (xQueueReceive(((struct queueHandles *) pvParameters), &pCanAsciiMessage, portMAX_DELAY)){
			;
			}
		}
		
    // prevent bad things happening if we drop off the bottom
    printf("Deleting task %s.\n", __func__);
    vTaskDelete(NULL);
}
