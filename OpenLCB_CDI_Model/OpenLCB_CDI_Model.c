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

#include <stdlib.h>
#include <stdarg.h>
#include "string.h"
#include "espressif/esp_common.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "esp8266.h"
#include "esp/uart.h"
#include "lwip/sockets.h"

#include "CANCommon.h"
#include "OpenLCBMessage.h"
#include "OpenLCBCDI.h"
#include "Util.h"
#include "tcpserver.h"
#include "OpenLCB_CDI_Model.h"
// documentation
#include "Description.h"

extern int32_t client_sock;
extern struct queueHandles qh;

// struct defining the user area of the EEPROM storage ( = Segment 253),
// except for the serial which is the 16-bit node identifier to be appended to the
// assigned OpenLCB number range. In my case 05 01 01 01 31.
struct OpenLCBEEPROMHeader {
    uint16_t  serial;
    char      userName[63];
    char      userDescription[64];
};

// my chosen event handlers (EVENTTYPE is a misnomer)
enum EVENTTYPE {I2COutput, I2CInput, pinOutput, pinInput};


// Struct to hold the configuration for an event.
// Beware C++ padding which will silently add bytes. The padding causes the whole data structure to be larger than 1024 bytes
// which is all the Arduino Nano has.

// The eventtypes allow configuration of an Arduino pin (D1 - D10). This is an arbitrary choice of the free pins on my Nano.
// Alternatively allows configuration of an I2C device (= ATTiny85).
// For example: setting a slowmo point (switch, turnout, weiche, whatever) to a set angle.

struct OpenLCBEvent {
    uint64_t       eventId;
    enum EVENTTYPE eventType;
    uint8_t        address;
    uint8_t        eventValue;
    char           eventName[42];
};


// build the data in RAM
struct EEPROM_Data {
    struct OpenLCBEEPROMHeader header;
    struct OpenLCBEvent event[20];
};

// The node id. For simplicity I'm using alias 123 without any negotiation.
const uint64_t nodeId = 0x0501010131FFULL;
const uint16_t alias = 0x123;

// the fixed information about the node. Usually in PROGMEM.
// This is for SNI, repeated below in the CDI. There is potential for reuse here.
const char Manufacturer[] = "O Schreibke";
const char ModelName[] = "Test Node";
const char HardwareVersion[] = "0.1";
const char SoftwareVersion[] = "0.1";

// The Configuration Description Information. Again in PROGMEM.
// Describes the elements which make up both the configuration information itself and how it is presented in the JMRI configuration panel.
// Does NOT contain any variable data (config values, etc).
// The easiest way to visualise is to use the model and call up the configuration panel in JMRI.
// See also S-9.7.4.1

// According to S-9.7.4.1, the identification section is optional. I didn't try to see if JMRI would use the SNI information to populate this information in the panel.

// Describes Segments 0xFB and 0FD
// Segment 0xFB contains two elements: the user name and user description.
// Segment 0xFD contains a repeating group of 20 entries, which is about all I can fit in the 1K EEPROM - See the notes about padding.
//              Each event entry contains five elements:
//                    - the event id associated with the entry. I don't think it has to be unique, but didn't try it.
//                      Repeating an event identifier would for example enable raising and lowering a barrier and also setting the signal protecting it. 
//                    - A user description for the event.
//                    - The handler type: I've defined both a physical Arduino pin and an I2C device.
//                    - The pin number or I2C address
//                    - The value to be sent 0/1 for pins or the command to send to an i2C device (for example setting a servo to a specific angle)
//                      In case if an input pin or device, receipt of the value would cause the corresponding event id to be generated

