/*
 * OpenLCBMessage.cpp
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

#include <stdlib.h>
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "esp8266.h"
 
#include "OpenLCBMessage.h"
//#include "util.h"

//OpenLCBMessage(void) {
    //initialise();
//}

// property get/set
uint32_t getId(struct CAN_MESSAGE* cm) {
    return cm->id;
}

void setId(struct CAN_MESSAGE* cm, uint32_t newId) {
    cm->id = newId;
}

uint16_t getMTI(struct CAN_MESSAGE* cm){
    return (uint16_t)((cm->id & 0x0FFFF000) >> 12);
}

uint16_t getSenderAlias(struct CAN_MESSAGE* cm) {
    return (uint16_t) (cm->id & 0x00000FFFULL);
}

uint8_t getDataLength(struct CAN_MESSAGE* cm) {
    return cm->len;
}

void setDataLength(struct CAN_MESSAGE* cm, uint8_t newDataLength) {
    cm->len = newDataLength;
}

void getData(struct CAN_MESSAGE* cm, byte* dataBuf, uint8_t* length) {
    *length = cm->len;
    for(uint8_t i = 0; i < cm->len; i++)
        dataBuf[i] = cm->dataBytes[i];
}

void setData(struct CAN_MESSAGE* cm, byte* newDataBuf, uint8_t newDataLength) {
    cm->len = newDataLength;
    for(uint8_t i = 0; i < cm->len; i++) {
        cm->dataBytes[i] = newDataBuf[i];
        //	    util::print8BitHex(newDataBuf[i]);
    }
    //	if (dataLength > 0) Serial.println();
}

uint32_t* getPId(struct CAN_MESSAGE* cm) {
    return &cm->id;
}

uint8_t * getPExt(struct CAN_MESSAGE* cm) {
    return &cm->ext;
}

byte* getPData(struct CAN_MESSAGE* cm) {
    return &cm->dataBytes[0];
}

uint8_t * getPDataLength(struct CAN_MESSAGE* cm) {
    return &cm->len;
}

void initialise(struct CAN_MESSAGE* cm) {
    // initialise to empty
    cm->id = 0;
    cm->len = 0;
    for (uint8_t i = 0; i < 8; i++) cm->dataBytes[i] = 0x0;
    //    newMessage = false;
}

//bool getNewMessage(){
//	return newMessage;
//	}


// When zero, the second-most-significant bit (0x0800,0000) indicates that the frame is for local control on the CAN segment.
bool isControlMessage(struct CAN_MESSAGE* cm) {
    return ((cm->id & 0x08000000ULL) == 0);
}

uint8_t getDataByte(struct CAN_MESSAGE* cm, uint8_t index) {
    if (index < cm->len) {
        return cm->dataBytes[index];
    } else {
        return 0;
    }
}

// set the can id from the MTI (also for the control types) and the node alias
void setCANid(struct CAN_MESSAGE* cm, uint16_t MTI, uint16_t alias) {
    cm->id = ((uint32_t)MTI << 12 | ((uint32_t)alias & 0x0FFF) );
}

void setNodeidToData(struct CAN_MESSAGE* cm, uint64_t nodeId) {
    cm->len = 6;

    for (uint8_t i = 2; i < 8; i++) {
        //Serial.print((8 * (7 - i))); Serial.print(" ");
        cm->dataBytes[i - 2] = (uint8_t) (nodeId >> (8 * (7 - i) & 0xFFUL));
        //util::print8BitHex((uint8_t) (nodeId >> (8 * (7 - i)) & 0xFFUL)); Serial.println();
    }
}

uint64_t getNodeIdFromData(struct CAN_MESSAGE* cm) {
    return (((uint64_t)cm->dataBytes[0] << 40) | ((uint64_t)cm->dataBytes[1] << 32) | ((uint64_t)cm->dataBytes[2] << 24) | ((uint64_t)cm->dataBytes[3] << 16) | ((uint64_t)cm->dataBytes[4] << 8) | (uint64_t)cm->dataBytes[5]);
}

uint64_t getEventIdFromData(struct CAN_MESSAGE* cm) {
    if (cm->len == 8) {
        uint64_t eid = 0;
        for (uint8_t i = 0; i < 8; i++)
            eid = (eid << 8) + cm->dataBytes[i];
        return eid;
	} else
        return 0;
}

uint16_t getDestAliasFromData(struct CAN_MESSAGE* cm) {
    if (cm->len > 1) {
        return ((uint16_t) cm->dataBytes[0] & 0x0F) << 8 | (uint16_t) cm->dataBytes[1];
    } else {
        return 0;
    }

};
