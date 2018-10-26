/*!
\file tuya_cloud_wifi_defs.h
Copyright(C),2017, 涂鸦科技 www.tuya.comm
*/

#ifndef TUYA_CLOUD_WIFI_DEFS_H
#define TUYA_CLOUD_WIFI_DEFS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_cloud_types.h"

typedef BYTE_T GW_WF_CFG_MTHD_SEL; // wifi config method select
#define GWCM_OLD 0 // do not have low power mode
#define GWCM_LOW_POWER 1 // with low power mode
#define GWCM_SPCL_MODE 2 // special with low power mode
#define GWCM_OLD_PROD  3 // GWCM_OLD mode with product



typedef BYTE_T GW_WF_NWC_STAT_T; // gateway wifi network config status
#define GWNS_LOWPOWER 0
#define GWNS_UNCFG_SMC 1
#define GWNS_UNCFG_AP 2
#define GWNS_TY_SMARTCFG 3
#define GWNS_TY_AP 4
#define GWNS_WECHAT_AK 5
#define GWNS_OTHER_CFG 6 // for example:uart、bluetooth、and so on
#define GWNS_NO_NEED_CFG 7 // no need config wifi

// wifi network status
typedef BYTE_T GW_WIFI_NW_STAT_E;
#define STAT_LOW_POWER 0 // idle status,use to external config network
#define STAT_UNPROVISION 1 // smart config status
#define STAT_AP_STA_UNCFG 2 // ap WIFI config status
#define STAT_AP_STA_DISC 3 // ap WIFI already config,station disconnect
#define STAT_AP_STA_CONN 4 // ap station mode,station connect
#define STAT_STA_DISC 5 // only station mode,disconnect
#define STAT_STA_CONN 6 // station mode connect
#define STAT_CLOUD_CONN 7 // cloud connect
#define STAT_AP_CLOUD_CONN 8 // cloud connect and ap start

typedef BYTE_T WF_RESET_TP_T;
#define WRT_SMT_CFG 0
#define WRT_AP 1
#define WRT_AUTO 2

typedef struct {
    CHAR_T *uuid; // strlen(uuid) <= 16,must not be null
    CHAR_T *auth_key; // strlen(auth_key) <= 32,must not be null
    CHAR_T *ap_ssid; // strlen(ap_ssid) <= 16,if ap_ssid is null ,then the default ssid is Smartlife_xxxx
    CHAR_T *ap_passwd; // strlen(ap_passwd) <= 16,default null
}WF_GW_PROD_INFO_S;

typedef VOID (*GET_WF_NW_STAT_CB)(IN CONST GW_WIFI_NW_STAT_E stat);


#ifdef __cplusplus
}
#endif

#endif // TUYA_CLOUD_DEV_CTL_H
