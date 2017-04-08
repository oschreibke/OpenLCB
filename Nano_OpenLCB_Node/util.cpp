#include "util.h"

void util::print64BitHex(uint64_t val){
    char buf[50];

    sprintf(buf, "%08lX%08lX", (uint32_t)(val>>32), (uint32_t)(val & 0xFFFFFFFFULL));

    Serial.print( buf );
} 

void util::print8BitHex(uint8_t val){
    char buf[50];

    sprintf(buf, "%02hhX", val);

    Serial.print( buf );
} 