const  char cdiXml[] = "<?xml version=\"1.0\"?>\n"
                      "<cdi xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"http://openlcb.org/schema/cdi/1/1/cdi.xsd\">\n"
						  "<identification>"
							  "<manufacturer>O Schreibke</manufacturer>"
							  "<model>Test Node</model>"
							  "<hardwareVersion>0.1</hardwareVersion>"
							  "<softwareVersion>0.1</softwareVersion>"
						  "</identification>\n"
						  "<acdi/>\n"
						  //space 251 (0xFB) = User Info Description
						  "<segment space=\"251\">"
						      "<name>User Identification</name>"
							  "<description>Lets the user add his own description</description>"
							  "<string size=\"63\"><name>Node Name</name></string>" 
							  "<string size=\"64\"><name>Node Description</name></string>"
						  "</segment>\n" 
						  // space 253 (0xFD) = configuration  
						  "<segment space=\"253\">"
							  "<group replication=\"20\">"
								  "<name>Events</name>"
								  "<description>Each tab is one entry in the event table.</description>"
								  "<repname>Event</repname>"
								  "<group>"
								      "<name>Event</name>"
									  "<eventid><name>I2C or pin command</name>"
										  "<description>When this event arrives, command will be sent to the selected i2c device or pin.</description>"
									  "</eventid>"
									  "<int size=\"4\"><name>Decoder type</name><default>1</default>"
										  "<map>"
											  "<relation><property>1</property><value>I2C output</value></relation>"
											  "<relation><property>2</property><value>I2C input</value></relation>"
											  "<relation><property>3</property><value>Pin output</value></relation>"
											  "<relation><property>4</property><value>Pin input</value></relation>"                      
										  "</map>"
									 "</int>"
									 "<int size=\"1\"><name>I2C Address (8-127) or pin number (D0-D10)</name><min>0</min><max>127</max></int>"
									 "<int size=\"1\"><name>Command</name><min>0</min><max>255</max></int>"
 								     "<string size=\"42\"><name>Description</name></string>"
								  "</group>"
							  "</group>"
						  "</segment>\n"
                      "</cdi>\n\0";
                      
// define SHOWMESSAGES to write to the Serial object. The output can be viewed using the Arduino Serial Monitor;
#define SHOWMESSAGES

//#ifdef SHOWMESSAGES
//#include <SoftwareSerial.h>

//SoftwareSerial mySerial(5, 4); // RX, TX
//#endif


// receive buffer for the can packets
// WiFi => received over WiFi
// Note CAN handling is fake. A physical CAN interface is not necessary.


//enum CAN_message_type  WiFiMessageType = Standard;
//uint32_t WiFiIdentifier = 0;
//uint8_t WiFiDataLen = 0;
//uint8_t WiFiData[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

char msgString[128];                        // Array to store serial string

// some status variables, probably left over from the code I hacked and not used
bool wifiInitialised = false;
//enum NodeStatus {uninitialised, initialised};
enum CIDSent {noneSent, CID1Sent, CID2Sent, CID3Sent, CID4Sent, RIDSent, AMDSent, INITSent};

//enum NodeStatus ns = uninitialised;
enum CIDSent cidSent = noneSent;

// More status variables, possibly not all used
bool sendCDI = false;
//uint8_t sendCDIStep = 0;
bool waitForDatagramACK = false;
uint32_t addressOffset = 0;

// The incoming and outgoing can messages
//struct CAN_MESSAGE msgIn;
struct CAN_MESSAGE msgOut;

// The sender alias ( = JMRI)
uint16_t senderAlias = 0;

// storage for an incoming datagram.
// There is no code to prevent collisions when two datagrams arrive concurrently.
// in this model that was not an issue. (bless you wol).
byte datagramBuffer[72];
uint8_t datagramPtr = 0;

// allocate the fake EEPROM area
struct EEPROM_Data eed;



void setUpModel() {
    //// start the connection for the Arduino serial monitor
    //uart_set_baud(0, 115200);
//#ifdef SHOWMESSAGES
    //printf("\nStarting...\nESP8266 Connecting to %s\n", ssid);
//#endif

    //// initialise the ESP8266
    //wifiInitialised = initialiseESP8266(&wifiServer);

    //if (wifiInitialised) {
        //printf("Connection OK\n");
    //} else {
        //printf("Connection failed\n");
    //}

    //// Allocate the cdi data area (surprised this stays in scope)
    //OpenLCBCDI cdi;

    //    eed.event[0].eventId = 0x0501010101310000ULL;
    //    hexDump("eventId", &eed.event[0].eventId, 8);
    //    print64BitHex(eed.event[0].eventId); Serial.println();

    struct OpenLCBEvent ev;

    printf("sizeof(OpenLCBEvent): %d, instance: %d \ncomponent size (no padding): %d\n", sizeof(struct OpenLCBEvent), sizeof(ev),
        sizeof(ev.eventId) + 
        sizeof(ev.eventName) +
        sizeof(ev.eventType) +
        sizeof(ev.address) +
        sizeof(ev.eventValue));
    
    // ... and initialise it all to 0x00
    memset(&eed, 0x00, sizeof(struct EEPROM_Data));

    // set up the serial and user area
    eed.header.serial = 0xFFFF;
    strcpy((char *)&eed.header.userName, "my first Node");
    strcpy((char *)&eed.header.userDescription, "first node for cdi");

    // small sanity check
    ShowCdiXmlLength();

    // not needed. the CDI is static.
    //cdi.AssembleXML();

}

