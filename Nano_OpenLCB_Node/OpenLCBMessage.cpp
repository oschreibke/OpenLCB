#include "OpenLCBMessage.h"
//#include "util.h"

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

MTI OpenLCBMessage::getMTI(){
	return (MTI)((id & 0x07FFF000) >> 12);
}

uint16_t OpenLCBMessage::getSenderAlias(){
	return (uint16_t (id & 0x00000FFFULL));
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
	for(uint8_t i = 0; i < dataLength; i++){
	    data[i] = newDataBuf[i];
//	    util::print8BitHex(newDataBuf[i]);
	}
//	if (dataLength > 0) Serial.println();
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

uint8_t OpenLCBMessage::getDataByte(uint8_t index){
	if (index < dataLength){
	    return data[index];
	} else {
		return 0;
	}    
}

// set the can id from the MTI (also for the control types) and the node alias
void OpenLCBMessage::setCANid(uint16_t MTI, uint16_t alias){
	id = ((uint32_t)MTI << 12 | ((uint32_t)alias & 0x0FFF) );
	}
	
void OpenLCBMessage::setNodeidToData(uint64_t nodeId){
	dataLength = 6;
	
	for (uint8_t i = 2; i < 8; i++) {
		//Serial.print((8 * (7 - i))); Serial.print(" ");
		data[i - 2] = (uint8_t) (nodeId >> (8 * (7 - i) & 0xFFUL));
		//util::print8BitHex((uint8_t) (nodeId >> (8 * (7 - i)) & 0xFFUL)); Serial.println();
	}
};

uint64_t OpenLCBMessage::getNodeIdFromData(){
	return (((uint64_t)data[0] << 40) | ((uint64_t)data[1] << 32) | ((uint64_t)data[2] << 24) | ((uint64_t)data[3] << 16) | ((uint64_t)data[4] << 8) | (uint64_t)data[5]);
}
