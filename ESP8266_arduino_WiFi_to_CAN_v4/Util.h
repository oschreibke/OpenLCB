#ifndef UtilINCLUDED
#define UtilINCLUDED

#if (ARDUINO >= 100)
    #include "Arduino.h"
#else
    #include "WProgram.h"    
#endif


uint8_t Hex2Int(char ch);
char Nybble2Hex(uint8_t nybble);

#endif
