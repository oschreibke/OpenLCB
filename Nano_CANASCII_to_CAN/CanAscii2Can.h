/*
 * CanAscii2Can.h
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
 
#ifndef CANASCII2CANINCLUDED
#define CANASCII2CANINCLUDED
#if (ARDUINO >= 100)
    #include "Arduino.h"
#else
    #include "WProgram.h"    
#endif

#include "CANCommon.h"

enum WiFiDecodeStatus {expect_colon, expect_msg_type, process_identifier, process_data};

//CAN_message_type  WiFiMessageType = Standard;
//long unsigned int WiFiIdentifier = 0;
//byte WiFiDataLen = 0;
//byte WiFiData[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

bool CanAscii2Can(uint32_t * id, CAN_message_type * messageType, uint8_t * len, uint8_t buf[], char * ch);
uint8_t Hex2Int(char ch);

#endif
