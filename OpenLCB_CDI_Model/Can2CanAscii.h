#ifndef CAN2CANASCIIINCLUDED
#define CAN2CANASCIIINCLUDED

#include "espressif/esp_common.h"

void Can2CanAscii(uint32_t * CANIdentifier, uint8_t * CANDataLen, uint8_t CANData[], char CANAscii[]);

#endif
