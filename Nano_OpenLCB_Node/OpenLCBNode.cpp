/*
 * OpenLCBNode.cpp
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


#include "OpenLCBNode.h"
#include "OpenLCBAliasRegistry.h"
#include "OpenLCBCANInterface.h"
#include "util.h"

// Method definitions for OpenLCBNode

OpenLCBNode::OpenLCBNode() {
    const char hexDigits[]  = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    uint16_t serial = readSerialFromEEProm();
    nodeId = NODE_ID + serial;
    alias = 0;

    readEventsFromEEPROM();

    //  for(int i = 0; i < 6; i ++){
    //	  strNodeId[i * 3] = hexDigit[id >> (4 * (11 - i)) & 0x0F];
    //	  strNodeId[(i * 3) + 1] = hexDigit[id >> (4 * ((11 - i - 1)) & 0x0F];
    //	  strNodeId[(i * 3) + 2] = '.';
    //	  }
    //  strNodeid[17] = '\0';

    permitted = false;

    randomSeed((uint32_t)(nodeId  & 0xFFFFFFULL) + micros()); // seed the random number from the low-order 24 bits of the id, plus the elapsed millis
    genAlias();

    Serial.print(F("Node initialised: ")); //Serial.println(strNodeId);
    util::print64BitHex(nodeId);
    Serial.println();
    Serial.print("alias ");
    Serial.println(alias, HEX);

}

/*
void OpenLCBNode::setNodeId(){
  char ch;
  const char hexDigits[]  = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
  uint8_t chix;
  char Id [] = NODE_ID;
  uint16_t serial = readSerialFromEEProm();
  Id[12] = hexDigits[serial >> 12];
  Id[13] = hexDigits[(serial >> 8) & 0x0F];
  Id[15] = hexDigits[(serial >> 4) & 0x0F];
  Id[16] = hexDigits[serial & 0x0F];

  Serial.println(F("Starting setNodeId"));
  // ToDo: normalise the id to nn.nn.nn.nn.nn.nn
  if (strlen(Id) != 17) {
	Serial.print(F("strLen (Id) != 17: ")); Serial.println(strlen (Id));
    nodeId = 0;
    return;
  }
//  Serial.begin(115200);
  Serial.print(F("Converting node id... ")); Serial.println(Id);
  // convert from nn.nn.nn.nn.nn.nn format to an uint64_t
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 2; j++) {
      ch = (char)Id[(i * 3) + j];

	  //Serial.print("i: "); Serial.print(i); Serial.print(", j: "); Serial.print(j); Serial.print(", (i * 3) + j: "); Serial.print((i * 3) + j); Serial.print(", ch: "); Serial.println(ch);

      for (chix = 0; chix < 16; chix++){
		  if (ch == hexDigits[chix])
		      break;
          }

        //Serial.print("ch "); Serial.print(ch); Serial.print(", chix "); Serial.println(chix);

      if (chix < 16) {
        nodeId = (nodeId << 4) + chix;
        //Serial.print("nodeId "); Serial.print((uint32_t) (nodeId >> 32), HEX);Serial.println((uint32_t) (nodeId & 0x00000000FFFFFFFFULL), HEX);
      }
      else { // invalid character in the id
		Serial.print("Invalid character in the node id: ");  Serial.print(ch); Serial.print(" "); Serial.println(ch, HEX);
        nodeId = 0;
        return;
      }
    }
    if (i < 5 && Id[(i * 3) + 2] != '.') {
      // node id is not properly normalised
      Serial.println(F("node id is not properly normalised"));
      nodeId = 0;
      return;
    }
  }
  // we got this far, the node id must be OK
  strcpy (strNodeId, Id);

  randomSeed((uint32_t)(nodeId  & 0xFFFFFFULL) + micros()); // seed the random number from the low-order 24 bits of the id, plus the elapsed millis

  genAlias();

Serial.print(F("Node initialised: ")); Serial.println(strNodeId);
util::print64BitHex(nodeId); Serial.println();
Serial.print("alias "); Serial.println(alias, HEX);
}
*/

// this is where the work gets done
void OpenLCBNode::loop() {
    processIncoming();

    if (permitted) {
    } else {
        // wait for the alias registration for jmri (process any others too)
        //		if (registry.JMRIRegistered()){
        // now try registering our alias
        // send the initialised message
        registerMe();
        //NodeAliasStatus status = registry.getStatus(alias);
        //permitted = (status == nodeAliasNotFound); // test - only attempt registration once
        //			}
    }
}

