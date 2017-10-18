/*
 * SPIFFS.c
 *
 * Copyright 2017 Otto Schreibke <oschreibke@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * The license can be found at https://www.gnu.org/licenses/gpl-3.0.txt
 *
 *
 */

#include "espressif/esp_common.h"
#include "esp_spiffs.h"

#include "SPIFFS.h"
#include "spiffs_params.h"
#include "cdidata.h"



bool read_spiffs(char* fileName, char* serverName, int bufSize) {

    // open it
    spiffs_file fd = SPIFFS_open(&fs, fileName, SPIFFS_RDWR, 0);
    if (fd < 0) {
        printf("%s: Open Read: errno %i\n", __func__, SPIFFS_errno(&fs));
        return false;
    }

    // read it
    if (SPIFFS_read(&fs, fd, serverName, bufSize) < 0) {
        printf("%s: Read: errno %i\n", __func__, SPIFFS_errno(&fs));
        return false;
    }
    // close it
    if (SPIFFS_close(&fs, fd) < 0) {
        printf("%s: errno %i\n", __func__, SPIFFS_errno(&fs));
        return false;
    }

    return true;
}


bool write_spiffs(char* fileName, char* serverName) {

    // create a file, delete previous if it already exists, and open it for reading and writing
    spiffs_file fd = SPIFFS_open(&fs, fileName, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
    if (fd < 0) {
        printf("%s: Open write: errno %i\n", __func__, SPIFFS_errno(&fs));
        return false;
    }
    // write to it
    if (SPIFFS_write(&fs, fd, serverName, strlen(serverName) + 1) < 0) {
        printf("%s: write: errno %i\n", __func__, SPIFFS_errno(&fs));
        return false;
    }
    // close it
    if (SPIFFS_close(&fs, fd) < 0) {
        printf("%s: Write Close: errno %i\n", __func__, SPIFFS_errno(&fs));
        return false;
    }
    return true;
}


void fs_info() {
    uint32_t total, used;
    SPIFFS_info(&fs, &total, &used);
    printf("%s: Total: %d bytes, used: %d bytes", __func__, total, used);
}

void decode_spiffs_err(int err) {
    switch (err) {
    case SPIFFS_OK:
        printf("SPIFFS_OK\n");
        break;

    case SPIFFS_ERR_NOT_MOUNTED:
        printf("SPIFFS_ERR_NOT_MOUNTED\n");
        break;

    case SPIFFS_ERR_FULL:
        printf("SPIFFS_ERR_FULL\n");
        break;

    case SPIFFS_ERR_NOT_FOUND:
        printf("SPIFFS_ERR_NOT_FOUND\n");
        break;

    case SPIFFS_ERR_END_OF_OBJECT:
        printf("SPIFFS_ERR_END_OF_OBJECT\n");
        break;

    case SPIFFS_ERR_DELETED:
        printf("SPIFFS_ERR_DELETED\n");
        break;

    case SPIFFS_ERR_NOT_FINALIZED:
        printf("SPIFFS_ERR_NOT_FINALIZED\n");
        break;

    case SPIFFS_ERR_NOT_INDEX:
        printf("SPIFFS_ERR_NOT_INDEX\n");
        break;

    case SPIFFS_ERR_OUT_OF_FILE_DESCS:
        printf("SPIFFS_ERR_OUT_OF_FILE_DESCS\n");
        break;

    case SPIFFS_ERR_FILE_CLOSED:
        printf("SPIFFS_ERR_FILE_CLOSED\n");
        break;

    case SPIFFS_ERR_FILE_DELETED:
        printf("SPIFFS_ERR_FILE_DELETED\n");
        break;

    case SPIFFS_ERR_BAD_DESCRIPTOR:
        printf("SPIFFS_ERR_BAD_DESCRIPTOR\n");
        break;

    case SPIFFS_ERR_IS_INDEX:
        printf("SPIFFS_ERR_IS_INDEX\n");
        break;

    case SPIFFS_ERR_IS_FREE:
        printf("SPIFFS_ERR_IS_FREE\n");
        break;

    case SPIFFS_ERR_INDEX_SPAN_MISMATCH:
        printf("SPIFFS_ERR_INDEX_SPAN_MISMATCH\n");
        break;

    case SPIFFS_ERR_DATA_SPAN_MISMATCH:
        printf("SPIFFS_ERR_DATA_SPAN_MISMATCH\n");
        break;

    case SPIFFS_ERR_INDEX_REF_FREE:
        printf("SPIFFS_ERR_INDEX_REF_FREE\n");
        break;

    case SPIFFS_ERR_INDEX_REF_LU:
        printf("SPIFFS_ERR_INDEX_REF_LU\n");
        break;

    case SPIFFS_ERR_INDEX_REF_INVALID:
        printf("SPIFFS_ERR_INDEX_REF_INVALID\n");
        break;

    case SPIFFS_ERR_INDEX_FREE:
        printf("SPIFFS_ERR_INDEX_FREE\n");
        break;

    case SPIFFS_ERR_INDEX_LU:
        printf("SPIFFS_ERR_INDEX_LU\n");
        break;

    case SPIFFS_ERR_INDEX_INVALID:
        printf("SPIFFS_ERR_INDEX_INVALID\n");
        break;

    case SPIFFS_ERR_NOT_WRITABLE:
        printf("SPIFFS_ERR_NOT_WRITABLE\n");
        break;

    case SPIFFS_ERR_NOT_READABLE:
        printf("SPIFFS_ERR_NOT_READABLE\n");
        break;

    case SPIFFS_ERR_CONFLICTING_NAME:
        printf("SPIFFS_ERR_CONFLICTING_NAME\n");
        break;

    case SPIFFS_ERR_NOT_CONFIGURED:
        printf("SPIFFS_ERR_NOT_CONFIGURED\n");
        break;

    case SPIFFS_ERR_NOT_A_FS:
        printf("SPIFFS_ERR_NOT_A_FS\n");
        break;

    case SPIFFS_ERR_MOUNTED:
        printf("SPIFFS_ERR_MOUNTED\n");
        break;

    case SPIFFS_ERR_ERASE_FAIL:
        printf("SPIFFS_ERR_ERASE_FAIL\n");
        break;

    case SPIFFS_ERR_MAGIC_NOT_POSSIBLE:
        printf("SPIFFS_ERR_MAGIC_NOT_POSSIBLE\n");
        break;

    case SPIFFS_ERR_NO_DELETED_BLOCKS:
        printf("SPIFFS_ERR_NO_DELETED_BLOCKS\n");
        break;

    case SPIFFS_ERR_INTERNAL:
        printf("SPIFFS_ERR_INTERNAL\n");
        break;

    case SPIFFS_ERR_TEST:
        printf("SPIFFS_ERR_TEST\n");
        break;

    default:
        printf("Unknown spiffs error %d.\n", err);
        break;
    }
}



/*
bool readConfig(struct EEPROM_Data* eed) {
    spiffs_stat fileStat;
    int rc = 0;
    
    char fotaServerName[50];



    if (esp_spiffs_mount() != SPIFFS_OK) {
        printf("%s: Error mount SPIFFS\n", __func__);
        decode_spiffs_err(fs.err_code);
        return false;
    }

        rc = SPIFFS_stat(&fs, "NodeConfig", &fileStat);

        if (rc != SPIFFS_OK) {
            printf("%s: Stat NodeConfig failed: creating file NodeConfig. ", __func__);
            if (write_spiffs("NodeConfig", "")) {
                printf("Write OK\n");
            } else {
                printf("spiffs write returned rc = %d. fs.err_code = %d.\n", rc, fs.err_code);
                decode_spiffs_err(fs.err_code);
                rc = fs.err_code;
                return false;
            }
        } else {
            printf("%s Stat FOTAServer OK - file exists\n", __func__);
        }
        
            if (read_spiffs("NodeConfig", fotaServerName, 50)) {
                printf("%s: Read success\n", __func__);
            } else {
                printf("%s: Spiffs read returned rc = %d. fs.err_code = %d.\n", __func__, rc, fs.err_code);
                decode_spiffs_err(fs.err_code);
                return false;
            }
            
    return true;
}
*/
