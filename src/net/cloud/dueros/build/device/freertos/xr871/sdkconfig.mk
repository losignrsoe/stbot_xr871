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

CUSTOMER ?= xr871

COMPILER := arm
TOOLCHAIN ?= ~/tools/gcc-arm-none-eabi-4_9-2015q2/bin/arm-none-eabi-

SDK_PATH = ../../../../$(BASE_DIR)

#if want use xr871-os, should add "__CONFIG_OS_FREERTOS"
COM_DEFS := DUER_PLATFORM_XR871 __CONFIG_OS_FREERTOS
CFLAGS := -Wall -g -Os -MMD -ffunction-sections -fdata-sections -fno-common
CFLAGS += -ffreestanding -mcpu=cortex-m4 -mthumb -DDEBUG_LEVEL=3
CFLAGS += -std=gnu99

# open this if want to use the AES-CBC encrypted communication
#COM_DEFS += NET_TRANS_ENCRYPTED_BY_AES_CBC

#=====start modules select=======#
modules_module_System_Info=y
modules_module_Device_Info=y
modules_module_HTTP=y
modules_module_OTA=y
modules_module_coap=y
modules_module_connagent=y
modules_module_dcs=y
modules_module_ntp=y
modules_module_voice_engine=y
modules_module_device_status=y

platform_module_platform=n
platform_module_port-freertos=y
platform_module_port-linux=n
platform_module_port-lpb100=n

module_framework=y

external_module_mbedtls=y
external_module_nsdl=y
external_module_cjson=y
external_module_speex=y
external_module_Zliblite=y
#=====end  modules select=======#
