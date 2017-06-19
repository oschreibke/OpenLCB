/*
 * Nano_OpenLCB_EEPROM_Initialise.ino
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

#include <EEPROM.h>

#define NODE_ID     0x0801000D0000ULL
#define NODE_NUMBER 0001


struct OpenLCBEEPROMHeader {
    uint16_t  serial;
    char      userName[63];
    char      userDescription[64];
    uint8_t  events; // maximum 20
};

enum EVENTTYPE:uint8_t {OnOff, SBBSignalAspect};

struct OpenLCBEvent {
    uint64_t eventId;
    EVENTTYPE eventType;
    uint8_t  eventValue;
    uint8_t  pin;
    char     eventName[33];
};

void print8BitHex(uint8_t val) {
    if (val < 0x10) {
        Serial.print(F("0"));
    }
    Serial.print(val, HEX);
}

void print64BitHex(uint64_t val) {
    for (int8_t i = 7; i >= 0; i--) {
        print8BitHex((uint8_t) (val >> (8*i)) & 0xFFULL);
    }
}

void WriteEEPromData() {
    OpenLCBEEPROMHeader eepromData = {
        NODE_NUMBER,
        "Test Node 00.01",
        "Description of node 00.01",
        4
    };

    Serial.print("serial: ");
    Serial.println(eepromData.serial);
    Serial.print("userName: ");
    Serial.println(eepromData.userName);
    Serial.print("userDescription: ");
    Serial.println(eepromData.userDescription);
    Serial.print("events: ");
    Serial.println(eepromData.events);


    int eeAddress = 0;

    EEPROM.put(eeAddress, eepromData);

    eeAddress += sizeof(OpenLCBEEPROMHeader);
    OpenLCBEvent e = {NODE_ID + NODE_NUMBER << 16 + 1,
                      OnOff,
                      0,
                      4,
                      "Point 1 straight"
                     };

    EEPROM.put(eeAddress, e);

    e.eventId = NODE_ID + NODE_NUMBER << 16 + 2;
    e.eventValue = 1;
    strcpy(e.eventName, "Point 1 thrown");
    eeAddress += sizeof(OpenLCBEvent);

    EEPROM.put(eeAddress, e);


    e.eventId = NODE_ID + NODE_NUMBER << 16 + 3;
    e.eventValue = 0;
    e.pin = 5;
    strcpy(e.eventName, "Point 2 Straight");

    eeAddress += sizeof(OpenLCBEvent);
    EEPROM.put(eeAddress, e);

    e.eventId = NODE_ID + NODE_NUMBER << 16 + 4;
    e.eventValue = 1;
    strcpy(e.eventName, "Point 2 thrown");
    eeAddress += sizeof(OpenLCBEvent);

    EEPROM.put(eeAddress, e);

}

void ReadEEPromData() {
    OpenLCBEEPROMHeader eepromData;
    OpenLCBEvent ev;
    int eeAddress = 0;

    EEPROM.get(eeAddress, eepromData);

    Serial.print("serial: ");
    Serial.println(eepromData.serial);
    Serial.print("userName: ");
    Serial.println(eepromData.userName);
    Serial.print("userDescription: ");
    Serial.println(eepromData.userDescription);
    Serial.print("events: ");
    Serial.println(eepromData.events);

    eeAddress += sizeof(OpenLCBEEPROMHeader);

    for (int i = 0; i < eepromData.events; i++) {
        EEPROM.get(eeAddress, ev);

        Serial.print("EventId: ");
        print64BitHex(ev.eventId);
        Serial.println();
        Serial.print("Name: ");
        Serial.println(ev.eventName);
        Serial.print("Type: ");
        Serial.println(ev.eventType);
        Serial.print("Value: ");
        Serial.println(ev.eventValue);
        Serial.print("Pin: ");
        Serial.println(ev.pin);

        eeAddress += sizeof(OpenLCBEvent);
    }
}

void setup() {

    Serial.begin(115200);

    Serial.print("EEPROM size: ");
    Serial.println(EEPROM.length());

    //WriteEEPromData();

    Serial.println("Data written");

    ReadEEPromData();

}

void loop() {}
