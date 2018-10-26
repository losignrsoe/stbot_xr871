#
# Rules for building application
#

# ----------------------------------------------------------------------------
# project local config
# ----------------------------------------------------------------------------
include localconfig.mk

# ----------------------------------------------------------------------------
# common rules
# ----------------------------------------------------------------------------
ROOT_PATH := ../../..

include $(ROOT_PATH)/gcc.mk

# ----------------------------------------------------------------------------
# project and objects
# ----------------------------------------------------------------------------
PROJECT := tuya_storybot
APP_BIN_NAME ?= xr871_storybot
USER_SW_VER ?= 9.9.9
$(warning $(APP_BIN_NAME) $(USER_SW_VER))

INCLUDE_PATHS += -I$(ROOT_PATH)/project/$(PROJECT)
INCLUDE_PATHS += -I$(ROOT_PATH)/project/$(PROJECT)/st_inc
INCLUDE_PATHS += -I$(ROOT_PATH)/project/$(PROJECT)/st_inc/arch
INCLUDE_PATHS += -I$(ROOT_PATH)/project/$(PROJECT)/st_inc/func
INCLUDE_PATHS += -I$(ROOT_PATH)/project/$(PROJECT)/st_inc/scene

INCLUDE_PATHS += -I$(ROOT_PATH)/project/$(PROJECT)/tuya_user/$(APP_BIN_NAME)/include
INCLUDE_PATHS += -I$(ROOT_PATH)/project/$(PROJECT)/tuya_common/include
INCLUDE_PATHS += -I$(ROOT_PATH)/project/$(PROJECT)/tuya_common/include/driver
INCLUDE_PATHS += -I$(ROOT_PATH)/project/$(PROJECT)/

ROOT_DIR = $(shell pwd)/../../../src
include $(ROOT_DIR)/tuya_iot/build/build_param
INCLUDE_PATHS += $(TY_IOT_INCLUDE)

CC_FLAGS += $(TY_IOT_CFLAGS)
CC_FLAGS += \
	-DAPP_BIN_NAME=\"$(APP_BIN_NAME)\" \
	-DUSER_SW_VER=\"$(USER_SW_VER)\"

DIRS_IGNORE := ../gcc% ../image% $(ROOT_PATH)/project/common/board/%
DIRS_ALL := $(shell find $(ROOT_PATH)/project/common -type d)
DIRS := $(filter-out $(DIRS_IGNORE),$(DIRS_ALL))
DIRS += $(ROOT_PATH)/project/common/board/$(__PRJ_CONFIG_BOARD)

DIRS += $(ROOT_PATH)/project/$(PROJECT)/tuya_user/$(APP_BIN_NAME)/src
DIRS += $(ROOT_PATH)/project/$(PROJECT)/tuya_common/src/driver
DIRS += $(ROOT_PATH)/project/$(PROJECT)/tuya_common/src
DIRS += $(ROOT_PATH)/project/$(PROJECT)/

DIRS += $(ROOT_PATH)/project/$(PROJECT)/st_src/arch
DIRS += $(ROOT_PATH)/project/$(PROJECT)/st_src/duer
DIRS += $(ROOT_PATH)/project/$(PROJECT)/st_src/func
DIRS += $(ROOT_PATH)/project/$(PROJECT)/st_src/scene
DIRS += $(ROOT_PATH)/project/$(PROJECT)/st_src/

SRCS := $(basename $(foreach dir,$(DIRS),$(wildcard $(dir)/*.[csS])))

OBJS := $(addsuffix .o,$(SRCS))

# extra libs
PRJ_EXTRA_LIBS := -ltuya_iot_lib -lduer-device
# ----------------------------------------------------------------------------
# override project variables
# ----------------------------------------------------------------------------
# linker script path/file
#   - relative to "./"
#   - define your own "LINKER_SCRIPT_PATH" and/or "LINKER_SCRIPT" to override
#     the default one
LINKER_SCRIPT_PATH := .
LINKER_SCRIPT := ./appos_xip_max.ld

# image config path/file
#   - relative to "../image/xxxxx/", eg. "../image/xr871/"
#   - define your own "IMAGE_CFG_PATH" and/or "IMAGE_CFG" to override the
#     default one
IMAGE_CFG_PATH := .
IMAGE_CFG := ./../../gcc/image_sta_ap_xip.cfg

# image name, default to xr_system
# IMAGE_NAME :=

# project make rules
include $(PRJ_MAKE_RULES)