void setUpNode(){
    printf("%s: entering\n", __func__);
    
   // Query the node status

        // abbreviated setup
        // (not sure if this is strictly necessary - JMRI will send Verified Node queries on opening the configuration panel)

        // state machine for the initialisation process.
            // send a RID (Reserve ID) message
            msgOut.id = ((uint32_t)RID << 12 | ((uint32_t)alias & 0x0FFF));
            msgOut.len = 0;
            SendMessage();
            //printf("RID: ");
            //Serial.println(*getPId(&msgOut, ), HEX);
            cidSent = RIDSent;

            // transition to permitted
            // Send a AMD (Alias Map Definition) message
            msgOut.id = ((uint32_t)AMD << 12 | ((uint32_t)alias & 0x0FFF));
            setNodeidToData(&msgOut, nodeId);
            SendMessage();
            cidSent = AMDSent;
            //Serial.println("Preparing AMD");
            //printf("CANid: "); Serial.println(getId(&msgOut, ), HEX);
            //printf("datalength: "); Serial.println(msgOut.len));
            //printf("data: ");
            //for (int i = 0; i < 8; i++){
            //printf(Nybble2Hex((getDataByte(&msgOut, i) >> 4) & 0x0F));
            //printf(Nybble2Hex(getDataByte(&msgOut, i) & 0x0F));
            //}
            //Serial.println();

            // send an initialisation complete message
            msgOut.id = ((uint32_t)INIT << 12 | ((uint32_t)alias & 0x0FFF));
            setNodeidToData(&msgOut, nodeId);
            cidSent = INITSent;
            SendMessage();
        
    printf("%s: leaving\n", __func__);
}

