#
# project local config options, override the common config options
#

# ----------------------------------------------------------------------------
# board definition
# ----------------------------------------------------------------------------
# xr871_evb_main	xr871_tuya_storybot	xr871_evb_audio
__PRJ_CONFIG_BOARD := xr871_tuya_storybot

# ----------------------------------------------------------------------------
# override global config options
# ----------------------------------------------------------------------------
# set y to enable bootloader and disable some features, for bootloader only
# export __CONFIG_BOOTLOADER := y

# set n to disable dual core features, for bootloader only
# export __CONFIG_ARCH_DUAL_CORE := n

# ----------------------------------------------------------------------------
# override project common config options
# ----------------------------------------------------------------------------
# support both sta and ap, default to n
__PRJ_CONFIG_WLAN_STA_AP := y

# support xplayer, default to n
__PRJ_CONFIG_XPLAYER := y

# enable XIP, default to n
__PRJ_CONFIG_XIP := y

ifeq ($(__PRJ_CONFIG_XIP), y)
export __CONFIG_XIP_SECTION_FUNC_LEVEL := y
endif

# enable OTA, default to n
__PRJ_CONFIG_OTA := y

# enable image compress
# __PRJ_CONFIG_IMG_COMPRESS := y
ifeq ($(__PRJ_CONFIG_IMG_COMPRESS), y)
export __CONFIG_BIN_COMPRESS := y
endif