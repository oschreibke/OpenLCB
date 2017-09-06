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

// documentation
#include "Description.h"


// struct defining the user area of the EEPROM storage ( = Segment 253),
// except for the serial which is the 16-bit node identifier to be appended to the
// assigned OpenLCB number range. In my case 05 01 01 01 31.
struct OpenLCBEEPROMHeader {
    uint16_t  serial;
    char      userName[63];
    char      userDescription[64];
};

// my chosen event handlers (EVENTTYPE is a misnomer)
enum EVENTTYPE:uint8_t {I2COutput, I2CInput, pinOutput, pinInput};


// Struct to hold the configuration for an event.
// Beware C++ padding which will silently add bytes. The padding causes the whole data structure to be larger than 1024 bytes
// which is all the Arduino Nano has.

// The eventtypes allow configuration of an Arduino pin (D1 - D10). This is an arbitrary choice of the free pins on my Nano.
// Alternatively allows configuration of an I2C device (= ATTiny85).
// For example: setting a slowmo point (switch, turnout, weiche, whatever) to a set angle.

struct OpenLCBEvent {
    uint64_t eventId;
    char     eventName[37];
    EVENTTYPE eventType;
    uint8_t  address;
    uint8_t  eventValue;
};


// build the data in RAM
struct EEPROM_Data {
    OpenLCBEEPROMHeader header;
    OpenLCBEvent event[20];
};

// The node id. For simplicity I'm using alias 123 without any negotiation.
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

// define SHOWMESSAGES to write to the Serial object. The output can be viewed using the Arduino Serial Monitor;
#define SHOWMESSAGES

//#ifdef SHOWMESSAGES
//#include <SoftwareSerial.h>

//SoftwareSerial mySerial(5, 4); // RX, TX
//#endif


// receive buffer for the can packets
// WiFi => received over WiFi
// Note CAN handling is fake. A physical CAN interface is not necessary.


