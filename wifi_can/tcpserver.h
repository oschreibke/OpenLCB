#ifndef TCPSERVER_H
#define TCPSERVER_H


#define TCP_MESSAGE_LENGTH 128
#define CAN_ASCII_MESSAGE_LENGTH 30

struct queueHandles{
    QueueHandle_t xQueueWiFiToCan, xQueueCanToWiFi;
};

// Class definitions
// Note: there is no canListener, that functionality is provided by the interrupt handler

class tcpServer: public esp_open_rtos::thread::task_t
{
public:
    
private:
    void task();
};

class tcpListener: public esp_open_rtos::thread::task_t
{
public:
    
private:
    void task();
};

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
