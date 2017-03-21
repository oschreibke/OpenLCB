#include "OpenLCBNode.h"
#include "OpenLCBAliasRegistry.h"
#include "OpenLCBCANInterface.h"

// Method definitions for OpenLCBNode

OpenLCBNode::OpenLCBNode(const char* Id) {

  char ch;
  
  id = 0;
  alias = 0;
  strcpy(strNodeId, "00.00.00.00.00.00");
  //initialised = false;
  permitted = false;

  // ToDo: normalise the id to nn.nn.nn.nn.nn.nn
  if (strlen(Id) != 17) {
    id = 0;
    return;
  }
//  Serial.begin(115200);
//  Serial.println("Converting node id...");
  // convert from nn.nn.nn.nn.nn.nn format to an uint64_t
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 2; j++) {
      ch = Id[(i * 3) + j];
      if (ch >= '0' && ch <= '9') {
        id = (id << 4) + (ch - 48);
      }
      else { // invalid character in the id
        id = 0;
        return;
      }
    }
    if (i < 5 && Id[(i * 3) + 2] != '.') {
      // id is not properly normalised
      id = 0;
      return;
    }
  }
  // we got this far, the node id must be OK
  strcpy (strNodeId, Id);
  GenAlias();
}

// this is where the work gets done
void OpenLCBNode::loop(){
	processIncoming();
	
    if (permitted){
		} else {
		// wait for the alias registration for jmri (process any others too)
		if (registry.JMRIRegistered()){
			// now try registering our alias
            // send the initialised message
            RegisterMe();
			}
		}
}

uint64_t OpenLCBNode::getId() {
  return id;
}

uint16_t OpenLCBNode::getAlias(){
  return alias;
}

void OpenLCBNode::setCanInt(OpenLCBCANInterface* canInterface){
	canInt = canInterface;
	}

char* OpenLCBNode::ToString(){
  return strNodeId;
}

bool OpenLCBNode::RegisterMe(){
   NodeAliasStatus status = registry.getStatus(alias);
   switch (status){
	   case nodeAliasInit:
			msgOut.setId(((uint32_t)CID1 << 20) | ((uint32_t) (id >> 24) & 0x07FFF000 ) | ((uint32_t)alias & 0x0FFF) );
			msgOut.setDataLength(0);
			canInt->sendMessage(&msgOut);
			registry.add(alias, id, CID1received);
			break;
			
	   case CID1received:
			msgOut.setId(((uint32_t)CID2 << 20) | ((uint32_t) (id >> 12) & 0x07FFF000 ) | ((uint32_t)alias & 0x0FFF) );
			msgOut.setDataLength(0);
			canInt->sendMessage(&msgOut);
			registry.setStatus(alias, CID2received);
			break;
				   
	   case CID2received:
			msgOut.setId(((uint32_t)CID3 << 20) | ((uint32_t) (id) & 0x07FFF000 ) | ((uint32_t)alias & 0x0FFF) );
			msgOut.setDataLength(0);
			canInt->sendMessage(&msgOut);
			registry.setStatus(alias, CID3received);
			break;	   
			
	   case CID3received:
			msgOut.setId(((uint32_t)CID4 << 20) | ((uint32_t) (id << 12) & 0x07FFF000 ) | ((uint32_t)alias & 0x0FFF) );
			msgOut.setDataLength(0);
			canInt->sendMessage(&msgOut);
			registry.setStatus(alias, CID4received);
			waitStart = millis();
			break;
				   
	   case CID4received:
	        // if we've waited 200 millisec, and no one has objected, send the RID;
	        if (millis() > waitStart + 200){
			    msgOut.setId((uint32_t)RID << 20);
			    msgOut.setDataLength(0);
			    canInt->sendMessage(&msgOut);
			    registry.setStatus(alias, RIDreceived);
			    
			    // transition to permitted
			    msgOut.setId((uint32_t)AMD << 20);
			    msgOut.setDataLength(8);
			   
			    permitted = true;
			}
	   }	
}

//
// private functions
//
void OpenLCBNode::GenAlias(){
  alias = random(1, 65535);
}

void OpenLCBNode::processIncoming(){
	
	}
