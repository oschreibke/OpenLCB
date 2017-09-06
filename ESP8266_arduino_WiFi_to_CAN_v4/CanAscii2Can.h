#ifndef CANASCII2CANINCLUDED
#define CANASCII2CANINCLUDED
#if (ARDUINO >= 100)
    #include "Arduino.h"
#else
    #include "WProgram.h"    
#endif

#include "CANCommon.h"

enum WiFiDecodeStatus {expect_colon, expect_msg_type, process_identifier, process_data};

//CAN_message_type  WiFiMessageType = Standard;
//long unsigned int WiFiIdentifier = 0;
//byte WiFiDataLen = 0;
//byte WiFiData[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

bool CanAscii2Can(uint32_t * id, CAN_message_type * messageType, uint8_t * len, uint8_t buf[], char * ch);
uint8_t Hex2Int(char ch);

#endif
