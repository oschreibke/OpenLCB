/*
 * OpenLCB_CDI_Model.h
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


#ifndef _OPENLCB_CDI_MODEL_H_
#define _OPENLCB_CDI_MODEL_H_

#ifndef byte
#define byte uint8_t
#endif

#include "canmessage.h"

#include "cdidata.h"

// forward declarations
bool SendMessage(); 
bool SendDROK(uint16_t senderAlias, uint16_t destAlias, bool pending);
bool sendSNIHeader(uint16_t senderAlias, uint16_t destAlias);
bool sendSNIReply(uint16_t senderAlias, uint16_t destAlias, const char infoText[]);
bool sendSNIUserHeader(uint16_t senderAlias, uint16_t destAlias);
bool sendSNIUserReply(uint16_t senderAlias, uint16_t destAlias, const char userValue);
void SendDatagram(const uint16_t destAlias, uint16_t senderAlias, struct CAN_MESSAGE* msg, const char * data, uint16_t dataLength);
void SendWriteReply(const uint16_t destAlias, uint16_t senderAlias, byte * buf, uint16_t errorCode);
void ReceiveDatagram(struct CAN_MESSAGE* m, byte* buffer, uint8_t * ptr);
void ProcessDatagram(uint16_t senderAlias, uint16_t alias, struct CAN_MESSAGE* m, byte* datagram, uint8_t datagramLength);
void DumpEEPROM();
void p(const char *fmt, ... );
void hexDump (const char *desc, void *addr, int len);
void DumpEEPROMFormatted();
uint64_t ReverseEndianness(uint64_t *val);
void processMessage(struct CAN_MESSAGE* cm);
void setUpModel(void);
void setUpNode(void);
void setNodeidToData(struct CAN_MESSAGE* cm, uint64_t nodeId);
uint64_t getEventIdFromData(struct CAN_MESSAGE* cm);
void ShowCdiXmlLength();

#endif
