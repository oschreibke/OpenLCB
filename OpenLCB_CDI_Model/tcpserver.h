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

void tcpServer(void *pvParameters) ;
void tcpListener(void *pvParameters);
void tcpProcessor(void *pvParameters);
void canProcessor(void *pvParameters);

uint8_t hex2int(char* hexChar);
void byte2hex(uint8_t byte, char* ptr);

#endif
