#ifndef TCPSERVER_H
#define TCPSERVER_H


#define TCP_MESSAGE_LENGTH 128
#define CAN_ASCII_MESSAGE_LENGTH 29

struct queueHandles{
    QueueHandle_t xQueueWiFiToCan, xQueueCanToWiFi;
};


void tcpServer(void *pvParameters);
void tcpListener(void * pvParameters);
void tcpProcessor(void * pvParameters); 
int32_t hex2int(char* hexChar);

#endif
