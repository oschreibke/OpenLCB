// encode a can message to CANASCII

#include "CANCommon.h"
#include "Can2CanAscii.h"

void Can2CanAscii(uint32_t * CANIdentifier, uint8_t * CANDataLen, uint8_t CANData[], char CANAscii[]){
	CAN_message_type  CANMessageType = Extended;        // always extended for OpenLCB
    char buf[50]; 

//    if ((*CANIdentifier & 0x40000000) == 0x40000000) {  // Determine if message is a remote request frame.
#ifdef SHOWLCD
      sprintf(msgString, " REMOTE REQUEST FRAME");
      lcd.clear();
      lcd.print(msgString);
#endif
//    }
    //else {
      //if ((*CANIdentifier & 0x80000000) == 0x80000000)  {  // Determine if ID is standard (11 bits) or extended (29 bits)
        //CANMessageType = Extended;
        ////*CANIdentifier &= 0x1FFFFFFF;
      //}
      //else {
        //CANMessageType = Standard;
        ////*CANIdentifier &= 0x7FF;
      //}
     
      if (CANMessageType == Extended){
		  //sprintf(CANAscii, ":X%08hhXN", *CANIdentifier);
		  sprintf(CANAscii, ":X%08lXN", *CANIdentifier);
	  } else {
		  sprintf(CANAscii, ":S%03lXN", *CANIdentifier);
	  }
	  for (uint8_t i = 0; i < *CANDataLen; i++){     // write any data bytes
		  sprintf(buf, "%02hhX", *(CANData + i));
		  strcat(CANAscii, buf);
		  }
	  strcat(CANAscii, ";");                        // add the terminator
	
	}
