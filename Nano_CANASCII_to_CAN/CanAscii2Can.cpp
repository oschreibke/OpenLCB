/*
 * CanAscii2Can.cpp
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

// decode a CANASCII message stream to CAN
// returns true when the message is ready to send

#include "CANCommon.h"
#include "CanAscii2Can.h"
#include "Util.h"

boolean firstNybble = true;
WiFiDecodeStatus decodeStatus = expect_colon;

bool CanAscii2Can(uint32_t * id, CAN_message_type * messageType, uint8_t * len, uint8_t dataBuf[], char * ch){
      int i = 0;
      
      switch (decodeStatus) {
        case expect_colon:
          // if we receive a : change the status, initialise the data stores and discard the character
          // otherwise ignore the character - it's not what we were expecting
          if (*ch == ':') {
            decodeStatus = expect_msg_type;
            *messageType = Standard;
            *id = 0;
            *len = 0;
            firstNybble = true;
            //Serial.println("Got colon");
          }
          break;

        case (expect_msg_type):
          // the message Type indicators are S (Standard) and X (Extended)
          // if we receive one of these, set the message type accordingly and change the status
          // otherwise something is wrong therefore we reset to expecting_colon
          decodeStatus = process_identifier;  // assume a valid message type
          switch (*ch) {
            case 'S':
              *messageType = Standard;
              break;

            case 'X':
              *messageType = Extended;
              break;

            default:
              // we shouldn't be here - reset the processor
              decodeStatus = expect_colon;
              break;
          }
          //Serial.println("Got Message type");
          break;

        case (process_identifier):
          // read hex chars until we hit an N
          if (*ch == 'N') {
            decodeStatus = process_data;
            //Serial.print("Got Identifier "); Serial.println(*id, HEX);
            break;
          }

          // convert the identifier from Hex to Int
          i = Hex2Int(*ch);
          //Serial.print(ch); Serial.print(": ");Serial.println(i);
          if (i > 15) {
            // not a hex character => reset the processor
            decodeStatus = expect_colon;
            break;
          }
          *id = (*id << 4) + i;
          //Serial.println("id = "); Serial.println(*id, HEX);
          break;

        case (process_data):
          //Serial.print("Processdata. ch = "); Serial.println(*ch);
          // process data bytes (Hex encoded) until we hit a ;
          if (*ch == ';') {
            // received a ; - send the message and reset the processor for the next
            
            decodeStatus = expect_colon;
            //Serial.println("Exiting CanAscii2Can (true)");
            return true;
          }

          if (firstNybble) {
            dataBuf[*len] = 0x0;
            (*len)++;
          }
          
          if (*len > 8){
			// message has too much data (max 8)  
            decodeStatus = expect_colon;
            break;			  
			}
          

          firstNybble = !firstNybble;
          i = Hex2Int(*ch);
          if (i > 15) {
            // not a hex character => reset the processor
            decodeStatus = expect_colon;
            break;
          }

          // *len contains the actual length, which is 1 greater than the index value
          dataBuf[(*len) - 1] = (dataBuf[(*len) - 1] << 4) + i;
          break;

        default:
          // should not be here - just reset the processor
          decodeStatus = expect_colon;
          break;
      }
	return false;
}