uint64_t OpenLCBNode::getNodeId() {
    return nodeId;
}

uint16_t OpenLCBNode::getAlias() {
    return alias;
}

void OpenLCBNode::setCanInt(OpenLCBCANInterface* canInterface) {
    canInt = canInterface;
}

//char* OpenLCBNode::ToString(){
//  return strNodeId;
//}

bool OpenLCBNode::registerMe() {
    //Serial.println("RegisterMe");
    // register the node's alias on the system
    bool fail = false;

    NodeAliasStatus status = registry.getStatus(alias);
    //Serial.print(alias); Serial.print(", status: "); Serial.println(status);
    switch (status) {
    case nodeAliasNotFound:
        //msgOut.setId((uint32_t) CID1 << 12 | ((uint32_t) (nodeId >> 36)) & 0x0FFF);
        msgOut.setCANid(CID1 | ((uint16_t) (nodeId >> 36) & 0x0FFF), alias);
        msgOut.setDataLength(0);
        Serial.print(F("CID1: "));
        Serial.println(*msgOut.getPId(), HEX);
        if (canInt->sendMessage(&msgOut)) {
            registry.add(alias, nodeId, CID1received);
        } else {
            fail = true;
            break;  // bail if we couldn't send the message
        }

        break;

    case CID1received:
        //msgOut.setId((uint32_t)CID2 << 12 | ((uint32_t) (nodeId >> 24)) & 0x0FFF);
        msgOut.setCANid(CID2 | ((uint16_t) (nodeId >> 24) & 0x0FFF), alias);
        msgOut.setDataLength(0);
        Serial.print("CID2: ");
        Serial.println(*msgOut.getPId(), HEX);
        if (canInt->sendMessage(&msgOut)) {
            registry.setStatus(alias, CID2received);
        } else {
            registry.remove(alias);                // start again if we couldn't send the message
            fail = true;
        }
        break;

    case CID2received:
        //msgOut.setId((uint32_t)CID3 << 12 | ((uint32_t) (nodeId >> 12)) & 0x0FFF);
        msgOut.setCANid(CID3 | ((uint16_t) (nodeId >> 12) & 0x0FFF), alias);
        msgOut.setDataLength(0);
        Serial.print(F("CID3: "));
        Serial.println(*msgOut.getPId(), HEX);
        if (canInt->sendMessage(&msgOut)) {
            registry.setStatus(alias, CID3received);
        } else {
            registry.remove(alias);                // start again if we couldn't send the message
            fail = true;
        }

        break;

    case CID3received:
        //msgOut.setId((uint32_t)CID4 << 12 | ((uint32_t) nodeId & 0x0FFF));
        msgOut.setCANid(CID4 | ((uint16_t) nodeId & 0x0FFF), alias);
        msgOut.setDataLength(0);
        Serial.print(F("CID4: "));
        Serial.println(*msgOut.getPId(), HEX);
        if (canInt->sendMessage(&msgOut)) {
            registry.setStatus(alias, CID4received);
        } else {
            registry.remove(alias);                // start again if we couldn't send the message
            fail = true;
        }
        waitStart = millis();
        break;

    case CID4received:
        // if we've waited 200 millisec, and no one has objected, send the RID;
        if (millis() > waitStart + 200) {
            fail = true;                   // assume bad things are going to happen

            msgOut.setCANid(RID, alias);
            msgOut.setDataLength(0);

            Serial.print(F("RID: "));
            Serial.println(*msgOut.getPId(), HEX);

            if (canInt->sendMessage(&msgOut)) {
                registry.setStatus(alias, RIDreceived);

                // transition to permitted
                // Send a AMD message
                msgOut.setCANid(AMD, alias);
                msgOut.setNodeidToData(nodeId);

                if (canInt->sendMessage(&msgOut)) {
                    permitted = true;

                    // send an initialisation complete message
                    msgOut.setCANid(INIT, alias);
                    msgOut.setNodeidToData(nodeId);

                    if (canInt->sendMessage(&msgOut)) {
                        fail = false;	 // bad things didn't happen
                    }
                }
            }
            break;
        }
    }

    if (fail) {
        // if something failed, retry with another alias
        Serial.println(F("fail"));
        registry.remove(alias);
        genAlias();
    }
}

