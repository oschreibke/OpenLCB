// private ID range?  d DCC - DIY
#define ID_PREFIX "08.01.00.0D."  
#define ID_SERIAL "00.01"
#define NODE_ID ID_PREFIX ID_SERIAL

#include "OpenLCBNode.h"

// the maximum nodes in a segment
#define MAX_ALIASES 48
//#include "OpenLCBAliasRegistry.h"
#include "OpenLCBCANInterface.h"

#include <mcp_can.h>
#define CAN0_INT 2                              // Set INT to pin 2
#define CAN0_CS  10                             // Set CS to pin 10


OpenLCBNode node;
MCP_CAN CAN0(CAN0_CS);
OpenLCBCANInterface canInt;

int looplimit = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println(F("Starting..."));

  node.setNodeId(NODE_ID);
  
  // init the can interface
  canInt.begin(&CAN0, CAN0_INT);

  if (canInt.getInitialised()){
	  node.setCanInt(&canInt);

      Serial.println(F("Initialisation complete"));
   } else {
      //while(1){};  // I've fallen over and can't get up (Todo: something intelligent here)
      Serial.println(F("Can initialisation failed"));
  }
}

void loop() {
	if (canInt.getInitialised())
	//    if (looplimit++ < 100)
	        node.loop();
}

