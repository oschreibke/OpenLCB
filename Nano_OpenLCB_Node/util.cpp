#include "util.h"

void util::print64BitHex(uint64_t val){
    //char buf[50];

    //sprintf(buf, "%08lX%08lX", (uint32_t)(val>>32), (uint32_t)(val & 0xFFFFFFFFULL));

    //Serial.print( buf );
    
    for (int8_t i = 7; i >= 0; i--){
		util::print8BitHex((uint8_t) (val >> (8*i)) & 0xFFULL);
	}
} 

void util::print8BitHex(uint8_t val){
//    char buf[50];
//
//    sprintf(buf, "%02hhX", val);
//
//    Serial.print( buf );
    
    if (val < 0x10){
		Serial.print(F("0"));
		}
	Serial.print(val, HEX);	
} 
