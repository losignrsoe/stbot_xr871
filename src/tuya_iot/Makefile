#
# Rules for building library
#

# ----------------------------------------------------------------------------
# common rules
# ----------------------------------------------------------------------------

ROOT_PATH := ../..
include $(ROOT_PATH)/gcc.mk

ROOT_DIR = $(shell pwd)/..
include build/build_param 
CC_FLAGS += $(TY_IOT_CFLAGS)

INCLUDE_PATHS += $(TY_IOT_INCLUDE)
INCLUDE_PATHS += -I$(ROOT_PATH)/project

APP_BIN_NAME ?= esp32_sample
USER_SW_VER ?= 9.9.9

CC_FLAGS += \
	-DAPP_BIN_NAME=\"$(APP_BIN_NAME)\" \
	-DUSER_SW_VER=\"$(USER_SW_VER)\" 
	
 

# ----------------------------------------------------------------------------
# library and objects
# ----------------------------------------------------------------------------
LIBS := lib/libtuya_iot_lib.a

#DIRS := $(shell find . -type d)

#SRCS := $(basename $(foreach dir,$(DIRS),$(wildcard $(dir)/*.[csS])))

#OBJS := $(addsuffix .o,$(SRCS))
OBJS := $(TY_OBJS)
# library make rules
include $(LIB_MAKE_RULES)
