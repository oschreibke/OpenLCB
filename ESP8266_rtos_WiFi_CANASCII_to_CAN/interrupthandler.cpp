/*
 * interrupthandler.cpp
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


#include <inttypes.h>
#include "interrupthandler.h"
#include "FreeRTOS.h"
#include "task.hpp"
#include "queue.h"
#include "tcpserver.h"
#include "mcp_can.hpp"
#include "canmessage.h"

#define os_zalloc(s)  calloc(1, (s))

extern MCP_CAN mcpcan;
extern struct queueHandles qh;

// Interrupt handler for gpio
// check the interrupt is from the MCP2515, if so, queue the can message for processing

extern "C" void gpio_intr_handler(uint8_t gpio_num) {
    CAN_MESSAGE canMsg;

    if (gpio_num == CAN_INT) {
        if (mcpcan.readMsgBuf(&canMsg.id, &canMsg.ext, &canMsg.len, (INT8U*)&canMsg.dataBytes) == CAN_OK) {
            if(xQueueSendToBackFromISR(qh.xQueueCanToWiFi, &canMsg, NULL)!= pdPASS ) {
                printf("Queue xQueueWiFiToCan full, discarding message\n");
            }
        }
    }
}

