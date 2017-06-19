/*
 * OpenLCBNode.h
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

#ifndef OpenLCBNodeIncluded
#define OpenLCBNodeIncluded

#if (ARDUINO >= 100)
    #include "Arduino.h"
#else
    #include "WProgram.h"    
#endif

#include "OpenLCBMessage.h"
#include "OpenLCBAliasRegistry.h"
#include "OpenLCBCANInterface.h"
#include <EEPROM.h>

// private ID range?  d DCC - DIY
// actual id will be set from the eeprom when initialising
//#define NODE_ID_STR "08.01.00.0D.00.00" 
#define NODE_ID     0x0801000D0000ULL 

struct OpenLCBEEPROM{
    uint16_t  serial;
    char      userName[63];
    char      userDescription[64];
    uint8_t   events;  // maximum 20
};

enum EVENTTYPE:uint8_t {OnOff, SBBSignalAspect};

struct OpenLCBEvent{
	uint64_t eventId;
	EVENTTYPE eventType;
	uint8_t  eventValue;
	uint8_t  pin;
	char     eventName[32];
};

struct eventCacheEntry{
	uint64_t eventId;
	EVENTTYPE eventType;
	uint8_t  eventValue;
	uint8_t  pin;
	};

class OpenLCBNode {
    uint64_t nodeId;    // 48-bit id
    uint16_t alias;     // 16-bit alias
//    char strNodeId[18]; // = "00.00.00.00.00.00";
    bool permitted;     // false => inhibited
    OpenLCBMessage msgIn, msgOut;
    OpenLCBAliasRegistry registry;
    uint32_t waitStart;
    OpenLCBCANInterface* canInt;
    
    const char* Manufacturer  = "Solwiz.ch";
    const char* ModelName     = "Arduino Nano LCC Node";
    const char* HardwareVersion  = "0.1";
    const char* SoftwareVersion  = "0.1";
    //const char* UserName         = "Test Node";
    //const char* UserDescription  = "Test Node 1";

    uint8_t numEvents;
    eventCacheEntry eventCache[20];

  public:
    OpenLCBNode();
    uint64_t getNodeId();
    uint16_t getAlias();
    void setCanInt(OpenLCBCANInterface* canInt);
 //   char* ToString();
    void loop();
//    void setNodeId();

  private:
    void genAlias();
    void processIncoming();
    uint16_t readSerialFromEEProm();
    void readUserNameFromEEProm(char* buf);
    void readUserDescriptionFromEEProm(char * buf);    
    void readEventsFromEEPROM();
    bool registerMe();
 //   bool sendAMD();
    bool sendOIR(uint16_t errorCode, uint16_t senderAlias, MTI mti);  
    bool sendSNIHeader(uint16_t senderAlias, uint16_t destAlias);
    bool sendSNIReply(uint16_t senderAlias, uint16_t destAlias, const char InfoText[]);  
    bool sendSNIUserHeader(uint16_t senderAlias, uint16_t destAlias);    
    bool sendSNIUserReply(uint16_t alias, uint16_t senderAlias, const char UserValue);    
};


#endif
