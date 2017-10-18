#ifndef __CDIHANDLER_H__
#define __CDIHANDLER_H__

#include "cdidata.h"

bool createJSON(struct EEPROM_Data* eed, char* configJSON);
bool parseJSON(char* configJSON, struct EEPROM_Data* eed); 
void encodeEventId(char* buf, uint64_t* eventId);
uint64_t decodeEventId(char* eventIdHex);
bool readConfig(struct EEPROM_Data* eed);
bool writeConfig(struct EEPROM_Data* eed);

#endif
