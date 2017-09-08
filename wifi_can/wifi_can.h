#ifndef WIFI_CAN_H
#define WIFI_CAN_H

bool check_wifi_connection();
void heartbeat_task(void *pvParameters);
void telnet_task(void *pvParameters);
void start_listeners_task(void *pvParameters);
void user_init(void);
#endif
