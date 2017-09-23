#ifndef WIFI_CAN_H
#define WIFI_CAN_H

class heartbeat_task: public esp_open_rtos::thread::task_t
{
public:
    
private:
    void task();
};

void telnet_task(void *pvParameters);

class start_listeners_task: public esp_open_rtos::thread::task_t
{
public:
    
private:
    void task();
    bool check_wifi_connection();
};

extern "C" void user_init(void);
#endif
