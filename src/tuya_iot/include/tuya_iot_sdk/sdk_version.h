/***********************************************************
*  File: sdk_version.h 
*  Author: 
*  Date: 
***********************************************************/
#ifndef _SDK_VERSION_H
#define _SDK_VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

#define BS_VER "11.02" //启用CID机制，单品/网关/网关设备 3IN1版本sdk
#define PT_VER "2.2"
#define LAN_PRO_VER "3.2"
#define CAD_VER "1.0.0" //启用CID机制，固件升级启用4.1
#define CD_VER "1.0.0" //固件支持mqtt消息确认

#define INT2STR(NUM) #NUM
#define I2S(R) INT2STR(R)

#define SDK_BUILD_INFO_STR "TUYA_IOT_SDK V"IOT_SDK_VER" FOR "TARGET_PLATFORM\
" BUILD AT "BUILD_DATE"_"BUILD_TIME\
" D:"I2S(DEBUG)\
" S:"I2S(HTTPC_SECURE_MBEDTLS)\
" W:"I2S(WIFI_GW)

#define SDK_INFO "< "SDK_BUILD_INFO_STR" V:"BS_VER"_"PT_VER"_"LAN_PRO_VER" >"


#ifdef __cplusplus
}
#endif
#endif /*_SDK_VERSION_H*/

