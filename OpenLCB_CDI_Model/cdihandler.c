#include "espressif/esp_common.h"
#include <string.h>
#include "esp_spiffs.h"

#include "cdidata.h"
#include "cdihandler.h"
#include "cJSON.h"
#include "SPIFFS.h"

bool createJSON(struct EEPROM_Data* eed, char* configJSON) {
    //    strcpy(configJSON, "{ dummy: true } \n");

    cJSON *root;
    cJSON *events;
    cJSON *event;

    char charBuf[20];

    root = cJSON_CreateObject();
    //itoa(eed->header.serial, charBuf, 20);
    sprintf(charBuf, "%d", eed->header.serial);
    cJSON_AddStringToObject(root, "serial", charBuf);
    cJSON_AddStringToObject(root, "userName", eed->header.userName);
    cJSON_AddStringToObject(root, "userDescription", eed->header.userDescription);
    cJSON_AddItemToObject(root, "events", events = cJSON_CreateArray());
    for (int i = 0 ; i < 20; i++) {
        if (eed->event[i].eventId != 0) {
            cJSON_AddItemToArray(events, event = cJSON_CreateObject());
            //sprintf(charBuf, "%lld", eed->event[i].eventId);
            encodeEventId(charBuf, &eed->event[i].eventId);
            cJSON_AddStringToObject(event, "eventId", charBuf);
            cJSON_AddStringToObject(event, "eventName", eed->event[i].eventName);
            sprintf(charBuf, "%d", eed->event[i].inputOutput);
            cJSON_AddStringToObject(event, "inputOutput", charBuf);
            sprintf(charBuf, "%d", eed->event[i].address);
            cJSON_AddStringToObject(event, "address", charBuf);
            sprintf(charBuf, "%d", eed->event[i].eventValue);
            cJSON_AddStringToObject(event, "eventValue", charBuf);
        }
    }

    char *rendered = cJSON_Print(root);
    strcpy(configJSON, rendered);
    free(rendered);
    cJSON_Delete(root);

    return true;
}


bool parseJSON(char* configJSON, struct EEPROM_Data* eed) {
    uint32_t tmp;
    cJSON * root;
    cJSON * events;
    cJSON * event;
    cJSON * item;

    //char  * pTmp;

    printf("%s: entered\n", __func__);
    //printf("eed:  %p\n", eed );

    root = cJSON_Parse(configJSON);

    if (root != NULL) {
        printf("%s: parsed configJSON OK\n", __func__);
    } else {
        printf("%s: failed to parse configJSON\n", __func__);
        return false;
    }

    //printf("%s getting 'serial'\n", __func__);
    item = cJSON_GetObjectItem(root, "serial");
    if (item != NULL) {
        sscanf(item->valuestring, "%d", &tmp);
        eed->header.serial = tmp;
        //printf("%s serial set to %d\n", __func__, eed->header.serial);
    } else {
        printf("%s: couldn't retrieve item 'serial'\n", __func__);
        return false;
    }

    //printf("%s getting 'userName'\n", __func__);
    item = cJSON_GetObjectItem(root, "userName");
    if (item != NULL) {
        strcpy(&eed->header.userName[0], item->valuestring);
        //printf("%s userName set to %s\n", __func__, eed->header.userName);
    } else {
        printf("%s: couldn't retrieve item 'userName'\n", __func__);
        return false;
    }


    //printf("%s getting 'userDescription'\n", __func__);
    item = cJSON_GetObjectItem(root, "userDescription");
    if (item != NULL) {
        strcpy(&eed->header.userDescription[0], item->valuestring);
        //printf("%s userDescription set to %s\n", __func__, eed->header.userDescription);
    } else {
        printf("%s: couldn't retrieve item 'userDescription'\n", __func__);
        return false;
    }

    //printf("%s getting 'events'\n", __func__);
    events = cJSON_GetObjectItem(root, "events");

    if (item != NULL) {

        for (uint32_t i = 0; i < cJSON_GetArraySize(events); i++) {
            event = cJSON_GetArrayItem(events, i);

            //printf("%s getting 'eventId[%d]'\n", __func__, i);
            if (item != NULL) {
                item = cJSON_GetObjectItem(event, "eventId");
                //sscanf(item->valuestring, "%d", &tmp);
                //eed->header.serial = tmp;
                eed->event[i].eventId = decodeEventId(item->valuestring);
                //printf("%s eventId[%d] set to %d\n", __func__, i, eed->event[i].eventId);
            } else {
                printf("%s: couldn't retrieve item 'eventId'\n", __func__);
                return false;
            }

            //printf("%s getting 'eventName'[%d]'\n", __func__, i);
            item = cJSON_GetObjectItem(event, "eventName");
            if (item != NULL) {
                strcpy(&eed->event[i].eventName[0], item->valuestring);
                //printf("%s eventName[%d] set to %s\n", __func__, i, eed->event[i].eventName);
            } else {
                printf("%s: couldn't retrieve item 'eventName'\n", __func__);
                return false;
            }

            //printf("%s getting 'inputOutput'[%d]'\n", __func__, i);
            item = cJSON_GetObjectItem(event, "inputOutput");
            if (item != NULL) {
                sscanf(item->valuestring, "%d", &tmp);
                eed->event[i].inputOutput = tmp;
                //printf("%s inputOutput[%d] set to %d\n", __func__, i, eed->event[i].inputOutput);
            } else {
                printf("%s: couldn't retrieve item 'inputOutput'\n", __func__);
                return false;
            }

            //printf("%s getting 'address'[%d]'\n", __func__, i);
            item = cJSON_GetObjectItem(event, "address");
            if (item != NULL) {
                sscanf(item->valuestring, "%d", &tmp);
                eed->event[i].address = tmp;
                //printf("%s address[%d] set to %d\n", __func__, i, eed->event[i].address);
            } else {
                printf("%s: couldn't retrieve item 'address'\n", __func__);
                return false;
            }

            //printf("%s getting 'eventValue'[%d]'\n", __func__, i);
            item = cJSON_GetObjectItem(event, "eventValue");
            if (item != NULL) {
                sscanf(item->valuestring, "%d", &tmp);
                eed->event[i].eventValue = tmp;
                //printf("%s eventValue[%d] set to %d\n", __func__, i, eed->event[i].eventValue);
            } else {
                printf("%s: couldn't retrieve item 'serial'\n", __func__);
                return false;
            }
        }
    } else {
        printf("%s: couldn't retrieve item array 'events'\n", __func__);
        return false;
    }


    cJSON_Delete(root);

    printf("%s: leaving\n", __func__);
    return true;
}