//
// private functions
//
void OpenLCBNode::genAlias() {
    alias = random(1, 0x0FFF);
    while (registry.getStatus(alias) !=  nodeAliasNotFound) // make sure no one is using it
        alias = random(1, 0x0FFF);                          // otherwise generate a new one
}

void OpenLCBNode::processIncoming() {
    uint32_t msgId;
    uint16_t senderAlias;
    uint64_t senderNodeId;
    byte dataBytes[8];

    if (canInt->receiveMessage(&msgIn)) {

        Serial.print(F("Processing message: Id = "));
        Serial.print(msgIn.getId(), HEX);
        Serial.print(F(", MTI = "));
        Serial.println(msgIn.getMTI(), HEX);
        Serial.print(F("Control Message: "));
        Serial.println((msgIn.isControlMessage()?"Y":"N"));
        msgId = msgIn.getId();
        senderAlias = msgIn.getSenderAlias();
        if (msgIn.isControlMessage()) { // Alias management

            // this section is short of error handling
            // it assumes everyone plays by the rules and that we don't miss any messages

            switch ((ControlId) msgIn.getMTI()) {
            case (RID):
                if (senderAlias == alias) {
                    // has someone responded to our CID request?
                    registry.remove(alias); // remove our alias (forces a restart)
                } else {
                    // registering his own alias (we should already know this alias)
                    registry.setStatus(senderAlias, RIDreceived);
                }
                break;

            case (AMD):
                break;
            case (AME):
                if (permitted) {                   // only respond if in permitted state
                    senderNodeId = nodeId;         // if the sender didn't send a node id, use ours
                    switch (msgIn.getDataLength()) {
                    case 6:                    // the sender included a node id: is it ours?
                        senderNodeId = 0;
                        for (uint8_t i = 0; i < 6; i++) {
                            senderNodeId = (senderNodeId << 8) + msgIn.getDataByte(i);
                        }
                    // no break - goes somewhat against the grain, but can save some code this way
                    case 0:
                        if (nodeId == senderNodeId) { // check the node id
                            msgOut.setCANid(AMD, alias);
                            msgOut.setNodeidToData(nodeId);
                            canInt->sendMessage(&msgOut);
                        }
                    }
                }
                break;
            case (AMR):
                break;
            default:
                switch ((byte)((msgId & 0x07FFF000) >> 20)) {
                // the CIDs
                case (0x07): //CID1
                    registry.add(senderAlias, ((uint64_t) (msgId & 0x00FFF000)) << 24, CID1received);
                    break;

                case (0x06): //CID2
                    senderNodeId = registry.getNodeId(senderAlias);
                    senderNodeId |= ((uint64_t) msgId & 0x00FFF000) << 12;
                    registry.setNodeId(senderAlias, senderNodeId);
                    registry.setStatus(senderAlias, CID2received);
                    break;

                case (0x05): //CID3
                    senderNodeId = registry.getNodeId(senderAlias);
                    senderNodeId |= ((uint64_t) msgId & 0x00FFF000);
                    registry.setNodeId(senderAlias, senderNodeId);
                    registry.setStatus(senderAlias, CID3received);
                    break;

                case (0x04): //CID4
                    senderNodeId = registry.getNodeId(senderAlias);
                    senderNodeId |= ((uint64_t) msgId & 0x00FFF000) >> 12;
                    registry.setNodeId(senderAlias, senderNodeId);
                    registry.setStatus(senderAlias, CID2received);
                    break;

                default:
                    break;
                }
                break;
            }
        } else {
            MTI mtiIn = msgIn.getMTI();
            bool addressedToMe = ((mtiIn & 0x0008) > 0) && (msgIn.getDestAliasFromData() == alias);
            uint32_t protflags = 0;
            Serial.print(F("Processing MTI: "));
            Serial.print(mtiIn, HEX);
            Serial.println();
            switch (mtiIn) {
            /*
             *4.1.2 Messages Received
            Simple nodes shall receive any message specifically addressed to them, plus the following unaddressed
            global messages:
            • Verify Node ID ;
            • Verified Node ID;
            • Protocol Support Inquiry; (always addressed??)
            • Identify Consumers;
            • Identify Producers;
            • Identify Events;
            • Learn Event;
            • P/C Event Report;
            */

            /*
            • Verify Node ID – because they need to reply to it;
            • Verified Node ID – because it may be the reply to their own request, and it might be used to
              e.g. locate a node for delayed sending of status;
            • Protocol Support Inquiry – because they need to reply to it;
            • Identify Consumers, Identify Producers, Identify Events – because others will ask this of them,
              and they may need to reply;
            • Learn Event – so they can be programmed;
            • P/C Event Report – so they can take actions.
             */
            case PSI:   // Protocol Support Inquiry
                protflags = SPSP | SNIP;
                msgOut.setCANid(PSR, alias);  // Respond with a Protocol Support Reply
                dataBytes[0] = (byte)(senderAlias >> 8);
                dataBytes[1] = (byte)(senderAlias & 0xFF);
                dataBytes[2] = (byte)(protflags >> 16);
                dataBytes[3] = (byte)(protflags >> 8) & 0xFF;
                dataBytes[4] = (byte)(protflags & 0xFF);
                dataBytes[5] = '\0';
                dataBytes[6] = '\0';
                dataBytes[7] = '\0';
                msgOut.setData(&dataBytes[0], 8);
                canInt->sendMessage(&msgOut);
                break;

            case VNIA:  // Verify Node ID Number Addressed
            case VNIG:  // Verify Node ID Number Global
                /*
                				Upon receipt of a directed (addressed) Verify Node ID message addressed to it, a node shall reply with
                				an unaddressed Verified Node ID message.

                				Upon receipt of a global (unaddressed) Verify Node ID message that does not contain an (optional)
                				Node ID, a node shall reply with an unaddressed Verified Node ID message.

                				Upon receipt of a global (unaddressed) Verify Node ID message that contains an (optional) Node ID, a
                				node will reply with an unaddressed Verified Node ID message, if and only if the receiving node's
                				Node ID matches the one received.
                 */

                Serial.println(F("Verify Node ID"));

                switch (msgIn.getDataLength()) {
                case 0:   // no data
                    break;
                case 6:   // received a node id
                    if (nodeId != msgIn.getNodeIdFromData())
                        return;
                    break;
                case 8:   // received an alias + node id
                    // just check the alias
                    if (!addressedToMe)
                        return;
                    break;
                default:
                    sendOIR(0x1080, senderAlias, mtiIn); // Invalid arguments. Some of the values sent in the message fall outside of the
                    // expected range, or do not match the expectations of the receiving node.
                    return;
                }

                msgOut.setCANid(VNN, alias);    // respond with a Verified Node ID Number reply
                msgOut.setNodeidToData(nodeId);
                canInt->sendMessage(&msgOut);
                return;

            // Identify Consumers
            case IC: {
                uint64_t eventId = msgIn.getEventIdFromData();
                int eventIx = 0;
                for (eventIx = 0; eventIx < numEvents; eventIx++) {
                    if (eventId == eventCache[eventIx].eventId)
                        break;
                }
                if (!(eventIx < numEvents))    // not one of mine
                    return;

                byte* pDataIn  = msgIn.getPData();
                byte* pDataOut = msgOut.getPData();
                msgOut.setDataLength(msgIn.getDataLength());
                msgOut.setCANid(CICV, alias);    // Consumer Identified as currently valid
                for (uint8_t i = 0; i < msgIn.getDataLength(); i++) {
                    *pDataOut++ = *pDataIn++;
                }
                canInt->sendMessage(&msgOut);

                return;
            }
            // Identify Producers
            case IP:
                //if (nodeId != msgIn.getNodeIdFromData())    // not an event I produce
                //    return;
                // I don't produce events
                return;
            // Identify Events
            case IEA:
                break;
            //				// Learn Event
            //				case LE:
            //				    break;
            // P/C Event Report
            case PCER: {
                // handle the global events
                uint64_t eventId = msgIn.getEventIdFromData();
                int eventIx = 0;
                for (eventIx = 0; eventIx < numEvents; eventIx++) {
                    if (eventId == eventCache[eventIx].eventId)
                        break;
                }
                if (!(eventIx < numEvents))    // not one of mine
                    return;

                /*
                				    switch ((uint16_t)msgIn.getDataByte(6) << 8 | (uint16_t)msgIn.getDataByte(7)){
                						case 1:
                						     Serial.print(F("Received event 1"));
                						     return;

                						case 2:
                						     Serial.print(F("Received event 2"));
                						     return;

                						case 3:
                						     Serial.print(F("Received event 3"));
                						     return;

                						case 4:
                						     Serial.print(F("Received event 4"));
                						     return;

                						default:
                				            sendOIR(0x1080, senderAlias, mtiIn); // Invalid arguments. Some of the values sent in the message fall outside of the
                                                                                 // expected range, or do not match the expectations of the receiving node.
                                            return;
                						}
                */

                switch (eventCache[eventIx].eventType) {
                case OnOff:
                    digitalWrite(eventCache[eventIx].pin, eventCache[eventIx].eventValue);
                    break;

                case SBBSignalAspect:
                    break;
                }
                return;
            }

            // Simple Node Information Request
            case SNIIRQ: {
                if (!addressedToMe)
                    return;


                //Serial.println(F("Simple Node Information Request"));
                //Serial.println(Manufacturer);
                //Serial.println(ModelName);
                //Serial.println(HardwareVersion);
                //Serial.println(SoftwareVersion);
                //Serial.println(UserName);
                //Serial.println(UserDescription);

                sendSNIHeader(alias, senderAlias);
                sendSNIReply(alias, senderAlias, Manufacturer);
                sendSNIReply(alias, senderAlias, ModelName);
                sendSNIReply(alias, senderAlias, HardwareVersion);
                sendSNIReply(alias, senderAlias, SoftwareVersion);
                sendSNIUserHeader(alias, senderAlias);
                sendSNIUserReply(alias, senderAlias, 'N');
                sendSNIUserReply(alias, senderAlias, 'D');

                break;
            }
            default:
                // If the message is adressed to me, send an OIR (Not Implemented)
                if (addressedToMe) {
                    sendOIR(0x1040, senderAlias, mtiIn);  // Not implemented
                }
                // ignore the message
                break;
            }
        }
    }
}

