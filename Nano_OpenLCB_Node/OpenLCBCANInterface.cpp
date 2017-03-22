#include "OpenLCBCANInterface.h"
#include <mcp_can.h>
#include "OpenLCBMessage.h"

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
	uint32_t* msgId; 
	byte*  msgData;
	uint8_t* msgLen;
	uint8_t rc;
	
	msgId = msg->getPId();
	msgData = msg->getPData();
	msgLen = msg->getPDataLength();
	
	rc = can->readMsgBuf(msgId, msgData, msgLen); 
	// ignore messages with standard (11 bit) ids or RTR messages
	// If the ID AND 0x80000000 EQUALS 0x80000000, the ID is of the Extended type, otherwise it is standard.  
    // If the ID AND 0x40000000 EQUALS 0x40000000, the message is a remote request.  
    if (rc == CAN_OK) {
	    if ((*msgId & 0xC0000000) == 0x0){
	        return true;
	    } else {
	        return false;
		}
	} else {
		return false;
	}
}

bool OpenLCBCANInterface::sendMessage(OpenLCBMessage* msg){
	uint32_t msgId; 
	uint8_t len;
	byte* data;
	
	msgId = msg->getId();
	len = msg->getDataLength();
	data = msg->getPData();
	// msg->getData(&data, dataLength);
    // sendMsgBuf(INT32U id, INT8U len, INT8U *buf);  
    // To mark an ID as extended, OR the ID with 0x80000000.
    msgId |= 0x80000000;
	byte sndStat = can->sendMsgBuf(msgId, 0, 8, data);
	return (sndStat == CAN_OK);
	}
