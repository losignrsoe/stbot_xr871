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

CUSTOMER ?= esp32

COMPILER := xtensa

TOOLCHAIN ?= xtensa-esp32-elf-

ifeq ($(strip $(IDF_PATH)),)
$(error please config the environment variable IDF_PATH \
        which point to the IDF root directory)
endif

CFLAGS += -DDUER_PLATFORM_ESPRESSIF -DCONFIG_TCP_OVERSIZE_MSS -DFIXED_POINT
CFLAGS += -DDUER_COMPRESS_QUALITY=5 -DDUER_ASSIGN_TASK_NO=0 -DENABLE_ALERT_TONE
CFLAGS += -mlongcalls
CFLAGS += -DESP_PLATFORM
CFLAGS += -DMBEDTLS_CONFIG_FILE=\"baidu_ca_mbedtls_config.h\"

# open this if want to use the AES-CBC encrypted communication
#COM_DEFS += NET_TRANS_ENCRYPTED_BY_AES_CBC

#=====start modules select=======#
modules_module_Device_Info=y
modules_module_System_Info=y
modules_module_HTTP=y
modules_module_OTA=y
modules_module_coap=y
modules_module_connagent=y
modules_module_dcs=y
modules_module_ntp=y
modules_module_voice_engine=y

platform_module_platform=n
platform_module_port-freertos=y
platform_module_port-linux=n
platform_module_port-lpb100=n

module_framework=y

external_module_mbedtls=n
external_module_nsdl=y
external_module_cjson=y
external_module_speex=y
external_module_Zliblite=y
#=====end  modules select=======#
