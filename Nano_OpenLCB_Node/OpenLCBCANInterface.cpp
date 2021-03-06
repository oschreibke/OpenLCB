/*
 * OpenLCBCANInterface.cpp
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
 
#include "OpenLCBCANInterface.h"
#include <mcp_can.h>
#include "OpenLCBMessage.h"
#include "util.h"

// Method definitions for OpenLCBCANInterface

OpenLCBCANInterface::OpenLCBCANInterface(){
	canInitialised = false;
}

void OpenLCBCANInterface::begin(MCP_CAN * mcpCanObj, uint8_t intPin){
    can = mcpCanObj;

    // Initialize MCP2515 running at 8MHz with a baudrate of 125kb/s and the masks and filters disabled.

    if(can->begin(MCP_ANY, CAN_125KBPS, MCP_8MHZ) == CAN_OK){
        can->setMode(MCP_NORMAL);
        pinMode(intPin, INPUT);   // Configuring pin for /INT input
        canInitialised = true;
     }
}

bool OpenLCBCANInterface::getInitialised(){
	return canInitialised;
}

bool OpenLCBCANInterface::receiveMessage(OpenLCBMessage* msg){
	//uint32_t* msgId; 
	uint32_t* msgId; 
	uint8_t* msgExt;
	uint8_t*  msgData;
	uint8_t* msgLen;
	uint8_t rc;
	
	msgId = msg->getPId();
	msgExt = msg->getPExt();
	msgData = msg->getPData();
	msgLen = msg->getPDataLength();
	
	rc = can->readMsgBuf(msgId, msgExt, msgLen, msgData); 
	// ignore messages with standard (11 bit) ids or RTR messages
	// If the ID AND 0x80000000 EQUALS 0x80000000, the ID is of the Extended type, otherwise it is standard.  
    // If the ID AND 0x40000000 EQUALS 0x40000000, the message is a remote request.  
    if (rc == CAN_OK) {
		//*msgId &= 0x1FFFFFFF;
		Serial.print(F("Message Received. ID = ")); Serial.println(*msgId, HEX); 
		Serial.print(F("Data bytes: ")); Serial.println(*msgLen); 
		if (*msgLen > 0){
		    for(uint8_t i = 0; i < *msgLen; i++) util::print8BitHex(*(msgData+i)); Serial.println();
		}
	    //if ((*msgId & 0xC0000000) == 0x0){
	        return true;
	    //} else {
	    //    return false;
		//}
	} else {
		return false;
	}
}

bool OpenLCBCANInterface::sendMessage(OpenLCBMessage* msg){
	uint32_t msgId; 
	uint8_t len;
	byte* pData;
	
	msgId = msg->getId();
	len = msg->getDataLength();
	pData = msg->getPData();
	// msg->getData(&data, dataLength);
    // sendMsgBuf(INT32U id, INT8U len, INT8U *buf);  
    // To mark an ID as extended, OR the ID with 0x80000000.
    // also set the "Solo top bit"
    //msgId |= 0x90000000;
    msgId |= 0x10000000; // Set the solo top bit

	Serial.print(F("Sending Message ID = ")); Serial.println(msgId, HEX); 
	Serial.print(F("Data bytes: ")); Serial.println(len); 
	if (len > 0) {
	    for(uint8_t i = 0; i < len; i++) util::print8BitHex(*(pData+i)); Serial.println();
	}
    
	byte sndStat = can->sendMsgBuf(msgId, 1, len, pData);
	return (sndStat == CAN_OK);
	}
