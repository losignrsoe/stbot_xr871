/***********************************************************
*  File: smart_frame.h 
*  Author: nzy
*  Date: 20170413
***********************************************************/
#ifndef _SMART_FRAME_H
#define _SMART_FRAME_H

#include "tuya_cloud_types.h"
#include "gw_intf.h"

#ifdef __cplusplus
	extern "C" {
#endif

typedef struct {
    DP_CMD_TYPE_E tp;
    cJSON *cmd_js;
}SF_GW_DEV_CMD_S;


typedef struct msg_data_s{
	UINT_T serno;
	UINT_T len;
	BYTE_T data[0];
}MSG_DATA_S;

OPERATE_RET smart_frame_init(VOID);

/* 接收并处理来自mqtt，tcp，本地定时的消息 */
OPERATE_RET sf_send_gw_dev_cmd(IN SF_GW_DEV_CMD_S *gd_cmd);
/* 开启定时器上送本地dp */
VOID sf_start_sync_obj_dp(VOID);

/* 生成本地待上传的dp数据字符串 */
CHAR_T *sf_pack_local_obj_dp_data(IN DEV_CNTL_N_S *dev_cntl, IN CONST UINT_T max_len, IN CONST BOOL_T addDevId, OUT BOOL_T *p_all_data_packed, BOOL_T reset_flow_ctl);

VOID sf_dbg_print_dp_stats(VOID);


UINT_T sf_get_serial_no(VOID);
VOID sf_inc_serial_no(VOID);

/* MQTT 31协议查询本地DP */
OPERATE_RET sf_respone_obj_dp_query(IN CONST cJSON *pCidArr, IN CONST cJSON *pDpIdArr);


OPERATE_RET sf_obj_dp_report_async(IN CONST CHAR_T *id,IN CONST CHAR_T *data, IN CONST BOOL_T force_send);
OPERATE_RET sf_raw_dp_report_sync(  IN CONST CHAR_T *id,IN CONST BYTE_T dpid,
                                    IN CONST BYTE_T *data,IN CONST UINT_T len,
                                    IN CONST UINT_T timeout);
/* stat */
OPERATE_RET sf_stat_dp_report_sync(IN CONST CHAR_T *dev_id, IN CONST CHAR_T *dp_str, IN CONST CHAR_T *time_str, IN CONST UINT_T timeout);

/***********************************************************
*  Function: sf_dev_online_stat_update
*  Input: dev_id online
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET sf_dev_online_stat_update(IN CONST CHAR_T *dev_id,IN CONST BOOL_T online);

/***********************************************************
*  Function: sf_sys_mag_hb_init
*  Input: hb_send_cb
*         if(NULL == hb_send_cb) 
*             passive heartbeat pattern
*         else
*             active heartbeat pattern
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET sf_sys_mag_hb_init(IN CONST DEV_HEARTBEAT_SEND_CB hb_send_cb);

/***********************************************************
*  Function: sf_refresh_dev_hb
*  Input: dev_id
*         hb_timeout_time->if(0xffffffff == hb_timeout_time)
*                          then device online forever
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET sf_set_dev_hb_timeout(IN CONST CHAR_T *dev_id,IN CONST TIME_S hb_timeout_time);

/***********************************************************
*  Function: sf_refresh_dev_hb
*  Input: dev_id
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET sf_refresh_dev_hb(IN CONST CHAR_T *dev_id);

#ifdef __cplusplus
}
#endif
#endif

