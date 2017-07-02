/*
 * ESP8266_OpenLCB_CDI_Model.ino
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

// working model for OpenLCB configuration


/*
   From: https://gridconnect.com/media/documentation/grid_connect/CAN_USB232_UM.pdf

   Normal CAN Message Syntax

   : <S | X > <IDENTIFIER> <N> <Data0> <Data1> ... <Data7> ;

   RTR CAN Message Syntax

   : <S | X > <IDENTIFIER> <R> <Length> ;

Parts from
  WiFiTelnetToSerial - Example Transparent UART to Telnet Server for esp8266

  Copyright (c) 2015 Hristo Gochkov. All rights reserved.
  This file is part of the ESP8266WiFi library for Arduino environment.
*/
/* Example CAN-ASCII Messagesfrom the JMRI OpenLCB tool

  Bytes Available: 12
  :X170002B7N;
  Bytes Available: 60
  :X160002B7N;:X150002B7N;:X140002B7N;:X107002B7N;:X194902B7N;
  Bytes Available: 18
  :X00000123N120456;
  Bytes Available: 18
  :X00000123N120456;
  Bytes Available: 18
  :X00000123N120456;
  Bytes Available: 26
  :X1A2B72B7N20610000000008;
  Bytes Available: 16
  :X199702B7N02B7;
  Bytes Available: 20
  :X00000123N12345678;
*/

#include <ESP8266WiFi.h>
// handling of the CAN adapter format is performed by the MCP2515 library (mcp_can)

#include "CANCommon.h"
#include "CanAscii2Can.h"
#include "Can2CanAscii.h"
#include "OpenLCBMessage.h"
#include "OpenLCBCDI.h"
#include "Util.h"



struct OpenLCBEEPROMHeader {
    uint16_t  serial;
    char      userName[63];
    char      userDescription[64];
    uint8_t  events; // maximum 20
};

enum EVENTTYPE:uint8_t {I2COutput, I2CInput, pinOutput, pinInput};

struct OpenLCBEvent {
    uint64_t eventId;
    EVENTTYPE eventType;
    uint8_t  address;
    uint8_t  eventValue;
    char     eventName[33];
};

// build the data in RAM
struct EEPROM_Data {
    OpenLCBEEPROMHeader header;
    OpenLCBEvent event[20];
};

const uint64_t nodeId = 0x0501010131FFULL;
const uint16_t alias = 0x123;

// function templates
bool initialiseESP8266(WiFiServer* server);

// ESP8266 set up

const char* ssid = "Schreibke";
const char* password = "******";
const IPAddress SERVERIP(192, 168, 0, 112);
const IPAddress GATEWAY(192, 168, 0, 1);
const IPAddress SUBNET(255, 255, 255, 0);
const IPAddress DNS(192, 168, 0, 32);

WiFiServer wifiServer(23);  //listen to port 23
WiFiClient wifiClient;


#define SHOWMESSAGES

//#ifdef SHOWMESSAGES
//#include <SoftwareSerial.h>

//SoftwareSerial mySerial(5, 4); // RX, TX
//#endif


// send and receive buffers for the can packets
// CAN => received from the CAN interface; WiFi => received over WiFi



