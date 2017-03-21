#ifndef OpenLCBCANInterfaceIncluded
#define OpenLCBCANInterfaceIncluded
#if (ARDUINO >= 100)
    #include "Arduino.h"
#else
    #include "WProgram.h"    
#endif

#include <mcp_can.h>
#include "OpenLCBMessage.h"


class OpenLCBCANInterface {
    MCP_CAN * can;
    bool canInitialised;
  public:
    OpenLCBCANInterface();
    void begin(MCP_CAN * mcpCanObj, uint8_t intPin);
    bool getInitialised();
    bool receiveMessage(OpenLCBMessage* msg);
    bool sendMessage(OpenLCBMessage* msg);
};



#endif
