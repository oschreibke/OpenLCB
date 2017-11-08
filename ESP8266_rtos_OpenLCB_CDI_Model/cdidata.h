#ifndef __CDIDATA_H__
#define __CDIDATA_H__

#include "espressif/esp_common.h"

// struct defining the user area of the EEPROM storage ( = Segment 253),
// except for the serial which is the 16-bit node identifier to be appended to the
// assigned OpenLCB number range. In my case 05 01 01 01 31.
struct OpenLCBEEPROMHeader {
    uint16_t  serial;
    char      userName[63];
    char      userDescription[64];
};

// Struct to hold the configuration for an event.
// Beware C++ padding which will silently add bytes. The padding causes the whole data structure to be larger than 1024 bytes
// which is all the Arduino Nano has.

// The eventtypes allow configuration of an I2C device (= ATTiny85, etc).
// For example: setting a slowmo point (switch, turnout, weiche, whatever) to a set angle.

struct OpenLCBEvent {
    uint64_t       eventId;
    char           eventName[45];
    uint8_t        inputOutput;   
    uint8_t        address;
    uint8_t        eventValue;
};


// build the data in RAM
struct EEPROM_Data {
    struct OpenLCBEEPROMHeader header;
    struct OpenLCBEvent event[20];
};


#endif
