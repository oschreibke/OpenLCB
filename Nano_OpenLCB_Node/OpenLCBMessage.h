#ifndef OpenLCBMessageIncluded
#define OpenLCBMessageIncluded

#if (ARDUINO >= 100)
    #include "Arduino.h"
#else
    #include "WProgram.h"    
#endif

// CAN ID format (from s-9.7.0.3)
//
//   3         2         1         0
//  10987654321098765432109876543210
//  ---                               not used (the extended id is 29 bits long)
//     x                              always 1 (ignore) = "Solo Top bit"  
//      x                             0 = CAN control frame, 1 = OpenLCB message
//       xxxxxxxxxxxxxxx              Variable field
//                      xxxxxxxxxxxx  Source NID Alias
//

// control frame ids (bit 27 = 0) (s-9.7.2.1)
enum ControlId: uint16_t {CID1 = 0x0700,      // Check ID  // N.B. the low-order nybble will be overwritten
	                      CID2 = 0x0600,
	                      CID3 = 0x0500,
	                      CID4 = 0x0400,
	                      RID  = 0x0700,      // Reserve ID
	                      AMD  = 0x0701,      // Alias Map Definition
	                      AME  = 0x0702,      // Alias Map Enquiry
	                      AMR  = 0x0703,      // Alias Map Release
	                      EIR0 = 0x0710,      // Error Information 0
	                      EIR1 = 0x0711,      // Error Information 1
	                      EIR2 = 0x0712,      // Error Information 2
	                      EIR3 = 0x0713,      // Error Information 3
	                     }; 

class OpenLCBMessage {
      uint32_t id;
      byte data[8];
      uint8_t dataLength;
//      bool newMessage;
      
  public:
    OpenLCBMessage(void);
    uint32_t getId();
    void setId(uint32_t newId);
    uint8_t getDataLength();
    void setDataLength(uint8_t newDataLength);
    void getData(byte* dataBuf, uint8_t length);
    void setData(byte* newDataBuf, uint8_t newDataLength);
    uint32_t* getPId();
	byte* getPData();
	uint8_t * getPDataLength();
	void initialise();
//	bool getNewMessage();
    bool isControlMessage();

  private:
//    void GenAlias();

};


#endif
