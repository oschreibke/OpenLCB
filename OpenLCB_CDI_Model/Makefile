PROGRAM=openlcbcdi
EXTRA_COMPONENTS=extras/rboot-ota extras/spiffs
# extras/mbedtls extras/cpp_support 
#INCLUDE_vTaskSuspend=1
FLASH_SIZE = 32

# spiffs configuration
SPIFFS_BASE_ADDR = 0x300000
SPIFFS_SIZE = 0x010000

#SPIFFS_SINGLETON = 0  # for run-time configuration

#SPIFFS overrides
SPIFFS_USE_MAGIC=(1)
SPIFFS_USE_MAGIC_LENGTH=1
SPIFFS_SINGLETON=1
SPIFFS_TEST_VISUALISATION=1
SPIFFS_CACHE=(1)
SPIFFS_CACHE_WR=(1)
SPIFFS_TEMPORAL_FD_CACHE=(1)

LOG_PAGE_SIZE=256

include ../../common.mk

$(eval $(call make_spiffs_image, files))