bool OpenLCBNode::sendOIR(uint16_t errorCode, uint16_t senderAlias, MTI mti) {
    byte dataBytes[6];

    msgOut.setCANid(OIR, alias);    // respond with a Optional Interaction Rejected message
    dataBytes[0] = (byte)(senderAlias >> 8);
    dataBytes[1] = (byte)(senderAlias & 0xFF);
    dataBytes[2] = (byte)(errorCode >> 8);
    dataBytes[3] = (byte)(errorCode & 0xFF);
    dataBytes[4] = (byte)(mti >> 8);
    dataBytes[5] = (byte)(mti & 0xFF);
    msgOut.setData(&dataBytes[0], 6);
    return canInt->sendMessage(&msgOut);
}


uint16_t OpenLCBNode::readSerialFromEEProm() {
    OpenLCBEEPROM eepromData;
    int  eeAddress = 0;
    EEPROM.get(eeAddress, eepromData);
    return eepromData.serial;
}

void OpenLCBNode::readUserNameFromEEProm(char * buf) {
    OpenLCBEEPROM eepromData;
    int  eeAddress = 0;
    EEPROM.get(eeAddress, eepromData);
    if (strlen(eepromData.userName) <= 63)
        strcpy(buf, eepromData.userName);
    else
        strcpy(buf, "Error getting user name");
}