void encodeEventId(char* buf, uint64_t* eventId) {
    // encode a 64-bit event id to a hex string
    // buf must have (at least) enough space for 17 characters;

    // the event id is held in the configuration data in big-endian format for compatability with JMRI
    // process the bytes in reverse order

    char hexDigits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    char* pBuf = buf + 15;
    *(pBuf + 1) = '\0';

    for (uint8_t i = 0; i < 8; i++) {
        *pBuf = hexDigits[(*eventId >> ((8 - i) + 1 * 4)) & 0x0F];
        *pBuf = hexDigits[(*eventId >> ((8 - i)     * 4)) & 0x0F];
        pBuf--;
    }
}

uint64_t decodeEventId(char* eventIdHex) {
    // convert an uppercase 16-character hex string to uint64
    
    // the event id is held in the configuration data in big-endian format for compatability with JMRI
    // process the bytes in reverse order
        
    char hexDigits[17];
    uint64_t eventId = 0;
    char ch = '\0';
    uint8_t strPos = 0;
    strcpy((char*)&hexDigits, "0123456789ABCDEF");

    //printf("%s eventIdHex(%d): %s \n", __func__, strlen(eventIdHex), eventIdHex);

    for (int8_t i = 7; i >= 0; i--) {
        ch = *(eventIdHex + (i * 2));
        for (strPos = 0; ch != hexDigits[strPos] && strPos <= strlen(eventIdHex); strPos++ ) {}
        //printf("%s: ch %c, strPos %d\n", __func__, ch, strPos);
        eventId = (eventId << 4) + strPos;
        ch = *(eventIdHex + (i * 2) + 1);
        for (strPos = 0; ch != hexDigits[strPos] && strPos <= strlen(eventIdHex); strPos++ ) {}
        //printf("%s: ch %c, strPos %d\n", __func__, ch, strPos);
        eventId = (eventId << 4) + strPos;
    }
    return eventId;
}


bool readConfig(struct EEPROM_Data* eed) {
    spiffs_stat fileStat;
    int rc = 0;

    char* configJSON;

    configJSON = malloc(4096);
    memset(configJSON, '\0', 4096);

    if (esp_spiffs_mount() != SPIFFS_OK) {
        printf("%s: Error mount SPIFFS\n", __func__);
        decode_spiffs_err(fs.err_code);
        return false;
    }

    rc = SPIFFS_stat(&fs, "NodeConfig", &fileStat);

    if (rc != SPIFFS_OK) {
        printf("%s: File 'NodeConfig' not found.\n", __func__);
        return false;
    }

    if (read_spiffs("NodeConfig", configJSON, 4096)) {
        printf("%s: Read success\n", __func__);
    } else {
        printf("%s: Spiffs read returned rc = %d. fs.err_code = %d.\n", __func__, rc, fs.err_code);
        decode_spiffs_err(fs.err_code);
        return false;
    }

    printf("%s: configJSON %s\n", __func__, configJSON);

    memset(eed, '\0', sizeof(struct EEPROM_Data));
    parseJSON(configJSON, eed);

    SPIFFS_unmount(&fs);

    return true;
}

bool writeConfig(struct EEPROM_Data* eed) {

    int rc = 0;

    char* configJSON;

    configJSON = malloc(4096);
    memset(configJSON, '\0', 4096);

    if (esp_spiffs_mount() != SPIFFS_OK) {
        printf("%s: Error mount SPIFFS\n", __func__);
        decode_spiffs_err(fs.err_code);
        return false;
    }

    createJSON(eed, configJSON);
    printf("%s: configJSON %s\n", __func__, configJSON);


    if (write_spiffs("NodeConfig", configJSON)) {
        printf("Write OK\n");
    } else {
        printf("spiffs write NodeConfig returned rc = %d. fs.err_code = %d.\n", rc, fs.err_code);
        decode_spiffs_err(fs.err_code);
        rc = fs.err_code;
        return false;
    }


    SPIFFS_unmount(&fs);

    return true;
}

