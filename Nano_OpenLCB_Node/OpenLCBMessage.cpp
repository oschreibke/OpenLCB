#include "OpenLCBMessage.h"

OpenLCBMessage::OpenLCBMessage(void){
	initialise();
}

// property get/set
uint32_t OpenLCBMessage::getId(){
	return id;
}
void OpenLCBMessage::setId(uint32_t newId){
	id = newId;
}

uint8_t OpenLCBMessage::getDataLength(){
	return dataLength;
	}
	
void OpenLCBMessage::setDataLength(uint8_t newDataLength){
	dataLength = newDataLength;
}

void OpenLCBMessage::getData(byte* dataBuf, uint8_t length){
	length = dataLength;
	for(uint8_t i = 0; i < dataLength; i++)
	    dataBuf[i] = data[i];
}

void OpenLCBMessage::setData(byte* newDataBuf, uint8_t newDataLength){
	dataLength = newDataLength;
	for(uint8_t i = 0; i < dataLength; i++)
	    newDataBuf[i] = data[i];
}

uint32_t* OpenLCBMessage::getPId(){
	return &id;
}
byte* OpenLCBMessage::getPData(){
	return &data[0];
}

uint8_t * OpenLCBMessage::getPDataLength(){
	return &dataLength;
}

void OpenLCBMessage::initialise(){
	// initialise to empty 
	id = 0;
	dataLength = 0;
	for (uint8_t i = 0; i < 8; i++) data[i] = 0x0; 
//    newMessage = false;	
	}

//bool OpenLCBMessage::getNewMessage(){
//	return newMessage;
//	}

bool OpenLCBMessage::isControlMessage(){
	return (id & 0x08000000ULL != 0);
	}

