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
 
 
#include "OpenLCBMessage.h"
//#include "util.h"

OpenLCBMessage::OpenLCBMessage(void) {
    initialise();
}

// property get/set
uint32_t OpenLCBMessage::getId() {
    return id;
}

void OpenLCBMessage::setId(uint32_t newId) {
    id = newId;
}

MTI OpenLCBMessage::getMTI() {
    return (MTI)((id & 0x0FFFF000) >> 12);
}

uint16_t OpenLCBMessage::getSenderAlias() {
    return (uint16_t (id & 0x00000FFFULL));
}

uint8_t OpenLCBMessage::getDataLength() {
    return dataLength;
}

void OpenLCBMessage::setDataLength(uint8_t newDataLength) {
    dataLength = newDataLength;
}

void OpenLCBMessage::getData(byte* dataBuf, uint8_t length) {
    length = dataLength;
    for(uint8_t i = 0; i < dataLength; i++)
        dataBuf[i] = data[i];
}

void OpenLCBMessage::setData(byte* newDataBuf, uint8_t newDataLength) {
    dataLength = newDataLength;
    for(uint8_t i = 0; i < dataLength; i++) {
        data[i] = newDataBuf[i];
        //	    util::print8BitHex(newDataBuf[i]);
    }
    //	if (dataLength > 0) Serial.println();
}

uint32_t* OpenLCBMessage::getPId() {
    return &id;
}

uint8_t * OpenLCBMessage::getPExt() {
    return &ext;
}

byte* OpenLCBMessage::getPData() {
    return &data[0];
}

uint8_t * OpenLCBMessage::getPDataLength() {
    return &dataLength;
}

void OpenLCBMessage::initialise() {
    // initialise to empty
    id = 0;
    dataLength = 0;
    for (uint8_t i = 0; i < 8; i++) data[i] = 0x0;
    //    newMessage = false;
}

//bool OpenLCBMessage::getNewMessage(){
//	return newMessage;
//	}


// When zero, the second-most-significant bit (0x0800,0000) indicates that the frame is for local control on the CAN segment.
bool OpenLCBMessage::isControlMessage() {
    return ((id & 0x08000000ULL) == 0);
}

uint8_t OpenLCBMessage::getDataByte(uint8_t index) {
    if (index < dataLength) {
        return data[index];
    } else {
        return 0;
    }
}

// set the can id from the MTI (also for the control types) and the node alias
void OpenLCBMessage::setCANid(uint16_t MTI, uint16_t alias) {
    id = ((uint32_t)MTI << 12 | ((uint32_t)alias & 0x0FFF) );
}

void OpenLCBMessage::setNodeidToData(uint64_t nodeId) {
    dataLength = 6;

    for (uint8_t i = 2; i < 8; i++) {
        //Serial.print((8 * (7 - i))); Serial.print(" ");
        data[i - 2] = (uint8_t) (nodeId >> (8 * (7 - i) & 0xFFUL));
        //util::print8BitHex((uint8_t) (nodeId >> (8 * (7 - i)) & 0xFFUL)); Serial.println();
    }
}

uint64_t OpenLCBMessage::getNodeIdFromData() {
    return (((uint64_t)data[0] << 40) | ((uint64_t)data[1] << 32) | ((uint64_t)data[2] << 24) | ((uint64_t)data[3] << 16) | ((uint64_t)data[4] << 8) | (uint64_t)data[5]);
}

uint64_t OpenLCBMessage::getEventIdFromData() {
    if (dataLength == 8) {
        uint64_t eid = 0;
        for (uint8_t i = 0; i < 8; i++)
            eid = (eid << 8) + data[i];
        return eid;
    } else
        return 0;
}

uint16_t OpenLCBMessage::getDestAliasFromData() {
    if (dataLength > 1) {
        return ((uint16_t) data[0] & 0x0F) << 8 | (uint16_t) data[1];
    } else {
        return 0;
    }
};
