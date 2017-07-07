/*

Working model for the configuration of an OpenLCB node using JMRI.
As such it is unoptimised and is missing much of the error handling.
It also comes with the usual disclaimers:
    It works for me, there is no guarantee it will work for you.
    If it breaks you may keep all the pieces.
    Please review the code before using. 
    Remember you chose to install and run it on your hardware. If "Bad Things" happen, I will not accept any responsibility. 

I expect it will also work with other configuration tools.

The code is written for an ESP8266. 
I used a nodemcu 0.9 development board, but expect it to work on any of the ESP8266 boards 
which expose a serial interface. Most of, if not all, ESP-01 - ESP-12.

The ESP8266 listens on port 23 (telnet). JMRI needs to be configured to use the GridConnect network interface
with the IP address assigned to the ESP8266 and port 23.

It will use the serial interface. The output can be viewed using the Arduino IDE's Serial Monitor.

The code implements two messages, send these using JMRI's Send Frame tool:
   [11111123]    -  hex dump the configuration area.
   [12222123]    -  formatted dump of the configuration area.

N.B. Configuration information is normally persisted to EEPROM, the model simply builds the structure in RAM.
The information will be lost on restart of the ESP8266.

Reference Material:
    The LCC standards documentation, particularily: 
         S-9.7.3.2 Datagram Transport
         S-9.7.4.1 Configuration Description Information
         S-9.7.4.2 Memory Configuration
         and their associated tech notes.

Basic concepts:

    Configuration is stored in three areas, so-called segments:
        Segment FB (251) contains the configuration information. Usually kept in EEPROM.
        Segment FD (253) contains the user name and description of the node. Usually kept in EEPROM.
        Segment FF (255) contains the XML for the CDI (Configuration Description Information). Usually kept in PROGMEM.
       
    When opening the configuration tool JMRI will request the data in all three segments in 64 byte chunks.
    The request sequence is:
        JMRI sends a datagram specifying the Segment and the offset within the segment, requesting 64 bytes be sent.
        The model responds with a "Datagram Received OK" message with the reply pending bit set.
        The model then sends a Datagram containing the requested data. Sending less than 64 bytes indicates end of data.
        JMRI responds with a "Datagram Received OK" message.
        
    When writing data the sequence is:
        JMRI sends a datagram containing the data element as described in the CDI, with the segment id and appropriate offset.
        The model responds with a "Datagram Received OK" message with the reply pending bit set. Not sure this is necessary. 
        The model updates the nominated data area.
        The model then sends a Datagram containing the header information from JMRI's datagram, with 0x10 added to the second byte
        to undicate successful processing.
        JMRI responds with a "Datagram Received OK" message.
        
        Note: this implies the CDI must match the program's expected structure of the data.
        
Pitfalls:
    The GCC compiler pads structs; JMRI is (rightly) unaware of any padding. I had to increase the size of the event name to 37
    to eliminate compiler padding. This means the data structure is now slightly too large for the EEPROM available on the Arduino
    Nano. A "ToDo".
    The ESP8266 is little-endian, OpenLCB is big-endian. I needed to reverse the bytes of the event id to display it correctly in the 
    serial monitor.

Files in this folder:
    Description.h This file
    ESP8266_OpenLCB_CDI_Model.ino.  The main sketch; does the processing.
    OpenLCBCDI.h .cpp               CDI definitions in .h, some related utility functions in .cpp
    CanAscii2Can.h .cpp             Handles translation from CANASCII format to a CAN message.
    Can2CanAscii.h .cpp             Handles translation from a CAN message to CANASCII format.
    CANCommon.h                     Defines items common to the two CAN modules. 
    OpenLCBMessage.h .cpp           Defines the CAN message and methods to manipulate it.
    Util.h .cpp                     Defines some utility functions.
                
*/
