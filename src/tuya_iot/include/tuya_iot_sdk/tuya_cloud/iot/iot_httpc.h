/***********************************************************
*  File: iot_httpc.h
*  Author: nzy
*  Date: 20150527
***********************************************************/
#ifndef _IOT_HTTPC_H
#define _IOT_HTTPC_H

    #include "tuya_cloud_types.h"
    #include "cJSON.h"
    #include "gw_intf.h"

#ifdef __cplusplus
	extern "C" {
#endif

#ifdef  __IOT_HTTPC_GLOBALS
    #define __IOT_HTTPC_EXT
#else
    #define __IOT_HTTPC_EXT extern
#endif

/***********************************************************
*************************micro define***********************
***********************************************************/
//#define TY_SMART_DOMAIN_AY "http://a.gw.cn.wgine.com/gw.json"
#define TY_SMART_DOMAIN_AY "http://a.gw.tuyacn.com/gw.json"				//China
#define TY_SMART_DOMAIN_AZ "http://a.gw.tuyaus.com/gw.json"					//American
#define TY_SMART_DOMAIN_EU "http://a.gw.tuyaeu.com/gw.json"					//Europe

//#define TY_SMART_MQTT "mq.gw.airtakeapp.com"
//#define TY_SMART_MQTTBAK "mq.gw1.airtakeapp.com"
#define TY_SMART_MQTT "mq.gw.tuyacn.com"
#define TY_SMART_MQTTBAK "mq.gw1.tuyacn.com"
#define	WX_TIMEZONE		"+08:00"

// gw interface
#define TI_GW_GET_URL_CFG "tuya.device.config.get" // get url config
#define TI_GW_REGISTER "tuya.device.register" // register

#define TI_GW_ACTIVE "tuya.device.active" // gw active
#define TI_GW_ACTIVE_41 "tuya.device.active" // gw_active_v41

#define TI_GW_IS_EXIST "tuya.device.exist" // gw is exist
#define TI_GW_RESET "tuya.device.reset" // gw reset
#define TI_UG_RST_LOG "atop.online.debug.log" // ug log

#define TI_GW_IF_UPDATE "tuya.device.update"
#define TI_GW_IF_UPDATE_V41 "tuya.device.versions.update"

#define TI_GW_GET_UPGRADE_IF "tuya.device.upgrade.get" // get gateway upgrade info
#define TI_GW_GET_UPGRADE_IF_V41 "tuya.device.upgrade.get" // get gateway upgrade info

#define TI_GW_UPGRD_STAT_UPDATE "tuya.device.upgrade.status.update"
#define TI_GW_UPGRD_STAT_UPDATE_V41 "tuya.device.upgrade.status.update"

#define TI_GW_GET_EXT_CFG "tuya.device.extension.config.get"

#define TI_DEV_GET_UPGRADE_IF "tuya.device.sub.upgrade.get"
#define TI_DEV_GET_UPGRADE_IF_V41 "tuya.device.upgrade.get"

#define TI_DEV_UPGRADE_STAT_UPDATE "tuya.device.sub.upgrade.status.update"
#define TI_DEV_UPGRADE_STAT_UPDATE_V41 "tuya.device.upgrade.status.update"

#define TI_DEV_IF_UPDATE "tuya.device.sub.update"
#define TI_DEV_IF_UPDATE_V41 "tuya.device.version.update" // dev_if update

#define TI_DEV_DP_REPORT "tuya.device.dp.report"

#define TI_DEV_BIND "tuya.device.sub.bind"
#define TI_DEV_BIND_V41 "tuya.device.sub.bind"

#define TI_DEV_UNBIND "tuya.device.sub.unbind"
#define TI_DEV_ONLINE_REPT "tuya.device.sub.online"
#define TI_DEV_OFFLINE_REPT "tuya.device.sub.offline"
#define TI_GW_GET_TIME "tuya.p.time.get" // time get
#define TI_GET_GW_DEV_TIMER_COUNT "tuya.device.timer.count" //get gw or dev timer count
#define TI_GET_GW_DEV_TIMER "tuya.device.timer.get" //get gw or dev timer schema
#define TI_GET_DEV_LIST "tuya.device.sub.list" //get device list

/* get dueros uuid token version */
#define TI_GET_DUEROS_CONFIG "tuya.device.proxy.config.get"	//add by luowq

// firmware upgrade by gw
#define TI_FW_SELF_UG_INFO "tuya.device.upgrade.silent.get"

#define TI_GW_GET_EXT_CFG "tuya.device.extension.config.get"
#define TI_GW_DYN_CFG_GET "tuya.device.dynamic.config.get"
#define TI_GW_DYN_CFG_ACK "tuya.device.dynamic.config.ack"


typedef struct {
    CHAR_T *url;
    CHAR_T *id;
    CHAR_T *uuid;
    CHAR_T *auth_key;
    CHAR_T *token;
    CHAR_T *sw_ver;
    CHAR_T *dev_sw_ver;
    CHAR_T *pv;
    CHAR_T *bv;
}GW_ACTV_IN_PARM_S;

typedef struct {
    CHAR_T *pk;
    CHAR_T *url;
    CHAR_T *id; // devid
    CHAR_T *uuid;
    CHAR_T *hid;
    CHAR_T *token;
    CHAR_T *sw_ver;
    CHAR_T *pv;
    CHAR_T *bv;
    CHAR_T *cad_ver;
    CHAR_T *cd_ver;
    CHAR_T *modules; // [{"type":3,online:true,"softVer":"1.0"}]
    CHAR_T *feature; // user self define
    CHAR_T *auth_key;
    CHAR_T *options;
    //BOOL_T is_oem;
}GW_ACTV_IN_PARM_V41_S;

/***********************************************************
*   Note:used to get file callback inform
*   devid ->deviceid
*   total_len->file total len
*   offset->file offset
*   data->file data
*   len->data len
*...remain_len->The remaining unused data length,
*               for next callback import
***********************************************************/
typedef OPERATE_RET (*HTTP_GET_FILE_DATA_CB)(IN PVOID_T priv_data, IN CONST UINT_T total_len,IN CONST UINT_T offset,\
                                                        IN CONST BYTE_T *data,IN CONST UINT_T len,OUT UINT_T *remain_len);

/***********************************************************
*************************variable define********************
***********************************************************/

/***********************************************************
*************************function define********************
***********************************************************/

/***********************************************************
*  Function: httpc_gw_timer_v40
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET httpc_gw_timer_v40(IN CONST CHAR_T *url,IN CONST CHAR_T *auth_key,\
                                   IN CONST INT_T offset, IN CONST INT_T limit,IN CONST CHAR_T *dev_id,OUT cJSON **result);

/***********************************************************
*  Function: httpc_gw_timer_cnt_v40
*  Return: OPERATE_RET
***********************************************************/
__IOT_HTTPC_EXT \
OPERATE_RET httpc_gw_timer_cnt_v40(IN CONST CHAR_T *url,IN CONST CHAR_T *auth_key,\
                                      IN CONST INT_T posix,IN CONST CHAR_T *dev_id,OUT cJSON **result);

/***********************************************************
*  Function: httpc_gw_get_urlcfg_v40
*  Input: url uuid auth_key token region
*  Output: OPRT_OK  result->{"apiUrl":"http://a.gw.tuyacn.com/gw.json",
                             "mqttUrl":"mq.gw.tuyacn.com",
                             "timeZone":"+08:00"}
           other  result == NULL
*  Return: OPERATE_RET
***********************************************************/
__IOT_HTTPC_EXT \
OPERATE_RET httpc_gw_get_urlcfg_v40(IN CONST CHAR_T *url,IN CONST CHAR_T *uuid,IN CONST CHAR_T *auth_key,\
                                    IN CONST CHAR_T *token,IN CONST CHAR_T *region,OUT cJSON **result);

/***********************************************************
*  Function: httpc_gw_register_v40
*  Input: url uuid auth_key hid prod_key
*  Output: OPRT_OK  result->{"devId": "6ca74974ab6f44c23c7T1r"}
           other  result == NULL
*  Return: OPERATE_RET
***********************************************************/
__IOT_HTTPC_EXT \
OPERATE_RET httpc_gw_register_v40(IN CONST CHAR_T *url,IN CONST CHAR_T *uuid,IN CONST CHAR_T *auth_key,\
                                  IN CONST CHAR_T *hid,IN CONST CHAR_T *prod_key,OUT cJSON **result);

/***********************************************************
*  Function: httpc_gw_active_v40
*  Input: param
*  Output: OPRT_OK
           result->{"schemaId": "000000000a",
                    "schema": "[{\"id\":1,\"mode\":\"rw\",\"property\":{\"type\":\"bool\"},\"type\":\"obj\"}]",
                    "secKey": "eae05133a824247e",
                    "localKey": "db1515cc2ff1cb2e"}
           other  result == NULL
*  Return: OPERATE_RET
***********************************************************/
__IOT_HTTPC_EXT \
OPERATE_RET httpc_gw_active_v40(IN CONST GW_ACTV_IN_PARM_S *param,OUT cJSON **result);

/***********************************************************
*  Function: httpc_gw_active_v41
*  Input: param
*  Output: OPRT_OK
           result->{"schemaId": "000000000a",
                    "schema": "[{\"id\":1,\"mode\":\"rw\",\"property\":{\"type\":\"bool\"},\"type\":\"obj\"}]",
                    "secKey": "eae05133a824247e",
                    "localKey": "db1515cc2ff1cb2e",
                    "devid":"xxxxxx"}
           other  result == NULL
*  Return: OPERATE_RET
***********************************************************/
__IOT_HTTPC_EXT \
OPERATE_RET httpc_gw_active_v41(IN CONST GW_ACTV_IN_PARM_V41_S *param,OUT cJSON **result);

/***********************************************************
*  Function: httpc_gw_is_exist_v40
*  Input: url
          id->gw id
          key->secKey
*  Output:
*  Return: OPERATE_RET
***********************************************************/
__IOT_HTTPC_EXT \
OPERATE_RET httpc_gw_is_exist_v40(IN CONST CHAR_T *url,IN CONST CHAR_T *id,IN CONST CHAR_T *key);

/***********************************************************
*  Function: httpc_gw_is_exist_v40
*  Input: url
          id->gw id
          key->secKey
*  Output:
*  Return: OPERATE_RET
***********************************************************/
__IOT_HTTPC_EXT \
OPERATE_RET httpc_gw_is_exist_v40(IN CONST CHAR_T *url,IN CONST CHAR_T *id,IN CONST CHAR_T *key);

/***********************************************************
*  Function: httpc_gw_reset_v40
*  Input: url
          id->gw id
          key->secKey
*  Output:
*  Return: OPERATE_RET
***********************************************************/
__IOT_HTTPC_EXT \
OPERATE_RET httpc_gw_reset_v40(IN CONST CHAR_T *url,IN CONST CHAR_T *id,IN CONST CHAR_T *key);

/***********************************************************
*  Function: httpc_get_gw_upgrd_if_v40
*  Input: url
          id->gw id
          key->secKey
*  Output: OPRT_OK  {
            "url": "http:xxx.bin",
            "size": "716411",
            "md5": "f8e741c7106b05bfffebbc7a86b032a1",
            "version": "1.0.0"
           }
           other  result == NULL
*  Return: OPERATE_RET
***********************************************************/
__IOT_HTTPC_EXT \
OPERATE_RET httpc_get_gw_upgrd_if_v40(IN CONST CHAR_T *url,IN CONST CHAR_T *id,IN CONST CHAR_T *key,\
                                      OUT cJSON **result);

/***********************************************************
*  Function: httpc_get_gw_upgrd_if_v41
*  Input: url
          id->gw id
          key->secKey
          tp
*  Output: OPRT_OK  {
            "url": "http:xxx.bin",
            "size": "716411",
            "md5": "f8e741c7106b05bfffebbc7a86b032a1",
            "originalUrl":"", // option
            "cdnUrl":"", // option
            "version": "1.0.0"
           }
           other  result == NULL
*  Return: OPERATE_RET
***********************************************************/
__IOT_HTTPC_EXT \
OPERATE_RET httpc_get_gw_upgrd_if_v41(IN CONST CHAR_T *url,IN CONST CHAR_T *id,IN CONST CHAR_T *key,\
                                                  IN CONST CLOUD_DEV_TP_DEF_T tp,OUT cJSON **result);

/***********************************************************
*  Function: httpc_gw_upgrd_stat_update_v40
*  Input: url
          id->gw id
          key->secKey
*  Output: OPRT_OK null
           other  result == NULL
*  Return: OPERATE_RET
***********************************************************/
__IOT_HTTPC_EXT \
OPERATE_RET httpc_gw_upgrd_stat_update_v40(IN CONST CHAR_T *url,IN CONST CHAR_T *id,IN CONST CHAR_T *key,\
                                           IN CONST BYTE_T stat);

/***********************************************************
*  Function: httpc_gw_upgrd_stat_update_v41
*  Input: url
          id->gw id
          tp->type
          key->secKey
*  Output: OPRT_OK null
           other  result == NULL
*  Return: OPERATE_RET
***********************************************************/
__IOT_HTTPC_EXT \
OPERATE_RET httpc_gw_upgrd_stat_update_v41(IN CONST CHAR_T *url,IN CONST CHAR_T *id,IN CONST CLOUD_DEV_TP_DEF_T tp,\
                                                          IN CONST CHAR_T *key,IN CONST BYTE_T stat);

/***********************************************************
*  Function: httpc_gw_update_if_v40
*  Input: url
          id->gw id
          key->secKey
          sw_ver dev_ver pv bv
*  Output: OPRT_OK null
           other  result == NULL
*  Return: OPERATE_RET
***********************************************************/
__IOT_HTTPC_EXT \
OPERATE_RET httpc_gw_update_if_v40(IN CONST CHAR_T *url,IN CONST CHAR_T *id,IN CONST CHAR_T *key,\
                                              IN CONST CHAR_T *sw_ver,IN CONST CHAR_T *dev_ver,IN CONST CHAR_T *pv,IN CONST CHAR_T *bv);


/***********************************************************
*  Function: httpc_gw_update_if_v41
*  Input: url
*         id->gw id
*         key->secKey
*         vers->[{"type":3,"softVer":"1.0"}]
*  Output: OPRT_OK null
           other  result == NULL
*  Return: OPERATE_RET
***********************************************************/
__IOT_HTTPC_EXT \
OPERATE_RET httpc_gw_update_if_v41(IN CONST CHAR_T *url,IN CONST CHAR_T *id,IN CONST CHAR_T *key,IN CONST CHAR_T *vers);

/***********************************************************
*  Function: httpc_get_dev_upgrd_if_v40
*  Input: url
          gid->gateway id
          key->secKey
          id->devid
*  Output: OPRT_OK  {
            "url": "http:xxx.bin",
            "size": "716411",
            "md5": "f8e741c7106b05bfffebbc7a86b032a1",
            "version": "1.0.0"
           }
           other  result == NULL
*  Return: OPERATE_RET
***********************************************************/
__IOT_HTTPC_EXT \
OPERATE_RET httpc_get_dev_upgrd_if_v40(IN CONST CHAR_T *url,IN CONST CHAR_T *gid,IN CONST CHAR_T *key,\
                                       IN CONST CHAR_T *id,OUT cJSON **result);

/***********************************************************
*  Function: httpc_get_dev_upgrd_if_v41
*  Input: url
          gid->gateway id
          key->secKey
          id->devid
          tp
*  Output: OPRT_OK  {
            "url": "http:xxx.bin",
            "size": "716411",
            "md5": "f8e741c7106b05bfffebbc7a86b032a1",
            "version": "1.0.0"
           }
           other  result == NULL
*  Return: OPERATE_RET
***********************************************************/
__IOT_HTTPC_EXT \
OPERATE_RET httpc_get_dev_upgrd_if_v41(IN CONST CHAR_T *url,IN CONST CHAR_T *gid,IN CONST CHAR_T *key,\
                                                    IN CONST CHAR_T *id,IN CONST CLOUD_DEV_TP_DEF_T tp,OUT cJSON **result);

/***********************************************************
*  Function: httpc_dev_upgrd_stat_update_v40
*  Input: url
          gid->gw id
          key->secKey
          id->devid
*  Output: OPRT_OK null
*  Return: OPERATE_RET
***********************************************************/
__IOT_HTTPC_EXT \
OPERATE_RET httpc_dev_upgrd_stat_update_v40(IN CONST CHAR_T *url,IN CONST CHAR_T *gid,IN CONST CHAR_T *key,\
                                            IN CONST CHAR_T *id,IN CONST BYTE_T stat);

/***********************************************************
*  Function: httpc_dev_upgrd_stat_update_v41
*  Input: url
          gid->gw id
          key->secKey
          id->devid
          tp
*  Output: OPRT_OK null
*  Return: OPERATE_RET
***********************************************************/
__IOT_HTTPC_EXT \
OPERATE_RET httpc_dev_upgrd_stat_update_v41(IN CONST CHAR_T *url,IN CONST CHAR_T *gid,IN CONST CHAR_T *key,\
                                                           IN CONST CHAR_T *id,IN CONST CLOUD_DEV_TP_DEF_T tp,IN CONST BYTE_T stat);

/***********************************************************
*  Function: httpc_dev_update_if_v40
*  Input: url
          gid->gw id
          key->secKey
          id->devid
*  Output: OPRT_OK null
*  Return: OPERATE_RET
***********************************************************/
__IOT_HTTPC_EXT \
OPERATE_RET httpc_dev_update_if_v40(IN CONST CHAR_T *url,IN CONST CHAR_T *gid,IN CONST CHAR_T *key,\
                                    IN CONST CHAR_T *id,IN CONST CHAR_T *sw_ver);

/***********************************************************
*  Function: httpc_dev_update_if_v41
*  Input: url
          gid->gw id
          key->secKey
          tp->CLOUD_DEV_TP_DEF_T
          id->devid
*  Output: OPRT_OK null
*  Return: OPERATE_RET
***********************************************************/
__IOT_HTTPC_EXT \
OPERATE_RET httpc_dev_update_if_v41(IN CONST CHAR_T *url,IN CONST CHAR_T *gid,IN CONST CHAR_T *key,\
                                               IN CONST CHAR_T *id,IN CONST CLOUD_DEV_TP_DEF_T tp,\
                                               IN CONST CHAR_T *sw_ver);

/***********************************************************
*  Function: httpc_dev_dp_report_v40
*  Input: url
*          gid->gw id
*          key->secKey
*          data->{
*                    "dps": {
*                        "1": true,
*                        "2": 9
*                    },
*                    // yes or no
*                    "dpsTime": {
*                        "1": 1476173089,
*                        "2": 1476173089
*                    },
*                    "devId":"002yt001sf000000sfV3"
*                }
*  Output: OPRT_OK null
*  Return: OPERATE_RET
***********************************************************/
__IOT_HTTPC_EXT \
OPERATE_RET httpc_dev_dp_report_v40(IN CONST CHAR_T *url,IN CONST CHAR_T *gid,\
                                    IN CONST CHAR_T *key,IN CONST CHAR_T *data);

/***********************************************************
*  Function: httpc_dev_bind_v40
*  Input: url
          gid->gateway id
          key->secKey
          id->devid
          product_key
          sw_ver
*  Output: OPRT_OK  result->{"schemaId": "000000000a",
                    "schema": "[{\"id\":1,\"mode\":\"rw\",\"property\":{\"type\":\"bool\"},\"type\":\"obj\"}]"}
           other  result == NULL
*  Return: OPERATE_RET
***********************************************************/
__IOT_HTTPC_EXT \
OPERATE_RET httpc_dev_bind_v40(IN CONST CHAR_T *url,IN CONST CHAR_T *gid,IN CONST CHAR_T *key,\
                               IN CONST CHAR_T *id,IN CONST CHAR_T *product_key,IN CONST CHAR_T *sw_ver,
                               OUT cJSON **result);


/***********************************************************
*  Function: httpc_dev_bind_v41
*  Input: url
          gid->gateway id
          key->secKey
          id->devid
          product_key->xxxxx
          sw_ver->x.x.x
          online
          type->CLOUD_DEV_TP_DEF_T
*  Output: OPRT_OK  result->{"schemaId": "000000000a",
                    "schema": "[{\"id\":1,\"mode\":\"rw\",\"property\":{\"type\":\"bool\"},\"type\":\"obj\"}]"}
           other  result == NULL
*  Return: OPERATE_RET
***********************************************************/
__IOT_HTTPC_EXT \
OPERATE_RET httpc_dev_bind_v41(IN CONST CHAR_T *url,IN CONST CHAR_T *gid,IN CONST CHAR_T *key,\
                                        IN CONST CHAR_T *id,IN CONST CHAR_T *product_key,IN CONST CHAR_T *sw_ver,\
                                        IN CONST BOOL_T online,IN CONST CLOUD_DEV_TP_DEF_T type,OUT cJSON **result);

/***********************************************************
*  Function: httpc_dev_unbind_v40
*  Input: url
          gid->gateway id
          key->secKey
          id->devid
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
__IOT_HTTPC_EXT \
OPERATE_RET httpc_dev_unbind_v40(IN CONST CHAR_T *url,IN CONST CHAR_T *gid,\
                                 IN CONST CHAR_T *key,IN CONST CHAR_T *id);


 /***********************************************************
 *  Function: httpc_dev_unbind_v41
 *  Input: url
           gid->gateway id
           key->secKey
           id->devid
 *  Output: none
 *  Return: OPERATE_RET
 ***********************************************************/
 __IOT_HTTPC_EXT \
 OPERATE_RET httpc_dev_unbind_v41(IN CONST CHAR_T *url,IN CONST CHAR_T *gid,\
                                  IN CONST CHAR_T *key,IN CONST CHAR_T *id);


/***********************************************************
*  Function: httpc_dev_stat_rept_v40
*  Input: url
          gid->gateway id
          key->secKey
          devids->[002yt001sf000000sfV3,002yt001sf000000sfV4,002yt001sf000000sfV5]
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
__IOT_HTTPC_EXT \
OPERATE_RET httpc_dev_stat_rept_v40(IN CONST CHAR_T *url,IN CONST CHAR_T *gid,\
                                    IN CONST CHAR_T *key,IN CONST BOOL_T online,\
                                    IN CONST CHAR_T *devids);

/***********************************************************
*  Function: httpc_gw_get_prt_intr_v10
*  Input: url
          gid->gateway id
          key->secKey
*  Output: result->{"validTime": 60}
*  Return: OPERATE_RET
***********************************************************/
__IOT_HTTPC_EXT \
OPERATE_RET httpc_gw_get_prt_intr_v10(IN CONST CHAR_T *url,IN CONST CHAR_T *gid,\
                                      IN CONST CHAR_T *key,OUT cJSON **result);

/***********************************************************
*  Function: httpc_get_file_v10
*  Input: url
*         mlk_buf_len->malloc buffer len,the function auto process
*         gfd_cb->get file data callback
*         priv_data -> private data
*  Output:
*  Return: OPERATE_RET
***********************************************************/
__IOT_HTTPC_EXT \
OPERATE_RET httpc_get_file_v10(IN CONST CHAR_T *url,IN CONST UINT_T mlk_buf_len,\
                                        IN CONST HTTP_GET_FILE_DATA_CB gfd_cb, IN PVOID_T priv_data, IN CONST UINT_T total_len);

__IOT_HTTPC_EXT \
OPERATE_RET httpc_gw_get_runtime_cfg_v40(IN CONST CHAR_T *url,IN CONST CHAR_T *id,IN CONST CHAR_T *key,\
                                                       OUT cJSON **result);

__IOT_HTTPC_EXT \
OPERATE_RET httpc_put_rst_log_v10(IN CONST CHAR_T *url,IN CONST CHAR_T *id,\
                                            IN CONST CHAR_T *auth_key,IN CONST CHAR_T *rs_data);

__IOT_HTTPC_EXT \
OPERATE_RET httpc_dynamic_cfg_get_v10(IN CONST CHAR_T *url,IN CONST CHAR_T *id,\
                                            IN CONST CHAR_T *auth_key,OUT cJSON **result);

__IOT_HTTPC_EXT \
OPERATE_RET httpc_dynamic_cfg_ack_v10(IN CONST CHAR_T *url,IN CONST CHAR_T *id,\
                                            IN CONST CHAR_T *auth_key,IN CONST CHAR_T *ackid);



__IOT_HTTPC_EXT \
OPERATE_RET httpc_get_self_fw_ug_info_v41(IN CONST CHAR_T *url,IN CONST CHAR_T *id,IN CONST CHAR_T *sub_id,\
                                            IN CONST CHAR_T *auth_key,OUT cJSON **result);

OPERATE_RET httpc_get_dev_list_v20(IN CONST CHAR_T *url,IN CONST CHAR_T *id,\
                                         IN CONST CHAR_T *key,OUT cJSON **result);

 OPERATE_RET httpc_gw_get_dueros_config_v10(IN CONST CHAR_T *url,IN CONST CHAR_T *gid,\
														 IN CONST CHAR_T *key,OUT cJSON **result);


#ifdef __cplusplus
}
#endif
#endif

