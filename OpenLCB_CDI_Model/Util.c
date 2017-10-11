// Utility functions

#include "Util.h"

const char hexDigits[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

uint8_t Hex2Int(char ch) {
  // convert the input character from a Hex value to Int
  // if the input is not a valid hex character (0..9A..F), 16 will be returned and is an error value

  uint8_t i = 0;
  for (i = 0; i <= 15; i++) {
    if (ch == hexDigits[i]) break;
  }
  //Serial.println(); Serial.print("in Hex2Int. ch = "); Serial.print(ch); Serial.print(", i = "); Serial.println(i);
  return i;
}

char Nybble2Hex(uint8_t nybble) {
	// convert the input nybble to a hex character representation (0..9A..F)
	// if the input is out of range (0..15) return Null (\0)
  if (nybble >= 0 && nybble <= 15)
    return hexDigits[nybble];
  else
    return '\0';
}

// print a 64-bit integer in Hex 
void print64BitHex(uint64_t val) {

    for (int8_t i = 7; i >= 0; i--) {
        print8BitHex((uint8_t) (val >> (8*i)) & 0xFFULL);
    }
}

// Print an eight bit integer as two Hex digits  
void print8BitHex(uint8_t val) {

    if (val < 0x10) {
        printf("0");
    }
    printf("%1x", val);
}