void processMessage(struct CAN_MESSAGE* cm) {
    byte dataBytes[8];

    ////	char buf[50];

    //// initialisation failed - not much we can do here
    //if (!wifiInitialised)
    //return;

    //// Is any one listening? If not there's no point processing anything
    //if (!wifiClient.connected()) {
    //if (wifiServer.hasClient())    {
    //// a new client has connected
    //wifiClient = wifiServer.available();
    //} else {
    //return;
    //}
    //}

    // (un) initialise the output id. 0=>no valid message to send
    msgOut.id = 0;

 



        // process any incoming messages

        //msgIn.id = 0;

#ifdef SHOWMESSAGES
        printf("%s: Received Message. [%04x]", __func__, cm->id);
        if (cm->len > 0) {
            for (uint8_t i = 0; i < cm->len; i++) {
                if (cm->dataBytes[i] < 0x10) {
                    printf("0");
                }
                printf("%01x", cm->dataBytes[i]);
            }
        }
        printf("\n");
#endif

        //setId(cm, WiFiIdentifier & !0x10000000);
 //       msgIn.id = cm->id;
 //       setData(cm, &cm->dataBytes[0], cm->len);

        // Respond to the message
            printf("%s: Processing message %04x\n", __func__, ((cm->id & 0x0FFFF000) >> 12));
            senderAlias = (cm->id & 0x00000FFFUL);  // get JMRI's alias

            // decode the message type identifier (MTI)
            switch ((uint16_t)((cm->id & 0x0FFFF000) >> 12)) {

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
                // Respond with a Protocol Support Reply
                msgOut.id = ((uint32_t)PSR << 12 | ((uint32_t)alias & 0x0FFF));
                dataBytes[0] = (byte)(senderAlias >> 8);
                dataBytes[1] = (byte)(senderAlias & 0xFF);
                dataBytes[2] = (byte)(protflags >> 16);
                dataBytes[3] = (byte)(protflags >> 8) & 0xFF;
                dataBytes[4] = (byte)(protflags & 0xFF);
                dataBytes[5] = '\0';
                dataBytes[6] = '\0';
                dataBytes[7] = '\0';
                setData(&msgOut, &dataBytes[0], 8);
                SendMessage();
                break;
            }

            // A Verify Node ID Number Global request
            case VNIG:  // 0x9490
                // respond with our full node id
                msgOut.id = ((uint32_t)VNN << 12 | ((uint32_t)alias & 0x0FFF) );
                setNodeidToData(&msgOut, nodeId);
                SendMessage();
                break;

            // datagrams

            // Note: the following case statements are specific to this model.
            // profuction code will need to handle the alias part differently.

            case 0xA123:  // a single frame datagran addressed to me
                ReceiveDatagram(cm, &datagramBuffer[0], &datagramPtr);
                ProcessDatagram(senderAlias, alias, cm, &datagramBuffer[0], datagramPtr);
                break;

            case 0xB123:  // the first of a multiframe datagram addressed to me
            case 0xC123:  // one of the middle frames, addressed to me
                ReceiveDatagram(cm, &datagramBuffer[0], &datagramPtr);
                break;

            case 0xD123:  // the final multiframe datagram addressed to me
                ReceiveDatagram(cm, &datagramBuffer[0], &datagramPtr);
                // as this is the last frame, we can process the whole datagram of up to 72 characters
                ProcessDatagram(senderAlias, alias, cm, &datagramBuffer[0], datagramPtr);
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

            default:
                printf("%s: unrecognised message type: %04X\n", __func__, (uint16_t)((cm->id & 0x0FFFF000) >> 12));
                break;
            }
        



    //// is there anything to send?
    //if (getId(&msgOut) != 0) {
    //SendMessage();
    //}

}

//bool initialiseESP8266(WiFiServer* server) {
    //// Set up the ESP8266
    //// set the fixed ip adresses
    //WiFi.config(SERVERIP, GATEWAY, SUBNET, DNS);

    //// connect to the WLAN router
    //WiFi.begin(ssid, password);

    //uint8_t i = 0;
    //while (WiFi.status() != WL_CONNECTED && i++ < 21) delay(500);
    //if (i == 21) {
//#ifdef SHOWMESSAGES
        //printf("Could not connect to");
        //Serial.println(ssid);
//#endif
        //return false;
    //}

    ////start the server

    //server->begin();
    //server->setNoDelay(true);

//#ifdef SHOWMESSAGES
    //printf("Ready! Use 'telnet ");
    //printf(WiFi.localIP());
    //Serial.println(" 23' to connect");
    //printf("subnetMask: ");
    //Serial.println(WiFi.subnetMask());
    //printf("gatewayIP: ");
    //Serial.println(WiFi.gatewayIP());
    //printf("dnsIP: ");
    //Serial.println(WiFi.dnsIP());
//#endif

    //return true;
//}

bool SendMessage() {
    // Convert MsgOut to CANASCII format and send it to the WiFi client (JMRI)

    bool OK = true;

	printf("%s: sending message, id = %08X\n", __func__, msgOut.id);

    // Only send if there's data
    if (msgOut.id != 0) {
		msgOut.id = msgOut.id | 0x10000000;
        if(xQueueSendToBack(qh.xQueueCanToWiFi, &msgOut, 500 / portTICK_PERIOD_MS) != pdPASS ) {
            printf("Queue xQueueWiFiToCan full, discarding message\n");
            OK = false;
        }
        msgOut.id = 0;
    }
    vTaskDelay(0); // yield();
    return OK;
}


bool SendDROK(uint16_t senderAlias, uint16_t destAlias, bool pending) {
    // Send a Datagram Received OK message with the Reply Pending bit set, if requested.
    byte dataBytes[8];

    msgOut.id = ((uint32_t)DROK << 12 | ((uint32_t)alias & 0x0FFF));
    dataBytes[0] = (byte)(senderAlias >> 8);
    dataBytes[1] = (byte)(senderAlias & 0xFF);
    dataBytes[2] = (!pending) ? 0x0 : 0x80;  // set reply pending if requested
    dataBytes[3] = '\0';
    dataBytes[4] = '\0';
    dataBytes[5] = '\0';
    dataBytes[6] = '\0';
    dataBytes[7] = '\0';
    setData(&msgOut, &dataBytes[0], 8);
    return SendMessage();
}