CAN_message_type  WiFiMessageType = Standard;
uint32_t WiFiIdentifier = 0;
uint8_t WiFiDataLen = 0;
uint8_t WiFiData[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

char msgString[128];                        // Array to store serial string

// some status variables, probably left over from the code I hacked and not used
bool wifiInitialised = false;
enum NodeStatus {uninitialised, initialised};
enum CIDSent {noneSent, CID1Sent, CID2Sent, CID3Sent, CID4Sent, RIDSent, AMDSent, INITSent};

NodeStatus ns = uninitialised;
CIDSent cidSent = noneSent;

// More status variables, possibly not all used
bool sendCDI = false;
//uint8_t sendCDIStep = 0;
bool waitForDatagramACK = false;
uint32_t addressOffset = 0;

// The incoming and outgoing can messages
OpenLCBMessage msgIn;
OpenLCBMessage msgOut;

// The sender alias ( = JMRI)
uint16_t senderAlias = 0;

// storage for an incoming datagram.
// There is no code to prevent collisions when two datagrams arrive concurrently.
// in this model that was not an issue. (bless you wol).
byte datagramBuffer[72];
uint8_t datagramPtr = 0;

// allocate the fake EEPROM area
EEPROM_Data eed;



void setup() {
    // start the connection for the Arduino serial monitor
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

    // Allocate the cdi data area (surprised this stays in scope)
    OpenLCBCDI cdi;

    //    eed.event[0].eventId = 0x0501010101310000ULL;
    //    hexDump("eventId", &eed.event[0].eventId, 8);
    //    print64BitHex(eed.event[0].eventId); Serial.println();

    // ... and initialise it all to 0x00
    memset(&eed, 0x00, sizeof(EEPROM_Data));

    // set up the serial and user area
    eed.header.serial = 0xFFFF;
    strcpy((char *)&eed.header.userName, "my first Node");
    strcpy((char *)&eed.header.userDescription, "first node for cdi");

    // small sanity check
    cdi.ShowItemLengths();

    // not needed. the CDI is static.
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

    // (un) initialise the output id. 0=>no valid message to send
    msgOut.setId(0);

    // Query the node status
    switch (ns) {
    case uninitialised:
        // abbreviated setup
        // (not sure if this is strictly necessary - JMRI will send Verified Node queries on opening the configuration panel)

        // state machine for the initialisation process.
        switch(cidSent) {
        case noneSent:
            // send a RID (Reserve ID) message
            msgOut.setCANid(RID, alias);
            msgOut.setDataLength(0);

            //Serial.print(F("RID: "));
            //Serial.println(*msgOut.getPId(), HEX);
            cidSent = RIDSent;
            break;

        case RIDSent:

            // transition to permitted
            // Send a AMD (Alias Map Definition) message
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

    // is there anything to send?
    if (msgOut.getId() != 0) {
        SendMessage();
    }

    // process any incoming messages

    msgIn.setId(0);

    //// send incoming message from WiFi to CAN  (if available)
    // in this case, simply populate the data structure
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

            // process incoming characters - when CanAscii2Can returns true the message is complete and can be processed
            // Note: this code assumes it is the only node on the network. Which it is as it is also the gateway.
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
                    senderAlias = msgIn.getSenderAlias();  // get JMRI's alias

                    // decode the message type identifier (MTI)
                    switch (msgIn.getMTI()) {

                    // A Simple Node Ident Info Request
                    case SNIIRQ:
                        // respond with the Simple Node Ident Information
                        sendSNIHeader(alias, senderAlias);
                        sendSNIReply(alias, senderAlias, Manufacturer);
                        sendSNIReply(alias, senderAlias, ModelName);
                        sendSNIReply(alias, senderAlias, HardwareVersion);
                        sendSNIReply(alias, senderAlias, SoftwareVersion);
                        sendSNIUserHeader(alias, senderAlias);
                        sendSNIUserReply(alias, senderAlias, 'N');  // send the user name for the node
                        sendSNIUserReply(alias, senderAlias, 'D');  // send the user definition for the node
                        break;

                    // A protocol support reply inquiry.
                    case  PSI: {
                        // we can do:
                        //  SPSP - Simple Protocol subset (whatever that is - the standards are vague)
                        //  SNIP - Simple Node Information Protocol
                        //  DP   - Datagram Protocol
                        //  MCP  - Memory Configuration Protocol
                        //  CDIP - Configuration Description Information

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

                    // A Verify Node ID Number Global request
                    case VNIG:
                        // respond with our full node id
                        msgOut.setCANid(VNN, alias);
                        msgOut.setNodeidToData(nodeId);
                        SendMessage();
                        break;

                    // datagrams

                    // Note: the following case statements are specific to this model.
                    // profuction code will need to handle the alias part differently.

                    case 0xA123:  // a single frame datagran addressed to me
                        ReceiveDatagram(&msgIn, &datagramBuffer[0], &datagramPtr);
                        ProcessDatagram(senderAlias, alias, &datagramBuffer[0], datagramPtr);
                        break;

                    case 0xB123:  // the first of a multiframe datagram addressed to me
                    case 0xC123:  // one of the middle frames, addressed to me
                        ReceiveDatagram(&msgIn, &datagramBuffer[0], &datagramPtr);
                        break;

                    case 0xD123:  // the final multiframe datagram addressed to me
                        ReceiveDatagram(&msgIn, &datagramBuffer[0], &datagramPtr);
                        // as this is the last frame, we can process the whole datagram of up to 72 characters
                        ProcessDatagram(senderAlias, alias, &datagramBuffer[0], datagramPtr);
                        break;

                    case DROK:
                        // process a Daragram received OK message from JMRI (No longer used?)
                        if (waitForDatagramACK) waitForDatagramACK = false;
                        break;

                    // Message [11111123] from the JMRI send Frame Tool
                    // hex dump the EEPROM data area
                    case 0x1111:
                        DumpEEPROM();
                        break;

                    // Message [12222123] from the JMRI send Frame Tool
                    // Do a formatted dump of the EEPROM data area to make sure the data has been correctly written
                    // to the expected location.
                    case 0x2222:
                        DumpEEPROMFormatted();
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
    // Convert MsgOut to CANASCII format and send it to the WiFi client (JMRI)

    uint32_t CANIdentifier = 0;
    //  uint8_t CANDataLen = 0;
    //  uint8_t CANData[8]; // = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    char CANAscii[CANASCII_SIZE]; // array to store the ASCII-encoded message

    CANIdentifier = msgOut.getId() | 0x10000000;
    //    msgOut.getData(CANData, CANDataLen);
    //    Serial.print("CANIdentifier: "); Serial.println(CANIdentifier, HEX);
    //    Serial.print("CANDataLen: "); Serial.println(CANDataLen);

    // Only send if there's data
    if (CANIdentifier != 0) {
        //        Can2CanAscii(&CANIdentifier, &CANDataLen, CANData, CANAscii);
        Can2CanAscii(&CANIdentifier, msgOut.getPDataLength(), msgOut.getPData(), CANAscii);
        Serial.print("Sending: ");
        Serial.println(CANAscii);
        // Transmit the CAN-Ascii message to the WiFiclient
        wifiClient.print(CANAscii);
        msgOut.setId(0);
    }
    yield();
}


bool SendDROK(uint16_t senderAlias, uint16_t destAlias, bool pending) {
    // Send a Datagram Received OK message with the Reply Pending bit set, if requested.
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
    // Send the SNI header for the fixed data elements.
    // for simplicity, this is not a full frame, only three bytes are sent
    // to be followed by the actual string in subsequent frames
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
    // Send the SNI fixed data elements, one per call. Note: they will usually be stored in PROGGMEM
    // used to transmit manufacturer name, node model name, node hardware version and node software version
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

    // break the text into 8-byte chunks due to the length limit of a CAN frame
    while(pos < strlen(infoText) +1) {

        //Serial.print(F("Character at ")); Serial.print(pos); Serial.print(F(": ")); Serial.print(infoText[pos]); Serial.print(F(" dataBytes[")); Serial.print(dataLen);	Serial.print(F("]: "));
        dataBytes[dataLen++] = infoText[pos++];
        //Serial.println((char)dataBytes[dataLen - 1]);

        dataBytes[0] |= 0x30;  // intermediate frame

        if (dataLen > 7 || pos > strlen(infoText)) {
            msgOut.setData(&dataBytes[0], dataLen);
            delay(50);   // add delay otherwise messages are not sent in sequence (Not necessary here: this is a problem with the CAN library)
            SendMessage();
            msgOut.setCANid(SNIIR, senderAlias);
            dataLen = 2;
        }
    }
    msgOut.setId(0);
    return fail;
}

bool sendSNIUserHeader(uint16_t senderAlias, uint16_t destAlias) {
    // Send the SNI header for the user data elements
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
    // Send the SNI fixed data elements, one per call. Note: they will usually be stored in EEPROM.
    // used to transmit the User Name and User Description.
    byte dataBytes[8];
    char infoText[64];
    uint8_t pos = 0;
    uint8_t dataLen = 2;
    bool fail = false;

    // marshal the requested data into a temporary buffer
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

    // break the text into 8-byte chunks due to the length limit of a CAN frame
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
            delay(50);   // add delay otherwise messages are not sent in sequence (Not necessary here: this is a problem with the CAN library)
            SendMessage();
            msgOut.setCANid(SNIIR, senderAlias);
        }
        dataLen = 2;
    }
    msgOut.setId(0);
    return fail;
}

void SendDatagram(const uint16_t destAlias, uint16_t senderAlias, OpenLCBMessage * msg, const char * data, uint16_t dataLength) {
    // send a datagram containing a reply to a request for data
    // Note: depending on the segment production code will need to retrieve from the appropriate storage: PROGMEM or EEPROM

    // fiddle with the location of the Segment specifier - JMRI is not consistent. See  S-9.7.4.2 for details
    uint8_t lenPos = (msg->getDataByte(1) == 0x40)? 7:6;
    // determine the offset within the segment of the data to read/write
    uint32_t address = ((uint32_t) msg->getDataByte(2) << 24) + ((uint32_t) msg->getDataByte(3) << 16) + ((uint32_t) msg->getDataByte(4) << 8) + ((uint32_t) msg->getDataByte(5));
    // how much data was requested? Normally a read will request 64 (0x40) bytes, a write will send the length of the data element (e.g. 8 bytes for an event id).
    uint8_t length = (uint8_t) msg->getDataByte(lenPos);

    // copy the request bytes
    //msgOut.setCANid(((length <= dataLength + 1 - 6)?0xB000:0xA000) + destAlias, senderAlias);

    // send a response datagram. The data content is the first 6/7 (depending on byte 1 of the request (S-9.7.4.2))
    msgOut.setCANid(0xB000 + destAlias, senderAlias);
    //    buf[0] = 0x20;
    //    buf[1] = 0x53; // address space FF
    //    buf[2] = (byte)(address >> 24);
    //    buf[3] = (byte)((address >> 16) & 0xFF);
    //    buf[4] = (byte)((address >> 8) & 0xFF);
    //    buf[5] = (byte)(address & 0xFF);
    msgOut.setData(msg->getPData(), msg->getDataLength() - 1);  // retrieve as many data bytes as we were sent
    // pointer arithmetic, add 0x10 to byte 1 of the data area
    * (msgOut.getPData() + 1) = msg->getDataByte(1) + 0x10;

    // send the reply header
    SendMessage();

    // calculate how much data to send
    if (address + length > dataLength + 1) {
        length = dataLength + 1 - address; // include the terminating null
    }

    // send the data in 8-byte chunks, identify the last frame as the final frame in the datagram.
    for (int i = 0; i < length; i+=8) {
        msgOut.setCANid((i < (length - 8)? 0xC000 : 0xD000) + destAlias, senderAlias);
        msgOut.setData((byte*)(&data[address + i]), (length - i) >= 8 ? 8 : length - i );

        SendMessage();
    }
    waitForDatagramACK = true;
}

void SendWriteReply(const uint16_t destAlias, uint16_t senderAlias, byte * buf, uint16_t errorCode) {
    // send a reply to a write request. This will fit in a singe frame, so use MTI OxAxxx.

    // again, fiddle with the request format to determine the length of the data to send
    uint8_t dataPos = (*(buf+1) == 0x00)? 7:6;
    //hexDump("SendWriteReply", buf, dataPos);
    msgOut.setCANid(0xA000 + destAlias, senderAlias);
    msgOut.setData(buf, dataPos);  // retrieve as many data bytes as we were sent
    //hexDump("SendWriteReply - set data buf", msgOut.getPData(), dataPos);
    //Serial.print("*buf+1 "); Serial.println(*(buf+1));

    // pointer Arithmetic: add 0x10 to byte 1 of the data area.
    * (msgOut.getPData() + 1) = (*(buf+1)) + 0x10;
    //Serial.print("msgOut.getPData() + 1 "); Serial.println(* (msgOut.getPData() + 1));

    // send an error response if the error code was set.
    // again pointer arithmetic to put things in the correct place. Could be improved to increase transparency.
    if (errorCode != 0) {
        * (msgOut.getPData() + 1) = (*buf+1) + 0x08;
        * (msgOut.getPData() + dataPos) = (byte)errorCode >> 8;
        * (msgOut.getPData() + dataPos + 1) = (byte)errorCode & 0xFF;
        msgOut.setDataLength(dataPos + 1);
    }

    //hexDump("SendWriteReply - message", msgOut.getPData(), dataPos);
    SendMessage();

}

void ReceiveDatagram(OpenLCBMessage* m, byte* buffer, uint8_t * ptr) {
    // receive a datagram and build the datagram buffer
    uint8_t dataLength = 0;

    //    Serial.print("ptr: ");
    //    Serial.println(*ptr);

    // 0xAxxx and 0xBxxx indicate the first (only) frame in the datagram, clear the buffer and reset the pointer before filling.
    if (m->getMTI() == 0xA123 || m->getMTI() == 0xB123) {
        memset(buffer, '\0', 72);
        *ptr = 0;
    }

    // fill the received data into the buffer at the appropriate offset, then adjust the pointer
    m->getData(buffer + *ptr, &dataLength);
    *ptr += dataLength;
}

void ProcessDatagram(uint16_t senderAlias, uint16_t alias,  byte* datagram, uint8_t datagramLength) {
    // process the datagram we just received

    uint32_t errorCode = 0x0000;  // no error (so far)
    byte addressSpace = 0x00;     // undefined Segment

    //Serial.print("Datagram Buffer: ");
    //for(int i = 0; i < 8; i++) {
    //if (datagram[i] < 0x10) Serial.print("0");
    //Serial.print(datagram[i], HEX);
    //Serial.print(" ");
    //}
    //Serial.println();

    // configuration datagrams (read and write) have 0x20 in the first byte (Byte[0])
    switch (*datagram) {
    case 0x20:  // is this configuration?
        // send acknowlegment
        SendDROK(senderAlias, alias, true);

        // determine the Segment - no error handling for an incorrect specification
        switch (*(datagram + 1)) {
        case 0x00:
        case 0x40:
            addressSpace = *(datagram + 6);
            break;

        case 0x01:
        case 0x41:
            addressSpace = 0xFD;     // Configuration definition
            break;

        case 0x42:
            addressSpace = 0xFE;     // All memory
            break;

        case 0x43:
            addressSpace = 0xFF;     // Configuration definition
            break;
        }

        // read requests
        switch (*(datagram + 1)) {
        case 0x40:
        case 0x41:
        case 0x42:
        case 0x43:
            switch(addressSpace) {
            // configuration requests for data

            case 0xFB:  // user descriptions
                //Serial.print("Sending user data, length ") Serial.println(sizeof(eed.header.userName) + sizeof(eed.header.userDescription));
                SendDatagram(senderAlias, alias, &msgIn, eed.header.userName, sizeof(eed.header.userName) + sizeof(eed.header.userDescription));
                break;

            case 0xFD:
                // it's a request for space 0xFD - configuration Data
                SendDatagram(senderAlias, alias, &msgIn, (const char *) &eed.event[0], sizeof(EEPROM_Data) - sizeof(eed.header));
                break;

            case 0xFF:
                // its a cdi request
                SendDatagram(senderAlias, alias, &msgIn, cdiXml, strlen(cdiXml) + 1);
                break;
            }
            break;

        // write requests - again, no error handling if an incorrect Segment is requested.
        case 0x00:
        case 0x01:
            // write configuration
            switch(addressSpace) {
            case 0xFB:
                // write to Segment 0xFB (251) - User Descriptions
                hexDump("Datagram", datagram, datagramLength);
                // write to config space FB (user Data)

                // dertrmine the address offset
                addressOffset = (((((((uint32_t)datagram[2] << 8) + (uint32_t)datagram[3]) << 8) + (uint32_t)datagram[4]) << 8) + (uint32_t)datagram[5]);
                Serial.print("addressOffset: ");
                Serial.println(addressOffset);
                // make sure we haven't gone outside the data area.
                // If OK, write the data
                // otherwise, create an error response.
                if (addressOffset + datagramLength - 7 <= sizeof(eed.header)) {
                    memcpy(((byte*)&eed.header.userName) + addressOffset, &datagram[7], datagramLength - 7);
                    errorCode = 0x0;
                } else {
                    errorCode = 0x1080; // out of range
                }

                // confirm the action
                SendWriteReply(senderAlias, alias, &datagram[0], errorCode);
                break;

            case 0xFD:
                // write to Segment 0xFD - Configuration data
                hexDump("Datagram", datagram, datagramLength);
                // write to config space FD (configuration Data)
                addressOffset = (((((((uint32_t)datagram[2] << 8) + (uint32_t)datagram[3]) << 8) + (uint32_t)datagram[4]) << 8) + (uint32_t)datagram[5]);
                Serial.print("addressOffset: ");
                Serial.println(addressOffset);
                // make sure we haven't gone outside the data area.
                // If OK, write the data
                // otherwise, create an error response.
                if (addressOffset + datagramLength <= sizeof(EEPROM_Data)- sizeof(eed.header)) {
                    memcpy(((byte*)&eed.event[0]) + addressOffset, &datagram[6], datagramLength - 6);
                    errorCode = 0x0;
                } else {
                    errorCode = 0x1080; // out of range
                }

                // confirm the action
                SendWriteReply(senderAlias, alias, &datagram[0], errorCode);
                break;
            }
            break;
        }
    }
}


void DumpEEPROM() {
    // dump the EEPROM in hex format
    hexDump("EEPROM Data", &eed, sizeof(EEPROM_Data));
    //for (int i = 0; i < sizeof(EEPROM_Data); i+= 32){
    //Serial.print(i); Serial.print(" ");
    //for (int j = 0; j < 32; j++){
    //Serial.print(Nybble2Hex((((char)eed)[(i*32)+j]) >> 4));	Serial.print(Nybble2Hex((((char)eed)[(i*32)+j]) & 0x0F));
    //}
    //Serial.print(" ");
    //for (int j = 0; j < 32; j++){
    //if (((byte)eed)[(i*32)+j] >= 32 && ((byte)eed)[(i*32)+j] <= 126){
    //Serial.print(((char)eed)[(i*32)+j]);
    //}else{
    //Serial.print(".");
    //}
    //}
    //Serial.println();
    //}
}


// Dump a data area in hex format helper (stolen from somewhere on StackOverflow)
void p(const char *fmt, ... ) {
    char buf[128]; // resulting string limited to 128 chars
    va_list args;
    va_start (args, fmt );
    vsnprintf(buf, 128, fmt, args);
    va_end (args);
    Serial.print(buf);
}

// Dump a data area in hex format (stolen from somewhere on StackOverflow)
void hexDump (const char *desc, void *addr, int len) {
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    // Output description if given.
    if (desc != NULL)
        p ("%s:\n", desc);

    if (len == 0) {
        p("  ZERO LENGTH\n");
        return;
    }
    if (len < 0) {
        p("  NEGATIVE LENGTH: %i\n",len);
        return;
    }

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                p ("  %s\n", buff);

            // Output the offset.
            p ("  %04x ", i);
        }

        // Now the hex code for the specific character.
        p (" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        p ("   ");
        i++;
    }

    // And print the final ASCII bit.
    p ("  %s\n", buff);
}


void DumpEEPROMFormatted() {
    // do a formatted dump of the Configuration data to make sure everything
    // was written where we expected.
    Serial.print("serial: ");
    Serial.println(eed.header.serial, HEX);
    Serial.print("userName: ");
    Serial.println(eed.header.userName);
    Serial.print("userDescription: ");
    Serial.println(eed.header.userDescription);
    for(int i = 0; i < 20; i++) {
        if (eed.event[i].eventId != 0) {
            Serial.print("event: ");
            Serial.println(i);
            Serial.print("eventId: ");
            print64BitHex(ReverseEndianness(&eed.event[i].eventId));
            Serial.println();
            Serial.print("eventName: ");
            Serial.println(eed.event[i].eventName);
            Serial.print("eventType: ");
            Serial.println(eed.event[i].eventType);
            Serial.print("I2C address / pin: ");
            Serial.println(eed.event[i].address);
            Serial.print("command value: ");
            Serial.println(eed.event[i].eventValue);
        }
    }
}

uint64_t ReverseEndianness(uint64_t *val) {
    // The ESP8266 is little-endian, OpenLCB is big-endian
    // adjust so the event id is rendered correctly
    // There are probably better ways to handle the event-id, for example as an 8-byte value *ToDo*
    uint64_t rev = 0;

    //hexDump("ReverseEndianness", val, 8);
    for (int8_t i = 0; i < 8; i++) {
        rev += (uint64_t)(*(((byte*)val) + i)) << ((7 - i) * 8);
    }
    return rev;
}
