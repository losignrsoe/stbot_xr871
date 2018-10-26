/*!
\file tuya_iot_wifi_api.h
Copyright(C),2017, 涂鸦科技 www.tuya.comm
*/

#ifndef _TUYA_IOT_WIFI_API_H
#define _TUYA_IOT_WIFI_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_cloud_types.h"
#include "tuya_cloud_com_defs.h"
#include "tuya_cloud_wifi_defs.h"
#include "tuya_iot_com_api.h"

#ifdef  __TUYA_IOT_WIFI_API_GLOBALS
    #define __TUYA_IOT_WIFI_API_EXT
#else
    #define __TUYA_IOT_WIFI_API_EXT extern
#endif

/***********************************************************
*************************micro define***********************
***********************************************************/

/***********************************************************
*************************variable define********************
***********************************************************/

/***********************************************************
*************************function define********************
***********************************************************/
/***********************************************************
*  Function: tuya_iot_set_wf_gw_prod_info 
*  Input: wf_prod_info
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
__TUYA_IOT_WIFI_API_EXT \
OPERATE_RET tuya_iot_set_wf_gw_prod_info(IN CONST WF_GW_PROD_INFO_S *wf_prod_info);

/***********************************************************
*  Function: tuya_iot_wf_mcu_dev_init->The device consists of MCU and WiFi
*  Input: cfg
*         cbs->tuya wifi sdk user callbacks
*         product_key->product key/proudct id,get from tuya open platform
*         wf_sw_ver->wifi module software version format:xx.xx.xx (0<=x<=9)
*         mcu_sw_ver->mcu software version format:xx.xx.xx (0<=x<=9)
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
__TUYA_IOT_WIFI_API_EXT \
OPERATE_RET tuya_iot_wf_mcu_dev_init(IN CONST GW_WF_CFG_MTHD_SEL cfg,\
                                                 IN CONST TY_IOT_CBS_S *cbs,IN CONST CHAR_T *product_key,\
                                                 IN CONST CHAR_T *wf_sw_ver,IN CONST CHAR_T *mcu_sw_ver);

/***********************************************************
*  Function: tuya_iot_wf_soc_init->The devcie consists of wifi soc
*  Input: cfg
*         cbs->tuya wifi sdk user callbacks,note cbs->dev_ug_cb is useless
*         product_key->product key/proudct id,get from tuya open platform
*         wf_sw_ver->wifi module software version format:xx.xx.xx (0<=x<=9)
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
__TUYA_IOT_WIFI_API_EXT \
OPERATE_RET tuya_iot_wf_soc_dev_init(IN CONST GW_WF_CFG_MTHD_SEL cfg,\
                                                 IN CONST TY_IOT_CBS_S *cbs,IN CONST CHAR_T *product_key,\
                                                 IN CONST CHAR_T *wf_sw_ver);

/***********************************************************
*  Function: tuya_iot_wf_gw_init->tuya iot entity gateway initialization
*  Input: cfg
*         cbs->tuya wifi sdk user callbacks
*         gw_cbs->tuya gw user callbacks
*         product_key->product key/proudct id,get from tuya open platform
*         wf_sw_ver->wifi module software version format:xx.xx.xx (0<=x<=9)
*         attr->gw attr
*         attr_num
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
__TUYA_IOT_WIFI_API_EXT \
OPERATE_RET tuya_iot_wf_gw_init(IN CONST GW_WF_CFG_MTHD_SEL cfg,IN CONST TY_IOT_CBS_S *cbs,\
                                         IN CONST TY_IOT_GW_CBS_S *gw_cbs,IN CONST CHAR_T *product_key,\
                                         IN CONST CHAR_T *wf_sw_ver,IN CONST GW_ATTACH_ATTR_T *attr,\
                                         IN CONST UINT_T attr_num);

/***********************************************************
*  Function: tuya_iot_wf_gw_dev_init->tuya iot entity gateway initialization
*                                     the gateway make as a device meanwhile
*  Input: cfg
*         cbs->tuya wifi sdk user callbacks
*         gw_cbs->tuya gw user callbacks
*         product_key->product key/proudct id,get from tuya open platform
*         wf_sw_ver->wifi module software version format:xx.xx.xx (0<=x<=9)
*         attr->gw attr
*         attr_num
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
__TUYA_IOT_WIFI_API_EXT \
OPERATE_RET tuya_iot_wf_gw_dev_init(IN CONST GW_WF_CFG_MTHD_SEL cfg,IN CONST TY_IOT_CBS_S *cbs,\
                                               IN CONST TY_IOT_GW_CBS_S *gw_cbs,IN CONST CHAR_T *product_key,\
                                               IN CONST CHAR_T *wf_sw_ver,IN CONST GW_ATTACH_ATTR_T *attr,\
                                               IN CONST UINT_T attr_num);

/***********************************************************
*  Function: tuya_iot_reg_get_wf_nw_stat_cb
*  Input: wf_nw_stat_cb
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
__TUYA_IOT_WIFI_API_EXT \
OPERATE_RET tuya_iot_reg_get_wf_nw_stat_cb(IN CONST GET_WF_NW_STAT_CB wf_nw_stat_cb);

/***********************************************************
*  Function: tuya_iot_wf_gw_unactive
*  Input: none
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
__TUYA_IOT_WIFI_API_EXT \
OPERATE_RET tuya_iot_wf_gw_unactive(IN CONST WF_RESET_TP_T rst_tp);

/***********************************************************
*  Function: tuya_iot_set_user_def_ap_if
*  Input: ssid passwd
*  Output: none
*  Return: OPERATE_RET
*  Note:need call before tuya_iot_wf_xxx_init
***********************************************************/
__TUYA_IOT_WIFI_API_EXT \
OPERATE_RET tuya_iot_set_user_def_ap_if(IN CONST CHAR_T *ssid,IN CONST CHAR_T *passwd);

/***********************************************************
*  Function: tuya_iot_wf_nw_cfg_ap_pri_set
*  Input: ap_pri
*  Output: none
*  Return: none
*  Note: wifi network config ap mode prior,default smartconfig first
*        valid must call before the device is initialized
***********************************************************/
__TUYA_IOT_WIFI_API_EXT \
VOID tuya_iot_wf_nw_cfg_ap_pri_set(IN CONST BOOL_T ap_pri);

/***********************************************************
*  Function: tuya_iot_gw_wf_user_cfg
*  Input: ssid passwd token
*  Output: none
*  Return: OPERATE_RET
*..Note:use to TY_APP_CFG_WF mode
***********************************************************/
__TUYA_IOT_WIFI_API_EXT \
OPERATE_RET tuya_iot_gw_wf_user_cfg(IN CONST CHAR_T *ssid,IN CONST CHAR_T *passwd,IN CONST CHAR_T *token);

#ifdef __cplusplus
}
#endif

#endif  /*_TUYA_IOT_API_H*/