CAN_message_type  WiFiMessageType = Standard;
uint32_t WiFiIdentifier = 0;
uint8_t WiFiDataLen = 0;
uint8_t WiFiData[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

char msgString[128];                        // Array to store serial string

bool wifiInitialised = false;
enum NodeStatus {uninitialised, initialised};
enum CIDSent {noneSent, CID1Sent, CID2Sent, CID3Sent, CID4Sent, RIDSent, AMDSent, INITSent};

NodeStatus ns = uninitialised;
CIDSent cidSent = noneSent;

bool sendCDI = false;
//uint8_t sendCDIStep = 0;
bool waitForDatagramACK = false;
uint32_t addressOffset = 0;

OpenLCBMessage msgIn;
OpenLCBMessage msgOut;

uint16_t senderAlias = 0;

byte datagramBuffer[72];
uint8_t datagramPtr = 0;

EEPROM_Data eed;

void setup() {
    // start the uart
    Serial.begin(115200);
#ifdef SHOWMESSAGES
    Serial.print("\nStarting...\nESP8266 Connecting to ");
    Serial.println(ssid);
#endif

    // initialise the ESP8266
    wifiInitialised = initialiseESP8266(&wifiServer);

    if (wifiInitialised) {
        Serial.println("Connection OK");
    } else {
        Serial.print("Connection failed");
    }

    OpenLCBCDI cdi;

    memset(&eed, 0x00, sizeof(EEPROM_Data));

    eed.header.serial = 0xFFFF;
    strcpy((char *)&eed.header.userName, "my first Node");
    strcpy((char *)&eed.header.userDescription, "first node for cdi");

    cdi.ShowItemLengths();
    //cdi.AssembleXML();


}

void loop() {
    byte dataBytes[8];
    //	char buf[50];

    // initialisation failed - not much we can do here
    if (!wifiInitialised)
        return;

    // Is any one listening? If not there's no point processing anything
    if (!wifiClient.connected()) {
        if (wifiServer.hasClient())    {
            // a new client has connected
            wifiClient = wifiServer.available();
        } else {
            return;
        }
    }

    msgOut.setId(0);

    switch (ns) {
    case uninitialised:
        switch(cidSent) {
        /*
        //case none:
        //msgOut.setCANid(CID1 | ((uint16_t) (nodeId >> 36) & 0x0FFF), alias);
        //msgOut.setDataLength(0);
        //Serial.print(F("CID1: "));
        //Serial.println(*msgOut.getPId(), HEX);
        //if (canInt->sendMessage(&msgOut)) {
        //registry.add(alias, nodeId, CID1received);
        //} else {
        //fail = true;
        //break;  // bail if we couldn't send the message
        //}
        //break;
        //case CID1:
        //msgOut.setCANid(CID2 | ((uint16_t) (nodeId >> 24) & 0x0FFF), alias);
        //msgOut.setDataLength(0);
        //Serial.print("CID2: ");
        //Serial.println(*msgOut.getPId(), HEX);
        //if (canInt->sendMessage(&msgOut)) {
        //registry.setStatus(alias, CID2received);
        //} else {
        //registry.remove(alias);                // start again if we couldn't send the message
        //fail = true;
        //}
        //break;
        //case CID2:
        //msgOut.setCANid(CID3 | ((uint16_t) (nodeId >> 12) & 0x0FFF), alias);
        //msgOut.setDataLength(0);
        //Serial.print(F("CID3: "));
        //Serial.println(*msgOut.getPId(), HEX);
        //if (canInt->sendMessage(&msgOut)) {
        //registry.setStatus(alias, CID3received);
        //} else {
        //registry.remove(alias);                // start again if we couldn't send the message
        //fail = true;
        //}

        //break;
        //case CID3:
        ////msgOut.setId((uint32_t)CID4 << 12 | ((uint32_t) nodeId & 0x0FFF));
        //msgOut.setCANid(CID4 | ((uint16_t) nodeId & 0x0FFF), alias);
        //msgOut.setDataLength(0);
        //Serial.print(F("CID4: "));
        //Serial.println(*msgOut.getPId(), HEX);
        //if (canInt->sendMessage(&msgOut)) {
        //registry.setStatus(alias, CID4received);
        //} else {
        //registry.remove(alias);                // start again if we couldn't send the message
        //fail = true;
        //}
        //waitStart = millis();
        //break;
        //case CID4:
        //break;
        */
        case noneSent:
            msgOut.setCANid(RID, alias);
            msgOut.setDataLength(0);

            //Serial.print(F("RID: "));
            //Serial.println(*msgOut.getPId(), HEX);
            cidSent = RIDSent;
            break;

        case RIDSent:

            // transition to permitted
            // Send a AMD message
            msgOut.setCANid(AMD, alias);
            msgOut.setNodeidToData(nodeId);
            cidSent = AMDSent;
            //Serial.println("Preparing AMD");
            //Serial.print("CANid: "); Serial.println(msgOut.getId(), HEX);
            //Serial.print("datalength: "); Serial.println(msgOut.getDataLength());
            //Serial.print("data: ");
            //for (int i = 0; i < 8; i++){
            //Serial.print(Nybble2Hex((msgOut.getDataByte(i) >> 4) & 0x0F));
            //Serial.print(Nybble2Hex(msgOut.getDataByte(i) & 0x0F));
            //}
            //Serial.println();
            break;

        case AMDSent:
            // send an initialisation complete message
            msgOut.setCANid(INIT, alias);
            msgOut.setNodeidToData(nodeId);
            cidSent = INITSent;
            ns = initialised;
            break;
        }
        break;
    }

    // inject fake can messages
    /*
    	Serial.println("Preparing to Send?");
    	Serial.print("CANid: "); Serial.println(msgOut.getId(), HEX);
    	Serial.print("datalength: "); Serial.println(msgOut.getDataLength());
    	Serial.print("data: ");
    	for (int i = 0; i < 8; i++){
    		Serial.print(Nybble2Hex((msgOut.getDataByte(i) >> 4) & 0x0F));
    		Serial.print(Nybble2Hex(msgOut.getDataByte(i) & 0x0F));
    		}
    	Serial.println();
    */
    // is there anything to send?
    if (msgOut.getId() != 0) {
        SendMessage();
    }

    // process any incoming messages

    msgIn.setId(0);

    //// send incoming message from WiFi to CAN  (if available)
    if (wifiClient.available()) {
        //#ifdef SHOWMESSAGES
        //mySerial.println("WiFi Message received.");
        //#endif

        //get data from the telnet client
        while (wifiClient.available()) {
            char ch = wifiClient.read();

            //#ifdef SHOWLCD
            //lcd.print(ch); // push it to the UART (=> Serial Monitor)
            //#endif
            //#ifdef SHOWMESSAGES
            //mySerial.print(ch); // push it to the UART (=> Serial Monitor)
            //#endif

            // process incoming characters - when CanAscii2Can returns true the message is complete and can be sent
            if (CanAscii2Can(&WiFiIdentifier, &WiFiMessageType, &WiFiDataLen, WiFiData, &ch)) {
#ifdef SHOWMESSAGES
                Serial.print("Received Message. [");
                Serial.print(WiFiIdentifier, HEX);
                Serial.print("]");
                if (WiFiDataLen > 0) {
                    for (uint8_t i = 0; i < WiFiDataLen; i++) {
                        if (WiFiData[i] < 0x10) {
                            Serial.print(F("0"));
                        }
                        Serial.print(WiFiData[i], HEX);
                    }
                }
                Serial.println("");
#endif
                //msgIn.setId(WiFiIdentifier & !0x10000000);
                msgIn.setId(WiFiIdentifier);
                msgIn.setData(WiFiData, WiFiDataLen);

                // Respond to the message
                if (ns == initialised) {
                    Serial.print("Processing message ");
                    Serial.println(msgIn.getMTI(), HEX);
                    senderAlias = msgIn.getSenderAlias();
                    switch (msgIn.getMTI()) {

                    case SNIIRQ:
                        sendSNIHeader(alias, senderAlias);
                        sendSNIReply(alias, senderAlias, Manufacturer);
                        sendSNIReply(alias, senderAlias, ModelName);
                        sendSNIReply(alias, senderAlias, HardwareVersion);
                        sendSNIReply(alias, senderAlias, SoftwareVersion);
                        sendSNIUserHeader(alias, senderAlias);
                        sendSNIUserReply(alias, senderAlias, 'N');
                        sendSNIUserReply(alias, senderAlias, 'D');
                        break;

                    case  PSI: {
                        uint32_t protflags = SPSP | SNIP | DGP | MCP | CDIP;
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
                        SendMessage();
                        break;
                    }
                    case VNIG:
                        msgOut.setCANid(VNN, alias);
                        msgOut.setNodeidToData(nodeId);
                        SendMessage();
                        break;

                    // datagrams
                    case 0xA123:
                        switch (msgIn.getDataByte(0)) {
                        case 0x20:
                            // send acknowlegment
                            SendDROK(senderAlias, alias, true);
                            if (msgIn.getDataByte(1) == 0x40) {
                                switch (msgIn.getDataByte(6)) {
                                case 0xFB:  // user descriptions
                                    //Serial.print("Sending user data, length ") Serial.println(sizeof(eed.header.userName) + sizeof(eed.header.userDescription));
                                    SendDatagram(senderAlias, alias, &msgIn, eed.header.userName, sizeof(eed.header.userName) + sizeof(eed.header.userDescription));
                                    break;
                                }
                            }

                            if (msgIn.getDataByte(1) == 0x41) {
                                // it's a request for space 0xFD - configuration Data
                                SendDatagram(senderAlias, alias, &msgIn, (const char *) &eed.event[0], sizeof(EEPROM_Data) - sizeof(eed.header));
                            }
                            if (msgIn.getDataByte(1) == 0x43) {
                                // its a cdi request
                                SendDatagram(senderAlias, alias, &msgIn, cdiXml, strlen(cdiXml) + 1);
                                //((uint32_t) msgIn.getDataByte(2) << 24) + ((uint32_t) msgIn.getDataByte(3) << 16) + ((uint32_t) msgIn.getDataByte(4) << 8) + ((uint32_t) msgIn.getDataByte(5)),
                                //(uint8_t) msgIn.getDataByte(6));

                            }
                            break;
                        }
                        break;

                    case 0xB123:
                    case 0xC123:
                        ReceiveDatagram(&msgIn, &datagramBuffer[0], &datagramPtr);
                        break;

                    case 0xD123:
                        ReceiveDatagram(&msgIn, &datagramBuffer[0], &datagramPtr);
                        SendDROK(senderAlias, alias, true);
                        Serial.print("Datagram Buffer: "); for(int i = 0; i < 8; i++) {
							if (datagramBuffer[i] < 0x10) Serial.print("0");
							Serial.print(datagramBuffer[i], HEX); Serial.print(" ");
							 }
							 Serial.println();
                        // is this configuration?
                        switch(datagramBuffer[0]) {
                        case 0x20: {
                            uint32_t errorCode = 0x0000;
                            switch(datagramBuffer[1]) {
                            case 0x01:
                                // write to config space FD (configuration Data)
                                addressOffset = (((((uint32_t)datagramBuffer[2] << 8) + (uint32_t)datagramBuffer[3] << 8) + (uint32_t)datagramBuffer[4] << 8) + (uint32_t)datagramBuffer[5]);
                                if (addressOffset + datagramPtr <= sizeof(EEPROM_Data)- sizeof(eed.header)) {
                                    memcpy(&eed.event[0], &datagramBuffer[6], datagramPtr);
                                    errorCode = 0x0;
                                } else {
                                    errorCode = 0x1080; // out of range
                                }
                                SendWriteReply(senderAlias, alias, &datagramBuffer[0], errorCode);
                                break;
                                
                                //case 0x02:
                                // write to config space FE (configuration Data)
                                //break:
                                //case 0xFB:
                                //if (datagramBuffer[5] == 0x00) {
                                //strcpy(UserName, (const char*)&datagramBuffer[7]);
                                //} else {
                                //strcpy(UserDescription, (const char*)&datagramBuffer[7]);
                                //}
                                //}
                                //SendDROK(alias, senderAlias, false);
                                //break;
                            }
                            break;
                        }
                        }

                    case DROK:
                        if (waitForDatagramACK) waitForDatagramACK = false;
                        break;
                    }
                }
            }
        }
    }
}

bool initialiseESP8266(WiFiServer* server) {
    // Set up the ESP8266
    // set the fixed ip adresses
    WiFi.config(SERVERIP, GATEWAY, SUBNET, DNS);

    // connect to the WLAN router
    WiFi.begin(ssid, password);

    uint8_t i = 0;
    while (WiFi.status() != WL_CONNECTED && i++ < 21) delay(500);
    if (i == 21) {
#ifdef SHOWMESSAGES
        Serial.print("Could not connect to");
        Serial.println(ssid);
#endif
        return false;
    }

    //start the server

    server->begin();
    server->setNoDelay(true);

#ifdef SHOWMESSAGES
    Serial.print("Ready! Use 'telnet ");
    Serial.print(WiFi.localIP());
    Serial.println(" 23' to connect");
    Serial.print("subnetMask: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("gatewayIP: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("dnsIP: ");
    Serial.println(WiFi.dnsIP());
#endif

    return true;
}

bool SendMessage() {
    uint32_t CANIdentifier = 0;
    //  uint8_t CANDataLen = 0;
    //  uint8_t CANData[8]; // = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    char CANAscii[CANASCII_SIZE]; // array to store the ASCII-encoded message

    //Serial.println("Preparing to Send");
    //Serial.print("CANid: "); Serial.println(msgOut.getId(), HEX);
    //Serial.print("datalength: "); Serial.println(msgOut.getDataLength());
    //Serial.print("data: ");
    //for (int i = 0; i < 8; i++){
    //Serial.print(Nybble2Hex((msgOut.getDataByte(i) >> 4) & 0x0F));
    //Serial.print(Nybble2Hex(msgOut.getDataByte(i) & 0x0F));
    //}
    //Serial.println();



    CANIdentifier = msgOut.getId() | 0x10000000;
    //    msgOut.getData(CANData, CANDataLen);
    //    Serial.print("CANIdentifier: "); Serial.println(CANIdentifier, HEX);
    //    Serial.print("CANDataLen: "); Serial.println(CANDataLen);

    if (CANIdentifier != 0) {
        //        Can2CanAscii(&CANIdentifier, &CANDataLen, CANData, CANAscii);
        Can2CanAscii(&CANIdentifier, msgOut.getPDataLength(), msgOut.getPData(), CANAscii);
        Serial.print("Sending: ");
        Serial.println(CANAscii);
        // Transmit the CAN-Ascii message to the wificlient
        wifiClient.print(CANAscii);
        msgOut.setId(0);
    }
    yield();
}

bool SendDROK(uint16_t senderAlias, uint16_t destAlias, bool pending) {
    byte dataBytes[8];

    msgOut.setCANid(DROK, destAlias);
    dataBytes[0] = (byte)(senderAlias >> 8);
    dataBytes[1] = (byte)(senderAlias & 0xFF);
    dataBytes[2] = (!pending) ? 0x0 : 0x80;  // set reply pending if requested
    dataBytes[3] = '\0';
    dataBytes[4] = '\0';
    dataBytes[5] = '\0';
    dataBytes[6] = '\0';
    dataBytes[7] = '\0';
    msgOut.setData(&dataBytes[0], 8);
    SendMessage();
}

bool sendSNIHeader(uint16_t senderAlias, uint16_t destAlias) {
    byte dataBytes[3];

    msgOut.setCANid(SNIIR, senderAlias);
    dataBytes[0] = (byte)(destAlias >> 8) | 0x10;  // first frame
    dataBytes[1] = (byte)(destAlias & 0xFF);
    dataBytes[2] = 0x04;   // version number: sending manufacturer name, node model name, node hardware version and node software version
    msgOut.setData(&dataBytes[0], 3);
    SendMessage();
    return true;
}

bool sendSNIReply(uint16_t senderAlias, uint16_t destAlias, const char infoText[]) {
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
            SendMessage();
            msgOut.setCANid(SNIIR, senderAlias);
            dataLen = 2;
        }
    }
    msgOut.setId(0);
    return fail;
}

