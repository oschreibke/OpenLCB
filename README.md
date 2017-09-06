# OpenLCB

Contains the following folders:
- ATTiny85_ServoController            Example servo controller.
- ESP8266_arduino_OpenLCB_CDI_Model   Working model of OpenLCB configuration (using JMRI)
- ESP8266_arduino_WiFi_to_CAN_v4      The sketch to directly interface the WiFi and CAN bus. Note: this sketch is completely untested.
- ESP8266_arduino_WiFiTelnetToSerial  The sketch for the WiFi gateway.
- ESP8266_rtos_ota                    Simple rtos demo of OTA (over the air) flashing.
- fritzing                            Fritzing connection diagrams.
- Nano_CANASCII_to_CAN                The sketch to convert CANASCII to CAN messages. 
- Nano_OpenLCB_EEPROM_Initialise      The sketch to set the configuration for the OpenLCB node to EEPROM.
- Nano_OpenLCB_Node                   The sketch for the OpenLCB Node.

To do:
 - Test and extend the processing of events.
 - Test the ESP8266 to CAN interface.
 - Allow configuration of the servo controller from EEPROM.

The sketches above rely on having the board definitions for ATTiny85 (Digispark), Arduino Nano, and ESP8266 configured in the Arduino IDE.
  
