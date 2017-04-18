#ifndef CAN2CANASCIIINCLUDED
#define CAN2CANASCIIINCLUDED

#if (ARDUINO >= 100)
    #include "Arduino.h"
#else
    #include "WProgram.h"    
#endif


void Can2CanAscii(uint32_t * CANIdentifier, uint8_t * CANDataLen, uint8_t CANData[], char CANAscii[]);

#endif
