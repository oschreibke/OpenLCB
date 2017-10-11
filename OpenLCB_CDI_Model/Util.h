#ifndef UtilINCLUDED
#define UtilINCLUDED

#include "espressif/esp_common.h"

uint8_t Hex2Int(char ch);
char Nybble2Hex(uint8_t nybble);

void print64BitHex(uint64_t val); 
void print8BitHex(uint8_t val);

#endif
