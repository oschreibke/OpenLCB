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
uint8_t sendCDIStep = 0;
bool waitForDatagramACK = false;
uint32_t addressOffset = 0;

OpenLCBMessage msgIn;
OpenLCBMessage msgOut;

uint16_t senderAlias = 0;

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

    cdi.ShowItemLengths();
    cdi.AssembleXML();


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

                    case 0xA123:
                        switch (msgIn.getDataByte(0)) {
                        case 0x20:
                            // send acknowlegment
                            msgOut.setCANid(DROK, alias);
                            dataBytes[0] = (byte)(senderAlias >> 8);
                            dataBytes[1] = (byte)(senderAlias & 0xFF);
                            dataBytes[2] = 0x80;  // set reply pending
                            dataBytes[3] = '\0';
                            dataBytes[4] = '\0';
                            dataBytes[5] = '\0';
                            dataBytes[6] = '\0';
                            dataBytes[7] = '\0';
                            msgOut.setData(&dataBytes[0], 8);
                            SendMessage();
                            // send datagrams (wait for ack for each)

                            if (msgIn.getDataByte(1) == 0x43) {
                                // its a cdi request
                                sendCDI = true;
                                sendCDIStep = 0;
                                waitForDatagramACK = false;
                            }
                        }
                        break;

                    case DROK:
                        if (waitForDatagramACK) waitForDatagramACK = false;
                        break;
                    }
                }
                if (sendCDI && ! waitForDatagramACK) {
                    switch (sendCDIStep) {
                    case 0:
                        addressOffset = 0;
                        SendCDIDatagram(cdiXmlHeader, senderAlias, alias, &addressOffset);
                        break;

                    case 1:

                        SendCDIDatagram(cdiStart, senderAlias, alias, &addressOffset);
                        break;

                    case 2:
                        SendCDIDatagram(cdiStart2, senderAlias, alias, &addressOffset);
                        break;

                    case 3:
                        SendCDIDatagram(cdiStart3, senderAlias, alias, &addressOffset);
                        break;

                    case 4:
                        SendCDIDatagram(cdiTagIdentification1, senderAlias, alias, &addressOffset);
                        break;

                    case 5:
                        SendCDIDatagram(Manufacturer, senderAlias, alias, &addressOffset);
                        break;

                    case 6:
                        SendCDIDatagram(cdiTagIdentification2, senderAlias, alias, &addressOffset);
                        break;

                    case 7:
                        SendCDIDatagram(ModelName, senderAlias, alias, &addressOffset);
                        break;

                    case 8:
                        SendCDIDatagram(cdiTagIdentification3, senderAlias, alias, &addressOffset);
                        break;

                    case 9:
                        SendCDIDatagram(HardwareVersion, senderAlias, alias, &addressOffset);
                        break;

                    case 10:
                        SendCDIDatagram(cdiTagIdentification4, senderAlias, alias, &addressOffset);
                        break;

                    case 11:
                        SendCDIDatagram(cdiAdci, senderAlias, alias, &addressOffset);
                        break;

                    case 12:
                        //space 251 (0xFB) = User Info
                        SendCDIDatagram(cdiUserInfo1, senderAlias, alias, &addressOffset);
                        break;

                    case 13:
                        SendCDIDatagram(cdiUserInfo2, senderAlias, alias, &addressOffset);
                        break;

                    case 14:
                        SendCDIDatagram(cdiUserInfo3, senderAlias, alias, &addressOffset);
                        break;

                    case 15:
                        SendCDIDatagram(cdiUserInfo4, senderAlias, alias, &addressOffset);
                        break;

                    case 16:
                        SendCDIDatagram(cdiUserInfo5, senderAlias, alias, &addressOffset);
                        break;

                    case 17:
                        SendCDIDatagram(cdiUserInfo6, senderAlias, alias, &addressOffset);
                        break;

                    case 18:
                        // space 253 (0xFD) = configuration
                        SendCDIDatagram(cdiSegment253, senderAlias, alias, &addressOffset);
                        break;

                    case 19:
                        SendCDIDatagram(cdiConfiguration, senderAlias, alias, &addressOffset);
                        break;

                    case 20:
                        SendCDIDatagram(cdiSegmentEnd, senderAlias, alias, &addressOffset);
                        break;

                    case 21:
                        SendCDIDatagram(cdiEnd, senderAlias, alias, &addressOffset);
                        break;

                    case 22:
                        // send the terminating null
                        msgOut.setCANid(0xB000 + senderAlias, alias);
                        msgOut.setData((byte*)"\x20\x53\0\0\0\0\0", 7);
                        dataBytes[0] = 0x20;
                        dataBytes[1] = 0x53;
                        dataBytes[2] = (byte)(addressOffset >> 24);
                        dataBytes[3] = (byte)((addressOffset >> 16) & 0xFF);
                        dataBytes[4] = (byte)((addressOffset >> 8) & 0xFF);
                        dataBytes[5] = (byte)(addressOffset & 0xFF);
                        msgOut.setData(&dataBytes[0], 7);
                        SendMessage();

                        sendCDI = false; // we're done

                        waitForDatagramACK = true;

                        //SendCDIDatagram(SoftwareVersion[] = "0.1";

                        //SendCDIDatagram(UserName[] = "my first Node";
                        //SendCDIDatagram(UserDescription[] = "first node for cdi";
                        break;
                    }
                    sendCDIStep++;
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
        strcpy(infoText, UserName);
        break;

    // User Description
    case 'D':
        //readUserDescriptionFromEEProm(infoText);
        strcpy(infoText, UserDescription);
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

void SendCDIDatagram(const char* text, uint16_t destAlias, uint16_t senderAlias, uint32_t * address) {
    byte buf[6];
    
    msgOut.setCANid(0xB000 + destAlias, senderAlias);
    buf[0] = 0x20;
    buf[1] = 0x53; // address space FF
    buf[2] = (byte)(*address >> 24);
    buf[3] = (byte)((*address >> 16) & 0xFF);
    buf[4] = (byte)((*address >> 8) & 0xFF);
    buf[5] = (byte)(*address & 0xFF);
    msgOut.setData(&buf[0], 6);
    SendMessage();
    for (int i = 0; i < strlen(text); i+=8) {
        msgOut.setCANid((i < (strlen(text) - 8)? 0xC000 : 0xD000) + destAlias, senderAlias);
        msgOut.setData((byte*)(text+i), (strlen(text) - i) >= 8 ? 8 : strlen(text) - i );
        SendMessage();
        if (i> 64) break;
    }
    waitForDatagramACK = true;
    *address += strlen(text);
}