bool sendSNIHeader(uint16_t senderAlias, uint16_t destAlias) {
    // Send the SNI header for the fixed data elements.
    // for simplicity, this is not a full frame, only three bytes are sent
    // to be followed by the actual string in subsequent frames
    byte dataBytes[3];

    msgOut.id = ((uint32_t)SNIIR << 12 | ((uint32_t)alias & 0x0FFF));
    dataBytes[0] = (byte)(destAlias >> 8) | 0x10;  // first frame
    dataBytes[1] = (byte)(destAlias & 0xFF);
    dataBytes[2] = 0x04;   // version number: sending manufacturer name, node model name, node hardware version and node software version
    setData(&msgOut, &dataBytes[0], 3);
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

    printf("infoText (%d) %s\n", strlen(infoText), infoText);

    msgOut.id = ((uint32_t)SNIIR << 12 | ((uint32_t)alias & 0x0FFF));
    dataBytes[0] = (byte)(destAlias >> 8);
    dataBytes[1] = (byte)(destAlias & 0xFF);

    // break the text into 8-byte chunks due to the length limit of a CAN frame
    while(pos < strlen(infoText) +1) {

        //printf("Character at "); printf(pos); printf(": "); printf(infoText[pos]); printf(" dataBytes["); printf(dataLen);	printf("]: ");
        dataBytes[dataLen++] = infoText[pos++];
        //Serial.println((char)dataBytes[dataLen - 1]);

        dataBytes[0] |= 0x30;  // intermediate frame

        if (dataLen > 7 || pos > strlen(infoText)) {
            setData(&msgOut, &dataBytes[0], dataLen);
            vTaskDelay(50 * portTICK_PERIOD_MS);   // add delay otherwise messages are not sent in sequence (Not necessary here: this is a problem with the CAN library)
            SendMessage();
            msgOut.id = ((uint32_t)SNIIR << 12 | ((uint32_t)alias & 0x0FFF));
            dataLen = 2;
        }
    }
    msgOut.id = 0;
    return fail;
}

bool sendSNIUserHeader(uint16_t senderAlias, uint16_t destAlias) {
    // Send the SNI header for the user data elements
    byte dataBytes[3];

    msgOut.id = ((uint32_t)SNIIR << 12 | ((uint32_t)alias & 0x0FFF));
    dataBytes[0] = (byte)(destAlias >> 8) | 0x30; // intermediate frame
    dataBytes[1] = (byte)(destAlias & 0xFF);
    dataBytes[2] = 0x02;   // version number: sending user-provided node name, user-provided node description
    setData(&msgOut, &dataBytes[0], 3);
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

    //printf("User infoText (");
    //printf(strlen(infoText));
    //printf(F(") "));
    //Serial.println(infoText);

    msgOut.id = ((uint32_t)SNIIR << 12 | ((uint32_t)alias & 0x0FFF));
    dataBytes[0] = (byte)(destAlias >> 8);
    dataBytes[1] = (byte)(destAlias & 0xFF);

    // break the text into 8-byte chunks due to the length limit of a CAN frame
    while(pos < strlen(infoText) +1) {

        //printf("Character at "); printf(pos); printf(": "); printf(infoText[pos]); printf(" dataBytes["); printf(dataLen);	printf("]: ");
        dataBytes[dataLen++] = infoText[pos++];
        //Serial.println((char)dataBytes[dataLen - 1]);

        if (!(userValue == 'D' && pos > strlen(infoText))) {
            dataBytes[0] |= 0x30;  // intermediate frame
        } else {
            dataBytes[0] &= ~0x10;  // last frame
        }
        if (dataLen > 7 || pos > strlen(infoText)) {
            setData(&msgOut, &dataBytes[0], dataLen);
            vTaskDelay(50);   // add delay otherwise messages are not sent in sequence (Not necessary here: this is a problem with the CAN library)
            SendMessage();
            msgOut.id = ((uint32_t)SNIIR << 12 | ((uint32_t)alias & 0x0FFF));
        }
        dataLen = 2;
    }
    msgOut.id = 0;
    return fail;
}