void OpenLCBNode::readUserDescriptionFromEEProm(char * buf) {
    OpenLCBEEPROM eepromData;
    int  eeAddress = 0;
    EEPROM.get(eeAddress, eepromData);
    if (strlen(eepromData.userName) <= 64)
        strcpy(buf, eepromData.userDescription);
    else
        strcpy(buf, "Error getting user description");
}

void OpenLCBNode::readEventsFromEEPROM() {
    OpenLCBEvent e;
    int eeAddress = sizeof(OpenLCBEEPROM) - sizeof(uint8_t);
    uint8_t events;
    EEPROM.get(eeAddress, events);
    eeAddress = sizeof(OpenLCBEEPROM);

    for(int i = 0; i < events; i++) {
        EEPROM.get(eeAddress, e);
        eventCache[i].eventId = e.eventId;
        eventCache[i].eventType = e.eventType;
        eventCache[i].eventValue = e.eventValue;
        eventCache[i].pin = e.pin;

        if (eventCache[i].eventType == OnOff) {
            pinMode(eventCache[i].pin, INPUT);
        }

        eeAddress += sizeof(OpenLCBEvent);
    }
}



bool OpenLCBNode::sendSNIHeader(uint16_t senderAlias, uint16_t destAlias) {
    byte dataBytes[3];

    msgOut.setCANid(SNIIR, senderAlias);
    dataBytes[0] = (byte)(destAlias >> 8) | 0x10;  // first frame
    dataBytes[1] = (byte)(destAlias & 0xFF);
    dataBytes[2] = 0x04;   // version number: sending manufacturer name, node model name, node hardware version and node software version
    msgOut.setData(&dataBytes[0], 3);
    return canInt->sendMessage(&msgOut);
}

