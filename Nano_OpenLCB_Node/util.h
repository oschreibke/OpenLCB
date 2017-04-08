#ifndef util_h
#define util_h

#if (ARDUINO >= 100)
    #include "Arduino.h"
#else
    #include "WProgram.h"    
#endif

class util{
public:
static void print64BitHex(uint64_t val);
static void print8BitHex(uint8_t val);
} ;

#endif
