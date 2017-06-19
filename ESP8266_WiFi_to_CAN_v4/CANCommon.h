#ifndef CANCommonIncluded
#define CANCommonIncluded
#if (ARDUINO >= 100)
    #include "Arduino.h"
#else
    #include "WProgram.h"    
#endif

#define CANASCII_SIZE 32

enum CAN_message_type {Standard = 0, Extended = 1};

#endif
