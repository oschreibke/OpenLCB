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
enum ControlId: uint16_t {CID1 = 0x7000,      // Check ID  // N.B. the low-order nybble will be overwritten
	                      CID2 = 0x6000,
	                      CID3 = 0x5000,
	                      CID4 = 0x4000,
	                      RID  = 0x0700,      // Reserve ID
	                      AMD  = 0x0701,      // Alias Map Definition
	                      AME  = 0x0702,      // Alias Map Enquiry
	                      AMR  = 0x0703,      // Alias Map Release
	                      EIR0 = 0x0710,      // Error Information 0
	                      EIR1 = 0x0711,      // Error Information 1
	                      EIR2 = 0x0712,      // Error Information 2
	                      EIR3 = 0x0713,      // Error Information 3
	                     }; 
	                     
// from MtiAllocations.odt	                     
enum MTI: uint16_t{// Basic
	               INIT      = 0x9100,        // Initialization Complete
	               INITSS    = 0x9101,        // Initialization Complete (Simple-set Sufficient)
                   VNIA      = 0x9488,        // Verify Node ID Number Addressed
                   VNIG      = 0x9490,        // Verify Node ID Number Global
                   VNN       = 0x9170,        // Verified Node ID Number
                   VNNSS     = 0x9171,        // Verified Node ID Number (Simple-set Sufficient)
                   OIR       = 0x9068,        // Optional Interaction Rejected
                   TDTE      = 0x90A8,        // Terminate Due to Error
                   // Protocol support
                   PSI       = 0x9828,        // Protocol Support Inquiry
                   PSR       = 0x9668,        // Protocol Support Reply
                   // Event Exchange
                   IC        = 0x98F4,        // Identify Consumer
                   CRI       = 0x94A4,        // Consumer Range Identified
                   CIVU      = 0x94C7,        // Consumer Identified w validity unknown
                   CICV      = 0x94C4,        // Consumer Identified as currently valid
                   CICI      = 0x94C5,        // Consumer Identified as currently invalid
 //                  CI        = 0x94C6,        // Consumer Identified (reserved) 
                   IP        = 0x9914,        // Identify Producer
                   PRI       = 0x9524,        // Producer Range Identified
                   PIVU      = 0x9547,        // Producer Identified w validity unknown
                   PICV      = 0x9544,        // Producer Identified as currently valid
                   PICI      = 0x9545,        // Producer Identified as currently invalid
//                   PI        = 0x9546,        // Producer Identified (reserved)
                   IEA       = 0x9968,        // Identify Events Addressed
                   IEG       = 0x9970,        // Identify Events Global
                   LE        = 0x9594,        // Learn Event
                   PCER      = 0x95B4,        // Producer/Consumer Event Report
                   // Traction control
                   TCC       = 0x95E8,        // Traction Control Command
                   TCR       = 0x91E8,        // Traction Control Reply
                   TPC       = 0x99E9,        // Traction Proxy Command
                   TPR       = 0x95E9,        // Traction Proxy Reply
                   // Other
                   XN        = 0x9820,        // Xpressnet
                   // Remote Button
                   RBRQ      = 0x9948,        // Remote Button Request
                   RBR       = 0x9549,        // Remote Button Reply
                   // Traction Ident
                   STNIIRQ   = 0x9DA8,        // Simple Train Node Ident Info Request
                   STNIIR    = 0x99C8,        // Simple Train Node Ident Info Reply
                   // Node Ident
                   SNIIRQ    = 0x9DE8,        // Simple Node Ident Info Request
                   SNIIR     = 0x9A08,        // Simple Node Ident Info Reply
                   // Datagram
                   //        = 0xAddd         // CAN-Datagram Content (one frame)
                   //        = 0xBddd         // CAN-Datagram Content (first frame) 
                   //        = 0xCddd         // CAN-Datagram Content (middle frame)
                   //        = 0xDddd         // CAN-Datagram Content (last frame)                            
                   DROK      = 0x9A28,        //Datagram Received OK
                   DR        = 0x9A48,        // Datagram Rejected
                   // Stream
                   SIRQ      = 0x9CC8,        // Stream Initiate Request
                   SIR       = 0x9868,        // Stream Initiate Reply
                   //        = 0xFddd,        // Stream Data Send
                   SDP       = 0x9888,        // Stream Data Proceed
                   SDC       = 0x98A8         // Stream Data Complete
};	                     

/*
 * Adopted as NMRA Standard S-9.7.3
OpenLCB Message Network Standard
Protocols
*/
enum protocol: uint32_t {
	               SPSP   = 0x800000, // Simple Protocol subset
                   DGP    = 0x400000, // Datagram Protocol
                   STP    = 0x200000, // Stream Protocol
                   MCP    = 0x100000, // Memory Configuration Protocol
                   RP     = 0x080000, // Reservation Protocol
                   EEP    = 0x040000, // Event Exchange (Producer/Consumer) Protocol
                   IDP    = 0x020000, // Identification Protocol
                   TLCP   = 0x010000, // Teaching/Learning Configuration Protocol
                   RBP    = 0x008000, // Remote Button Protocol
                   ADCDIP = 0x004000, // Abbreviated Default CDI Protocol
                   DP     = 0x002000, // Display Protocol
                   SNIP   = 0x001000, // Simple Node Information Protocol
                   CDIP   = 0x000800, // Configuration Description Information (CDI)
                   TCP    = 0x000400, // Traction Control Protocol (Train Protocol)
                   FDIP   = 0x000200, // Function Description Information (FDI)
                   DCCCSP = 0x000100, // DCC Command Station Protocol
                   STNIP  = 0x000080, // Simple Train Node Information Protocol
                   FCP    = 0x000040, // Function Configuration
                   FUP    = 0x000020, // Firmware Upgrade Protocol
                   FUAP   = 0x000010  // Firmware Upgrade Active
};

class OpenLCBMessage {
      uint32_t id;
      uint8_t dataLength;
      byte data[8];

      byte filler[50];
      
  public:
    OpenLCBMessage(void);
    uint32_t getId();
    MTI getMTI();
    uint16_t getSenderAlias();
    void setId(uint32_t newId);
    uint8_t getDataLength();
    void setDataLength(uint8_t newDataLength);
    void getData(byte* dataBuf, uint8_t length);
    uint8_t getDataByte(uint8_t index);
    void setData(byte* newDataBuf, uint8_t newDataLength);
    uint32_t * getPId();
	byte* getPData();
	uint8_t * getPDataLength();
	void initialise();
//	bool getNewMessage();
    bool isControlMessage();
    void setCANid(uint16_t MTI, uint16_t alias);
    void setNodeidToData(uint64_t nodeId);
    uint64_t getNodeIdFromData();
    uint16_t getDestAliasFromData();	

  private:
//    void GenAlias();

};


#endif