bool OpenLCBNode::sendSNIReply(uint16_t senderAlias, uint16_t destAlias, const char infoText[]) {
    byte dataBytes[8];
    uint8_t pos = 0;
    uint8_t dataLen = 2;
    bool fail = false;

    Serial.print(F("infoText ("));
    Serial.print(strlen(infoText));
    Serial.print(F(") "));
    Serial.println(infoText);

    msgOut.setCANid(SNIIR, senderAlias);
    dataBytes[0] = (byte)(destAlias >> 8);
    dataBytes[1] = (byte)(destAlias & 0xFF);

    while(pos < strlen(infoText) +1) {

        //Serial.print(F("Character at ")); Serial.print(pos); Serial.print(F(": ")); Serial.print(infoText[pos]); Serial.print(F(" dataBytes[")); Serial.print(dataLen);	Serial.print(F("]: "));
        dataBytes[dataLen++] = infoText[pos++];
        //Serial.println((char)dataBytes[dataLen - 1]);

        dataBytes[0] |= 0x30;  // intermediate frame

        if (dataLen > 7 || pos > strlen(infoText)) {
            msgOut.setData(&dataBytes[0], dataLen);
            delay(50);   // add delay otherwise messages are not sent in sequence
            if (!canInt->sendMessage(&msgOut)) {
                fail = true;
                break;
            }
            dataLen = 2;
        }
    }
    return fail;
}

bool OpenLCBNode::sendSNIUserHeader(uint16_t senderAlias, uint16_t destAlias) {
    byte dataBytes[3];

    msgOut.setCANid(SNIIR, senderAlias);
    dataBytes[0] = (byte)(destAlias >> 8) | 0x30; // intermediate frame
    dataBytes[1] = (byte)(destAlias & 0xFF);
    dataBytes[2] = 0x02;   // version number: sending user-provided node name, user-provided node description
    msgOut.setData(&dataBytes[0], 3);
    return canInt->sendMessage(&msgOut);
}

bool OpenLCBNode::sendSNIUserReply(uint16_t senderAlias, uint16_t destAlias, const char userValue) {
    byte dataBytes[8];
    char infoText[64];
    uint8_t pos = 0;
    uint8_t dataLen = 2;
    bool fail = false;

    switch (userValue) {
    // User Name
    case 'N':
        readUserNameFromEEProm(infoText);
        break;

    // User Description
    case 'D':
        readUserDescriptionFromEEProm(infoText);
        break;

    default:
        return fail; // invalid parameter
    }

    //Serial.print(F("User infoText ("));
    //Serial.print(strlen(infoText));
    //Serial.print(F(") "));
    //Serial.println(infoText);

    msgOut.setCANid(SNIIR, senderAlias);
    dataBytes[0] = (byte)(destAlias >> 8);
    dataBytes[1] = (byte)(destAlias & 0xFF);

    while(pos < strlen(infoText) +1) {

        //Serial.print(F("Character at ")); Serial.print(pos); Serial.print(F(": ")); Serial.print(infoText[pos]); Serial.print(F(" dataBytes[")); Serial.print(dataLen);	Serial.print(F("]: "));
        dataBytes[dataLen++] = infoText[pos++];
        //Serial.println((char)dataBytes[dataLen - 1]);

        if (!(userValue == 'D' && pos > strlen(infoText))) {
            dataBytes[0] |= 0x30;  // intermediate frame
        } else {
            dataBytes[0] &= ~0x10;  // last frame
        }
        if (dataLen > 7 || pos > strlen(infoText)) {
            msgOut.setData(&dataBytes[0], dataLen);
            delay(50);   // add delay otherwise messages are not sent in sequence
            if (!canInt->sendMessage(&msgOut)) {
                fail = true;
                break;
            }
            dataLen = 2;
        }
    }
    return fail;
}