bool sendSNIUserHeader(uint16_t senderAlias, uint16_t destAlias) {
    byte dataBytes[3];

    msgOut.setCANid(SNIIR, senderAlias);
    dataBytes[0] = (byte)(destAlias >> 8) | 0x30; // intermediate frame
    dataBytes[1] = (byte)(destAlias & 0xFF);
    dataBytes[2] = 0x02;   // version number: sending user-provided node name, user-provided node description
    msgOut.setData(&dataBytes[0], 3);
    SendMessage();
    return true;
}

bool sendSNIUserReply(uint16_t senderAlias, uint16_t destAlias, const char userValue) {
    byte dataBytes[8];
    char infoText[64];
    uint8_t pos = 0;
    uint8_t dataLen = 2;
    bool fail = false;

    switch (userValue) {
    // User Name
    case 'N':
        //readUserNameFromEEProm(infoText);
        strcpy(infoText, eed.header.userName);
        break;

    // User Description
    case 'D':
        //readUserDescriptionFromEEProm(infoText);
        strcpy(infoText, eed.header.userDescription);
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
            SendMessage();
            msgOut.setCANid(SNIIR, senderAlias);
        }
        dataLen = 2;
    }
    msgOut.setId(0);
    return fail;
}

void SendDatagram(const uint16_t destAlias, uint16_t senderAlias, OpenLCBMessage * msg, const char * data, uint16_t dataLength) {
    uint8_t lenPos = (msg->getDataByte(1) == 0x40)? 7:6;
    uint32_t address = ((uint32_t) msg->getDataByte(2) << 24) + ((uint32_t) msg->getDataByte(3) << 16) + ((uint32_t) msg->getDataByte(4) << 8) + ((uint32_t) msg->getDataByte(5));
    uint8_t length = (uint8_t) msg->getDataByte(lenPos);

    // copy the request bytes
    //msgOut.setCANid(((length <= dataLength + 1 - 6)?0xB000:0xA000) + destAlias, senderAlias);
    msgOut.setCANid(0xB000 + destAlias, senderAlias);
    //    buf[0] = 0x20;
    //    buf[1] = 0x53; // address space FF
    //    buf[2] = (byte)(address >> 24);
    //    buf[3] = (byte)((address >> 16) & 0xFF);
    //    buf[4] = (byte)((address >> 8) & 0xFF);
    //    buf[5] = (byte)(address & 0xFF);
    msgOut.setData(msg->getPData(), msg->getDataLength() - 1);  // retrieve as many data bytes as we were sent
    * (msgOut.getPData() + 1) = msg->getDataByte(1) + 0x10;

    SendMessage();

    if (address + length > dataLength + 1) {
        length = dataLength + 1 - address; // include the terminating null
    }

    for (int i = 0; i < length; i+=8) {
        msgOut.setCANid((i < (length - 8)? 0xC000 : 0xD000) + destAlias, senderAlias);
        msgOut.setData((byte*)(&data[address + i]), (length - i) >= 8 ? 8 : length - i );

        SendMessage();
    }
    waitForDatagramACK = true;
}

void SendWriteReply(const uint16_t destAlias, uint16_t senderAlias, byte * buf, uint16_t errorCode) {
    uint8_t dataPos = (*(buf+1) == 0x10)? 7:6;
    msgOut.setCANid(0xA000 + destAlias, senderAlias);
    msgOut.setData(buf, dataPos);  // retrieve as many data bytes as we were sent
    if (errorCode != 0) {
        * (msgOut.getPData() + 1) = *buf+1 + 0x08;
        * (msgOut.getPData() + dataPos) = (byte)errorCode >> 8;
        * (msgOut.getPData() + dataPos + 1) = (byte)errorCode & 0xFF;
        msgOut.setDataLength(dataPos + 1);
    }

    SendMessage();

}

void ReceiveDatagram(OpenLCBMessage* m, byte* buffer, uint8_t * ptr) {
    uint8_t dataLength = 0;

    Serial.print("ptr: "); Serial.println(*ptr);

    if (m->getMTI() == 0xB123) {
        memset(buffer, '\0', 72);
        *ptr = 0;
    }

    m->getData(buffer + *ptr, &dataLength);
    *ptr += dataLength;
}
