/*
 * Can2CanAscii.cpp
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
 
// encode a can message to CANASCII

#include "CANCommon.h"
#include "Can2CanAscii.h"
#include "Util.h"

void Can2CanAscii(uint32_t * CANIdentifier, uint8_t * CANDataLen, uint8_t CANData[], char CANAscii[]){
	CAN_message_type  CANMessageType = Extended;        // always extended for OpenLCB
    //char buf[50]; 
    uint8_t ch;
	uint8_t chPos = 2;

    //Serial.print("in Can2CanAscii. CANIdentifier = "); Serial.print(* CANIdentifier, HEX); Serial.print(", CANDataLen = "); Serial.println(*CANDataLen);
    //Serial.print("CANId = "); Serial.println(CANId, HEX);


    memset(CANAscii, '\0', CANASCII_SIZE);
    
    if (CANMessageType == Extended){
		*CANIdentifier &= 0x7FFFFFFF;
		//*CANIdentifier &= 0x1FFFFFFF;  // mask off to 29 bits

		//sprintf(CANAscii, ":X%08lXN", *CANIdentifier);
		strcpy(CANAscii, ":X");
		for (int8_t i = 3; i >= 0; i--){  // signed int - I need it to go < 0
			 ch = (uint8_t)(((*CANIdentifier) >> (8 * i)) & 0x000000FF);
			 //Serial.print("ch = "); Serial.println(ch, HEX);
			 //Serial.print("*CANIdentifier = "); Serial.println(*CANIdentifier, HEX);
			 CANAscii[chPos++] = Nybble2Hex(ch >> 4);
			 CANAscii[chPos++] = Nybble2Hex(ch & 0x0F); 
			 } 
	} else {
	     //sprintf(CANAscii, ":S%03lXN", *CANIdentifier);
	     strcpy(CANAscii, ":S");
	     CANAscii[1] = Nybble2Hex((uint8_t)((*CANIdentifier >> 8) & 0x0F));
	     CANAscii[2] = Nybble2Hex((uint8_t)((*CANIdentifier >> 4) & 0x0F));
	     CANAscii[3] = Nybble2Hex((uint8_t)(*CANIdentifier & 0x0F));
	}
    
	strcat(CANAscii, "N");	
	
	//Serial.print("CANAscii (");	Serial.print(strlen(CANAscii));	Serial.print(") ");	Serial.println(CANAscii);	 
	//Serial.print("CANDataLen = "); Serial.println(*CANDataLen);
	
    chPos = strlen(CANAscii);
    
	for (uint8_t i = 0; i < *CANDataLen; i++){     // write any data bytes
	    //Serial.print("Writing data byte "); Serial.print(i); Serial.print(" to CANAscii["); Serial.print(chPos); Serial.print("], data = "); Serial.println(CANData[i], HEX);
	    //sprintf(buf, "%02hhX", *(CANData + i));
	    //strcat(CANAscii, buf);
	    CANAscii[chPos++] = Nybble2Hex((uint8_t)((CANData[i] >> 4) & 0x0F));
	    CANAscii[chPos++] = Nybble2Hex((uint8_t)(CANData[i] & 0x0F));
	    
	    //Serial.print("CANAscii (");	Serial.print(strlen(CANAscii));	Serial.print(") ");	Serial.println(CANAscii);	 
	}

	strcat(CANAscii, ";");                        // add the terminator
	
	}