void SendDatagram(const uint16_t destAlias, uint16_t senderAlias, struct CAN_MESSAGE * msg, const char * data, uint16_t dataLength) {
    // send a datagram containing a reply to a request for data
    // Note: depending on the segment production code will need to retrieve from the appropriate storage: PROGMEM or EEPROM

    // fiddle with the location of the Segment specifier - JMRI is not consistent. See  S-9.7.4.2 for details
    uint8_t lenPos = (msg->dataBytes[1] == 0x40)? 7:6;
    // determine the offset within the segment of the data to read/write
    uint32_t address = ((uint32_t) msg->dataBytes[2] << 24) + ((uint32_t) msg->dataBytes[3] << 16) + ((uint32_t) msg->dataBytes[4] << 8) + ((uint32_t) msg->dataBytes[5]);
    // how much data was requested? Normally a read will request 64 (0x40) bytes, a write will send the length of the data element (e.g. 8 bytes for an event id).
    uint8_t length = (uint8_t) msg->dataBytes[lenPos];

    // copy the request bytes
    //setCANid(((&msgOutlength <= dataLength + 1 - 6)?0xB000:0xA000) + destAlias, senderAlias);

    // send a response datagram. The data content is the first 6/7 (depending on byte 1 of the request (S-9.7.4.2))
    msgOut.id = ((uint32_t)(0xB000 + destAlias) << 12 | ((uint32_t)senderAlias & 0x0FFF));
    //    buf[0] = 0x20;
    //    buf[1] = 0x53; // address space FF
    //    buf[2] = (byte)(address >> 24);
    //    buf[3] = (byte)((address >> 16) & 0xFF);
    //    buf[4] = (byte)((address >> 8) & 0xFF);
    //    buf[5] = (byte)(address & 0xFF);
    setData(&msgOut, &msg->dataBytes[0], msg->len - 1);  // retrieve as many data bytes as we were sent
    // pointer arithmetic, add 0x10 to byte 1 of the data area
    msgOut.dataBytes[1] = msg->dataBytes[1] | 0x10;

    // send the reply header
    SendMessage();

    // calculate how much data to send
    if (address + length > dataLength + 1) {
        length = dataLength + 1 - address; // include the terminating null
    }

    // send the data in 8-byte chunks, identify the last frame as the final frame in the datagram.
    for (int i = 0; i < length; i+=8) {
        msgOut.id = ((uint32_t)((i < (length - 8)? 0xC000 : 0xD000) + destAlias) << 12 | ((uint32_t)senderAlias & 0x0FFF));
        
        setData(&msgOut, (byte*)(&data[address + i]), (length - i) >= 8 ? 8 : length - i );

        SendMessage();
    }
    waitForDatagramACK = true;
}

void SendWriteReply(const uint16_t destAlias, uint16_t senderAlias, byte * buf, uint16_t errorCode) {
    // send a reply to a write request. This will fit in a singe frame, so use MTI OxAxxx.

    // again, fiddle with the request format to determine the length of the data to send
    uint8_t dataPos = (*(buf+1) == 0x00)? 7:6;
    //hexDump("SendWriteReply", buf, dataPos);
    msgOut.id = ((uint32_t)(0xA000 + destAlias) << 12 | ((uint32_t)senderAlias & 0x0FFF));
    setData(&msgOut, buf, dataPos);  // retrieve as many data bytes as we were sent
    //hexDump("SendWriteReply - set data buf", getPData(&msgOut, ), dataPos);
    //printf("*buf+1 "); Serial.println(*(buf+1));

    // pointer Arithmetic: add 0x10 to byte 1 of the data area.
    msgOut.dataBytes[1] = (*(buf+1)) + 0x10;
    //printf("getPData(&msgOut, ) + 1 "); Serial.println(* (getPData(&msgOut, ) + 1));

    // send an error response if the error code was set.
    // again pointer arithmetic to put things in the correct place. Could be improved to increase transparency.
    if (errorCode != 0) {
        msgOut.dataBytes[1] = (*buf+1) + 0x08;
        msgOut.dataBytes[dataPos] = (byte)errorCode >> 8;
        msgOut.dataBytes[dataPos + 1] = (byte)errorCode & 0xFF;
        msgOut.len = dataPos + 1;
    }

    //hexDump("SendWriteReply - message", getPData(&msgOut, ), dataPos);
    SendMessage();

}

