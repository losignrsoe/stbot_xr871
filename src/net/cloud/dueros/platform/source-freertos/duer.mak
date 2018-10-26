#
# Copyright (2017) Baidu Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

include $(CLEAR_VAR)

MODULE_PATH := $(BASE_DIR)/platform/source-freertos

LOCAL_MODULE := port-freertos

LOCAL_STATIC_LIBRARIES := framework cjson connagent coap voice_engine platform device_status Device_Info

LOCAL_SRC_FILES := $(wildcard $(MODULE_PATH)/*.c)

LOCAL_INCLUDES :=

ifeq ($(strip $(CUSTOMER)),mw300)
LOCAL_INCLUDES += \
	$(MODULE_PATH)/include-mw300 \
	$(MODULE_PATH)/include-mw300/freertos \
	$(MODULE_PATH)/include-mw300/port \
	$(MODULE_PATH)/include-mw300/ipv4 \
	$(MODULE_PATH)/include-mw300/ipv6
else ifeq ($(strip $(CUSTOMER)),esp32)
LOCAL_INCLUDES += \
	$(MODULE_PATH)/include-esp32 \
	$(IDF_PATH)/components/freertos/include \
	$(IDF_PATH)/components/esp32/include \
	$(IDF_PATH)/components/log/include \
	$(IDF_PATH)/components/heap/include \
	$(IDF_PATH)/components/driver/include \
	$(IDF_PATH)/components/vfs/include \
	$(IDF_PATH)/components/soc/include \
	$(IDF_PATH)/components/soc/esp32/include \
	$(IDF_PATH)/components/lwip/include/lwip \
	$(IDF_PATH)/components/lwip/include/lwip/port \
	$(IDF_PATH)/components/tcpip_adapter/include
else ifeq ($(strip $(CUSTOMER)),esp8266)
LOCAL_INCLUDES += \
    $(SDK_PATH)/include/ \
    $(SDK_PATH)/include/include/freertos/ \
    $(SDK_PATH)/include/espressif/ \
    $(SDK_PATH)/include/lwip/ \
    $(SDK_PATH)/include/lwip/ipv4/ \
    $(SDK_PATH)/include/lwip/ipv6/ \
    $(SDK_PATH)/extra_include/ \
    $(MODULE_PATH)/include-esp8266/
else ifeq ($(strip $(CUSTOMER)),xr871)
LOCAL_INCLUDES += \
	$(SDK_PATH)/include \
    $(SDK_PATH)/include/kernel/os \
    $(SDK_PATH)/include/kernel/FreeRTOS \
    $(SDK_PATH)/include/kernel/FreeRTOS/portable/GCC/ARM_CM4F\
    $(SDK_PATH)/include/net/lwip-1.4.1 \
    $(SDK_PATH)/include/net/lwip-1.4.1/ipv4 \
    $(SDK_PATH)/include/net/lwip-1.4.1/ipv6
endif


include $(BUILD_STATIC_LIB)
