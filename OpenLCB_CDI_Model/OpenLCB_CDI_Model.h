#ifndef _OPENLCB_CDI_MODEL_H_
#define _OPENLCB_CDI_MODEL_H_

#ifndef byte
#define byte uint8_t
#endif

// forward declarations
bool SendMessage(); 
bool SendDROK(uint16_t senderAlias, uint16_t destAlias, bool pending);
bool sendSNIHeader(uint16_t senderAlias, uint16_t destAlias);
bool sendSNIReply(uint16_t senderAlias, uint16_t destAlias, const char infoText[]);
bool sendSNIUserHeader(uint16_t senderAlias, uint16_t destAlias);
bool sendSNIUserReply(uint16_t senderAlias, uint16_t destAlias, const char userValue);
void SendDatagram(const uint16_t destAlias, uint16_t senderAlias, struct CAN_MESSAGE* msg, const char * data, uint16_t dataLength);
void SendWriteReply(const uint16_t destAlias, uint16_t senderAlias, byte * buf, uint16_t errorCode);
void ReceiveDatagram(struct CAN_MESSAGE* m, byte* buffer, uint8_t * ptr);
void ProcessDatagram(uint16_t senderAlias, uint16_t alias, struct CAN_MESSAGE* m, byte* datagram, uint8_t datagramLength);
void DumpEEPROM();
void p(const char *fmt, ... );
void hexDump (const char *desc, void *addr, int len);
void DumpEEPROMFormatted();
uint64_t ReverseEndianness(uint64_t *val);
void processMessage(struct CAN_MESSAGE* cm);
void setUpModel(void);
void setUpNode(void);

#endif
