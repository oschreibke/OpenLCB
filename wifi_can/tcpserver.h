#ifndef TCPSERVER_H
#define TCPSERVER_H


#define TCP_MESSAGE_LENGTH 128
#define CAN_ASCII_MESSAGE_LENGTH 29

struct queueHandles{
    QueueHandle_t xQueue1, xQueue2;
};


void tcpServer(void *pvParameters);
void tcpListener(void * pvParameters);
void tcpProcessor(void * pvParameters); 

#endif
