/***********************************************************
*  File: wifi_hwl.h
*  Author: nzy
*  Date: 20170914
***********************************************************/
#ifndef _WIFI_HWL_H
#define _WIFI_HWL_H

#include "tuya_cloud_types.h"
#include "wf_basic_intf.h"
#include "uni_semaphore.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIFI_HWL_GLOBAL
    #define _WIFI_HWL_EXT
#else
    #define _WIFI_HWL_EXT extern
#endif

/***********************************************************
*************************micro define***********************
***********************************************************/
typedef struct {
    AP_IF_S *ap_if;
    BYTE_T ap_if_nums;
    BYTE_T ap_if_count;
    SEM_HANDLE sem_handle;
}SACN_AP_RESULT_S;

typedef enum xr_promisc_t {
	XR_PROMISC_DISABLE,
	XR_PROMISC_ENABLE
}xr_promisc_t;

#define SCAN_MAX_AP 64
//#define MAX_SCAN_RESULTS 10
#define MAX_SCAN_RESULTS SCAN_MAX_AP
#define UNVALID_SIGNAL -127
/***********************************************************
*************************variable define********************
***********************************************************/
extern struct netif *g_wlan_netif;
/***********************************************************
*************************function define********************
***********************************************************/
/***********************************************************
*  Function: hwl_wf_all_ap_scan
*  Input: none
*  Output: ap_ary num
*  Return: OPERATE_RET
***********************************************************/
_WIFI_HWL_EXT \
OPERATE_RET hwl_wf_all_ap_scan(OUT AP_IF_S **ap_ary,OUT UINT_T *num);

/***********************************************************
*  Function: hwl_wf_assign_ap_scan
*  Input: ssid
*  Output: ap
*  Return: OPERATE_RET
***********************************************************/
_WIFI_HWL_EXT \
OPERATE_RET hwl_wf_assign_ap_scan(IN CONST CHAR_T *ssid,OUT AP_IF_S **ap);

/***********************************************************
*  Function: hwl_wf_release_ap
*  Input: ap
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
_WIFI_HWL_EXT \
OPERATE_RET hwl_wf_release_ap(IN AP_IF_S *ap);

/***********************************************************
*  Function: hwl_wf_set_cur_channel
*  Input: chan
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
_WIFI_HWL_EXT \
OPERATE_RET hwl_wf_set_cur_channel(IN CONST BYTE_T chan);

/***********************************************************
*  Function: hwl_wf_get_cur_channel
*  Input: none
*  Output: chan
*  Return: OPERATE_RET
***********************************************************/
_WIFI_HWL_EXT \
OPERATE_RET hwl_wf_get_cur_channel(OUT BYTE_T *chan);

/***********************************************************
*  Function: hwl_wf_sniffer_set
*  Input: en cb
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
_WIFI_HWL_EXT \
OPERATE_RET hwl_wf_sniffer_set(IN CONST BOOL_T en,IN CONST SNIFFER_CALLBACK cb);

/***********************************************************
*  Function: hwl_wf_get_ip
*  Input: wf
*  Output: ip
*  Return: OPERATE_RET
***********************************************************/
_WIFI_HWL_EXT \
OPERATE_RET hwl_wf_get_ip(IN CONST WF_IF_E wf,OUT NW_IP_S *ip);

/***********************************************************
*  Function: hwl_wf_set_ip
*  Input: wf
*  Output: ip
*  Return: OPERATE_RET
***********************************************************/
_WIFI_HWL_EXT \
OPERATE_RET hwl_wf_set_ip(IN CONST WF_IF_E wf,IN CONST NW_IP_S *ip);

/***********************************************************
*  Function: hwl_wf_get_mac
*  Input: wf
*  Output: mac
*  Return: OPERATE_RET
***********************************************************/
_WIFI_HWL_EXT \
OPERATE_RET hwl_wf_get_mac(IN CONST WF_IF_E wf,OUT NW_MAC_S *mac);

/***********************************************************
*  Function: hwl_wf_set_mac
*  Input: wf mac
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
_WIFI_HWL_EXT \
OPERATE_RET hwl_wf_set_mac(IN CONST WF_IF_E wf,IN CONST NW_MAC_S *mac);

/***********************************************************
*  Function: hwl_wf_wk_mode_set
*  Input: mode
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
_WIFI_HWL_EXT \
OPERATE_RET hwl_wf_wk_mode_set(IN CONST WF_WK_MD_E mode);

/***********************************************************
*  Function: hwl_wf_wk_mode_get
*  Input: none
*  Output: mode
*  Return: OPERATE_RET
***********************************************************/
_WIFI_HWL_EXT \
OPERATE_RET hwl_wf_wk_mode_get(OUT WF_WK_MD_E *mode);

/***********************************************************
*  Function: hwl_wf_station_connect
*  Input: ssid passwd
*  Output: mode
*  Return: OPERATE_RET
***********************************************************/
_WIFI_HWL_EXT \
OPERATE_RET hwl_wf_station_connect(IN CONST CHAR_T *ssid,IN CONST CHAR_T *passwd);

/***********************************************************
*  Function: hwl_wf_station_disconnect
*  Input: none
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
_WIFI_HWL_EXT \
OPERATE_RET hwl_wf_station_disconnect(VOID);

/***********************************************************
*  Function: hwl_wf_station_get_conn_ap_rssi
*  Input: none
*  Output: rssi
*  Return: OPERATE_RET
***********************************************************/
_WIFI_HWL_EXT \
OPERATE_RET hwl_wf_station_get_conn_ap_rssi(OUT SCHAR_T *rssi);

/***********************************************************
*  Function: hwl_wf_station_stat_get
*  Input: none
*  Output: stat
*  Return: OPERATE_RET
***********************************************************/
_WIFI_HWL_EXT \
OPERATE_RET hwl_wf_station_stat_get(OUT WF_STATION_STAT_E *stat);

/***********************************************************
*  Function: hwl_wf_ap_start
*  Input: cfg
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
_WIFI_HWL_EXT \
OPERATE_RET hwl_wf_ap_start(IN CONST WF_AP_CFG_IF_S *cfg);

/***********************************************************
*  Function: hwl_wf_ap_stop
*  Input: none
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
_WIFI_HWL_EXT \
OPERATE_RET hwl_wf_ap_stop(VOID);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif



