/***********************************************************
*  File: mqc_app.h 
*  Author: nzy
*  Date: 20170411
***********************************************************/
#ifndef _MQC_APP_H
    #define _MQC_APP_H

    #include "tuya_cloud_types.h"
    #include "mqtt_client.h"
    #include "gw_intf.h"

#ifdef __cplusplus
	extern "C" {
#endif

#ifdef  __MQC_APP_GLOBALS
    #define __MQC_APP_EXT
#else
    #define __MQC_APP_EXT extern
#endif

/***********************************************************
*************************micro define***********************
***********************************************************/
// mqtt protocol
#define PRO_DATA_PUSH 4   /* dev -> cloud push dp data */
#define PRO_CMD 5         /* cloud -> dev send dp data */
#define PRO_GW_RESET 11   /* cloud -> dev reset dev */
#define PRO_TIMER_UG_INF 13   /* cloud -> dev update timer */
#define PRO_UPGD_REQ 15       /* cloud -> dev update dev/gw */
#define PRO_UPGE_PUSH 16      /* dev -> cloud update upgrade percent */
#define PRO_DEV_LINE_STAT_UPDATE 25 /* dev -> sub device online status update */
#define PRO_CMD_ACK  26    /* dev -> cloud  dev send ackId to cloud */
#define PRO_MQ_EXT_CFG_INF 27 /* cloud -> dev runtime cfg update */
#define PRO_MQ_QUERY_DP  31  /* cloud -> dev query dp stat */
    
#define PRO_IOT_DA_REQ 22   /* cloud -> dev send data req */
#define PRO_IOT_DA_RESP 23  /* dev -> cloud send data resp */
#define PRO_DEV_UNBIND 8 /* cloud -> dev */
// #define PRO_DEV_LINE_STAT_UPDATE 25 /* dev->cloud */
#define PRO_UG_SUMER_TABLE 41 // ug sumer timer table


#define PRO_MQ_EN_GW_ADD_DEV_REQ 200 // gw enable add sub device request
#define PRT_MQ_EN_GW_ADD_DEV_RESP 201 // gw enable add sub device respond

/***********************************************************
*************************variable define********************
***********************************************************/


/***********************************************************
*************************function define********************
***********************************************************/
/***********************************************************
*  Function: mqc_app_init
*  Input: none
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
__MQC_APP_EXT \
OPERATE_RET mqc_app_init(VOID);

/***********************************************************
*  Function: get_mqc_conn_stat
*  Input: none
*  Output: none
*  Return: BOOL_T
***********************************************************/
__MQC_APP_EXT \
BOOL_T get_mqc_conn_stat(VOID);

/***********************************************************
*  Function: mqc_pub_async
*  Input: qos if(0 == qos) then to_lmt cb prv_data useless,
*                 becase no respond wait. 
*             else if(1 == qos) then need wait respond.
*             else then do't support.
*         data
*         len
*         to_lmt timeout limit if(0 == to_lmt) then use system default limit
*         cb
*         prv_data
*  Output: hand
*  Return: OPERATE_RET
***********************************************************/
__MQC_APP_EXT \
OPERATE_RET mqc_pub_async(IN CONST BYTE_T qos,IN CONST BYTE_T *data,IN CONST INT_T len,\
                          IN CONST UINT_T to_lmt,IN CONST MQ_PUB_ASYNC_IFM_CB cb,IN VOID *prv_data);

/***********************************************************
*  Function: mqc_dp_report_async
*  Input: data
*         len
*         to_lmt timeout limit if(0 == to_lmt) then use system default limit
*         cb
*         prv_data
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
__MQC_APP_EXT \
OPERATE_RET mqc_dp_report_async(IN CONST VOID *data,IN CONST UINT_T len,IN CONST UINT_T to_lmt,\
                                IN CONST MQ_PUB_ASYNC_IFM_CB cb,IN VOID *prv_data);

/***********************************************************
*  Function: mqc_obj_dp_query
*  Input: data
*         len
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
__MQC_APP_EXT \
OPERATE_RET mqc_obj_dp_query(IN CONST VOID *data,IN CONST UINT_T len);

/***********************************************************
*  Function: mqc_upgd_progress_rept
*  Input: percent devid
*  Output: none
*  Return: OPERATE_RET
*  Note: if(devid == null) 
*            gateway upgrade; 
*         else 
*            devid upgrade;
***********************************************************/
__MQC_APP_EXT \
OPERATE_RET mqc_upgd_progress_rept(IN CONST UINT_T percent,IN CONST CHAR_T *devid);

/***********************************************************
*  Function: mqc_upgd_progress_rept_v41
*  Input: percent devid tp
*  Output: none
*  Return: OPERATE_RET
*  Note: if(devid == null) 
*            gateway upgrade;
*         else 
*            devid upgrade;
***********************************************************/
__MQC_APP_EXT \
OPERATE_RET mqc_upgd_progress_rept_v41(IN CONST UINT_T percent,IN CONST CHAR_T *devid,\
                                                    IN CONST CLOUD_DEV_TP_DEF_T tp);

OPERATE_RET mqc_obj_resp_ackid(IN CONST CHAR_T *ackid);

OPERATE_RET mqc_obj_handle_da_req(IN CONST VOID *rootJson);


/***********************************************************
*  Function: mqc_prot_data_rept
*  Input: pro->mqtt protocol number
*         num->command seq only use to dp command other num == 0
*         data
*         qos if(0 == qos) then to_lmt cb prv_data useless,
*                 becase no respond wait. 
*             else if(1 == qos) then need wait respond.
*             else then do't support.
*         to_lmt timeout limit if(0 == to_lmt) then use system default limit
*         cb
*         prv_data
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
__MQC_APP_EXT \
OPERATE_RET mqc_prot_data_rept(IN CONST UINT_T pro,IN CONST UINT_T num,IN CONST CHAR_T *data,IN CONST BYTE_T qos,\
                                        IN CONST UINT_T to_lmt,IN CONST MQ_PUB_ASYNC_IFM_CB cb,IN VOID *prv_data);

/***********************************************************
*  Function: mqc_prot_data_rept_seq
*  Input: pro->mqtt protocol number
*         data
*         qos if(0 == qos) then to_lmt cb prv_data useless,
*                 becase no respond wait. 
*             else if(1 == qos) then need wait respond.
*             else then do't support.
*         to_lmt timeout limit if(0 == to_lmt) then use system default limit
*         cb
*         prv_data
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
__MQC_APP_EXT \
OPERATE_RET mqc_prot_data_rept_seq(IN CONST UINT_T pro,IN CONST CHAR_T *data,IN CONST BYTE_T qos,\
                                              IN CONST UINT_T to_lmt,IN CONST MQ_PUB_ASYNC_IFM_CB cb,IN VOID *prv_data);


#ifdef __cplusplus
}
#endif
#endif

