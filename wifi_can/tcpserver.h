#ifndef TCPSERVER_H
#define TCPSERVER_H


#define TCP_MESSAGE_LENGTH 128
#define CAN_ASCII_MESSAGE_LENGTH 29

struct queueHandles{
    QueueHandle_t xQueueWiFiToCan, xQueueCanToWiFi;
};


//void tcpServer(void *pvParameters);
class tcpServer: public esp_open_rtos::thread::task_t
{
public:
    
private:
    void task();
};

//void tcpListener(void * pvParameters);
class tcpListener: public esp_open_rtos::thread::task_t
{
public:
    
private:
    void task();
};

//void tcpProcessor(void * pvParameters); 
class tcpProcessor: public esp_open_rtos::thread::task_t
{
public:
    
private:
    void task();
    uint8_t hex2int(char* hexChar);
};

class canProcessor: public esp_open_rtos::thread::task_t
{
public:
    
private:
    void task();
    void byte2hex(uint8_t byte, char* ptr);
};
#endif
