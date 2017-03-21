#ifndef OpenLCBNodeIncluded
#define OpenLCBNodeIncluded

#if (ARDUINO >= 100)
    #include "Arduino.h"
#else
    #include "WProgram.h"    
#endif

#include "OpenLCBMessage.h"
#include "OpenLCBAliasRegistry.h"
#include "OpenLCBCANInterface.h"

class OpenLCBNode {
    uint64_t id;        // 48-bit id
    uint16_t alias;     // 16-bit alias
    char strNodeId[18]; // = "00.00.00.00.00.00";
    bool permitted;     // false => inhibited
    OpenLCBMessage msgIn, msgOut;
    OpenLCBAliasRegistry registry;
    uint32_t waitStart;
    OpenLCBCANInterface* canInt;

  public:
    OpenLCBNode(const char* Id);
    uint64_t getId();
    uint16_t getAlias();
    void setCanInt(OpenLCBCANInterface* canInt);
    char* ToString();
    void loop();

  private:
    void GenAlias();
    void processIncoming();
    bool RegisterMe();
    
};


#endif
