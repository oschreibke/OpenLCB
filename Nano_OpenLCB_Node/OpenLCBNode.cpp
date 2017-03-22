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
  genAlias();
  
Serial.println("Node initialised") ;
Serial.println(strNodeId); 
Serial.println(alias);
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
            registerMe();
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

bool OpenLCBNode::registerMe(){
	// register the node's alias on the system
	bool fail = false;
	
   NodeAliasStatus status = registry.getStatus(alias);
   switch (status){
	   case nodeAliasNotFound:
			msgOut.setId(((uint32_t)CID1 << 20) | ((uint32_t) (id >> 24) & 0x07FFF000 ) | ((uint32_t)alias & 0x0FFF) );
			msgOut.setDataLength(0);
			if (canInt->sendMessage(&msgOut)){
				registry.add(alias, id, CID1received);
			} else {
				fail = true;
				break;  // bail if we couldn't send the message
			}

			break;
			
	   case CID1received:
			msgOut.setId(((uint32_t)CID2 << 20) | ((uint32_t) (id >> 12) & 0x07FFF000 ) | ((uint32_t)alias & 0x0FFF) );
			msgOut.setDataLength(0);
			if (canInt->sendMessage(&msgOut)){
				registry.setStatus(alias, CID2received);
			} else {
				registry.remove(alias);                // start again if we couldn't send the message
				fail = true;
			}
			break;
				   
	   case CID2received:
			msgOut.setId(((uint32_t)CID3 << 20) | ((uint32_t) (id) & 0x07FFF000 ) | ((uint32_t)alias & 0x0FFF) );
			msgOut.setDataLength(0);
			if (canInt->sendMessage(&msgOut)){
				registry.setStatus(alias, CID3received);
			} else {
				registry.remove(alias);                // start again if we couldn't send the message
				fail = true;
			}

			break;	   
			
	   case CID3received:
			msgOut.setId(((uint32_t)CID4 << 20) | ((uint32_t) (id << 12) & 0x07FFF000 ) | ((uint32_t)alias & 0x0FFF) );
			msgOut.setDataLength(0);
			if (canInt->sendMessage(&msgOut)){
				registry.setStatus(alias, CID4received);
			} else {
				registry.remove(alias);                // start again if we couldn't send the message
				fail = true;
			}
			waitStart = millis();
			break;
				   
	   case CID4received:
	        // if we've waited 200 millisec, and no one has objected, send the RID;
	        if (millis() > waitStart + 200){
			    msgOut.setId((uint32_t)RID << 20);
			    msgOut.setDataLength(0);
			    if (canInt->sendMessage(&msgOut)){
			        registry.setStatus(alias, RIDreceived);
			    
					// transition to permitted
					msgOut.setId((uint32_t)AMD << 20);
					msgOut.setData(((byte*)&id) + 2, 6);       // id is uint64_t - only the low order 48 bits (6 bytes) are actually significant
					if (!canInt->sendMessage(&msgOut)){
						permitted = true;
					} else {
						fail = true;
					}
			    } else {
				    fail = true;
			    }
			break;  
			}
	   }
	   
	   if (fail){
		   // if something failed, retry with another alias	
		   registry.remove(alias);               
           genAlias(); 
		}
}

//
// private functions
//
void OpenLCBNode::genAlias(){
    alias = random(1, 65535);
	while (registry.getStatus(alias) !=  nodeAliasNotFound) // make sure no one is using it 
		alias = random(1, 65535);                           // otherwise generate a new one
}

void OpenLCBNode::processIncoming(){
	uint16_t senderAlias;
	uint64_t senderNodeId;
	
	if (canInt->receiveMessage(&msgIn)){
		if (msgIn.isControlMessage()){  // Alias management
			
			// this section is short of error handling
			// it assumes everyone plays by the rules and that we don't miss any messages
			
			senderAlias = (uint16_t (id & 0x00000FFF));
			switch ((ControlId)((id & 0x07FFF000) >> 12)){
				case (RID):
				    if (senderAlias == alias) {
						// has someone responded to our CID request?
						registry.remove(alias); // remove our alias (forces a restart)
					} else {
						// registering his own alias (we should already know this alias)
						registry.setStatus(senderAlias, RIDreceived);
					}
					break;
					
				case (AMD):
				case (AME):
				case (AMR):
				    break;
				default:
					switch ((byte)((id & 0x07FFF000) >> 20)) {
						// the CIDs
						case (0x07): //CID1
							registry.add(senderAlias, ((uint64_t) (id & 0x00FFF000)) << 24, CID1received);
							break;
							
						case (0x06): //CID2
						    senderNodeId = registry.getNodeId(senderAlias);
						    senderNodeId |= ((uint64_t) id & 0x00FFF000) << 12;
						    registry.setNodeId(senderAlias, senderNodeId);
						    registry.setStatus(senderAlias, CID2received);
						    break;
						    
						case (0x05): //CID3
						    senderNodeId = registry.getNodeId(senderAlias);
						    senderNodeId |= ((uint64_t) id & 0x00FFF000);
						    registry.setNodeId(senderAlias, senderNodeId);
						    registry.setStatus(senderAlias, CID3received);
						    break;
						    
						case (0x04): //CID4	
							senderNodeId = registry.getNodeId(senderAlias);
						    senderNodeId |= ((uint64_t) id & 0x00FFF000) >> 12;
						    registry.setNodeId(senderAlias, senderNodeId);
						    registry.setStatus(senderAlias, CID2received);
						    break;
						    
						default:
						    break;														
					}
				break;
			}
		}
	}
}
