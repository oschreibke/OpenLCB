#ifndef CANMESSAGE_H
#define CANMESSAGE_H

#include "CANCommon.h"

typedef struct CAN_MESSAGE{
    uint32_t id; 
    uint8_t  ext;
    uint8_t  len; 
    uint8_t  dataBytes[8];
} can_message;

#endif