void ReceiveDatagram(struct CAN_MESSAGE* m, byte* buffer, uint8_t * ptr) {
    // receive a datagram and build the datagram buffer
    uint8_t dataLength = 0;

    //    printf("ptr: ");
    //    Serial.println(*ptr);

    // 0xAxxx and 0xBxxx indicate the first (only) frame in the datagram, clear the buffer and reset the pointer before filling.
    if (((m->id & 0x0FFFF000) >> 12) == 0xA123 || ((m->id & 0x0FFFF000) >> 12) == 0xB123) {
        memset(buffer, '\0', 72);
        *ptr = 0;
    }

    // fill the received data into the buffer at the appropriate offset, then adjust the pointer
    getData(m, buffer + *ptr, &dataLength);
    *ptr += dataLength;
}

void ProcessDatagram(uint16_t senderAlias, uint16_t alias, struct CAN_MESSAGE* m, byte* datagram, uint8_t datagramLength) {
    // process the datagram we just received

    uint32_t errorCode = 0x0000;  // no error (so far)
    byte addressSpace = 0x00;     // undefined Segment

    //printf("Datagram Buffer: ");
    //for(int i = 0; i < 8; i++) {
    //if (datagram[i] < 0x10) printf("0");
    //printf(datagram[i], HEX);
    //printf(" ");
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
                //printf("Sending user data, length ") Serial.println(sizeof(eed.header.userName) + sizeof(eed.header.userDescription));
                SendDatagram(senderAlias, alias, m, eed.header.userName, sizeof(eed.header.userName) + sizeof(eed.header.userDescription));
                break;

            case 0xFD:
                // it's a request for space 0xFD - configuration Data
                SendDatagram(senderAlias, alias, m, (const char *) &eed.event[0], sizeof(struct EEPROM_Data) - sizeof(eed.header));
                break;

            case 0xFF:
                // its a cdi request
                SendDatagram(senderAlias, alias, m, cdiXml, strlen(cdiXml) + 1);
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
                printf("addressOffset: %d\n", addressOffset);
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
                printf("addressOffset: %d\n", addressOffset);
                // make sure we haven't gone outside the data area.
                // If OK, write the data
                // otherwise, create an error response.
                if (addressOffset + datagramLength <= sizeof(struct EEPROM_Data)- sizeof(eed.header)) {
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
    hexDump("EEPROM Data", &eed, sizeof(struct EEPROM_Data));
    //for (int i = 0; i < sizeof(EEPROM_Data); i+= 32){
    //printf(i); printf(" ");
    //for (int j = 0; j < 32; j++){
    //printf(Nybble2Hex((((char)eed)[(i*32)+j]) >> 4));	printf(Nybble2Hex((((char)eed)[(i*32)+j]) & 0x0F));
    //}
    //printf(" ");
    //for (int j = 0; j < 32; j++){
    //if (((byte)eed)[(i*32)+j] >= 32 && ((byte)eed)[(i*32)+j] <= 126){
    //printf(((char)eed)[(i*32)+j]);
    //}else{
    //printf(".");
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
    printf(buf);
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
    printf("serial: %04x\n", eed.header.serial);
    printf("userName: %s\n", eed.header.userName);
    printf("userDescription: %s\n", eed.header.userDescription);
    for(int i = 0; i < 20; i++) {
        if (eed.event[i].eventId != 0) {
            printf("event: %d\n", i);
            printf("eventId: ");
            print64BitHex(ReverseEndianness(&eed.event[i].eventId));
            printf("\n");
            printf("eventName: %s\n", eed.event[i].eventName);
            printf("eventType: %d\n", eed.event[i].eventType);
            printf("I2C address / pin: %d\n", eed.event[i].address);
            printf("command value: %d\n", eed.event[i].eventValue);
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
