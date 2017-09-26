/*
 * tcpserver.h
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

#ifndef TCPSERVER_H
#define TCPSERVER_H


#define TCP_MESSAGE_LENGTH 128
#define CAN_ASCII_MESSAGE_LENGTH 30

#define CAN_INT 4
#define CAN_CS  5

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
