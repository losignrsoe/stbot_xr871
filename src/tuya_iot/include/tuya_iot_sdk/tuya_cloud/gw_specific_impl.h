#ifndef _GW_SPECIFIC_IMPL_H
#define _GW_SPECIFIC_IMPL_H

    #include "tuya_cloud_types.h"
    #include "cJSON.h"
    #include "gw_intf.h"

#ifdef __cplusplus
    extern "C" {
#endif

OPERATE_RET http_gw_get_timer_cnt(IN CONST INT_T lastFetchTime, OUT cJSON **result);
OPERATE_RET http_gw_get_timer_content(IN CONST INT_T offset, IN CONST INT_T limit, OUT cJSON **result);

OPERATE_RET http_device_check_upgrade_info_by_devid(IN CONST CHAR_T *dev_id, OUT cJSON **result);
OPERATE_RET http_dev_chk_upgd_info_v41(IN CONST CHAR_T *dev_id,IN CONST CLOUD_DEV_TP_DEF_T tp,OUT cJSON **result);

OPERATE_RET http_device_update_upgrade_status_by_devid(IN CONST CHAR_T *dev_id, IN CONST BYTE_T status);
OPERATE_RET http_dev_update_upgd_stat_v41(IN CONST CHAR_T *dev_id,IN CONST CLOUD_DEV_TP_DEF_T tp,IN CONST BYTE_T status);

OPERATE_RET http_device_bind(IN CONST CHAR_T *dev_id, IN CONST CHAR_T *product_key, IN CONST CHAR_T *sw_ver,OUT cJSON **result);
OPERATE_RET http_device_bind_v41(IN CONST CHAR_T *dev_id, IN CONST CHAR_T *product_key,\
                                           IN CONST CHAR_T *sw_ver, IN CONST BOOL_T online,\
                                           IN CONST CLOUD_DEV_TP_DEF_T type, OUT cJSON **result);

OPERATE_RET http_device_unbind(IN CONST CHAR_T *dev_id);
OPERATE_RET http_device_unbind_v41(IN CONST CHAR_T *dev_id);


OPERATE_RET http_device_update_version(IN CONST CHAR_T *dev_id);
OPERATE_RET http_device_update_version_v41(IN CONST CHAR_T *dev_id,IN CONST CHAR_T *ver,\
                                                          IN CONST CLOUD_DEV_TP_DEF_T tp);

OPERATE_RET http_device_online(IN CONST CHAR_T *dev_id);
OPERATE_RET http_device_offline(IN CONST CHAR_T *dev_id);

OPERATE_RET http_gw_check_upgrade_info(OUT cJSON **result);
OPERATE_RET http_gw_chk_upgd_info_v41(IN CONST CLOUD_DEV_TP_DEF_T tp,OUT cJSON **result);

OPERATE_RET http_gw_update_upgrade_status(IN CONST BYTE_T status);
OPERATE_RET http_gw_update_upgrade_status_v41(IN CONST CLOUD_DEV_TP_DEF_T tp,IN CONST BYTE_T status);

OPERATE_RET http_gw_reset(VOID);
OPERATE_RET http_gw_check_exist(VOID);
OPERATE_RET http_gw_get_cloud_url(VOID);

OPERATE_RET http_gw_register(OUT cJSON **result);
OPERATE_RET http_gw_register_V41(OUT cJSON **result);

OPERATE_RET http_gw_active(OUT cJSON **result);
OPERATE_RET http_gw_active_v41(OUT cJSON **result);

OPERATE_RET http_gw_update_version(VOID);
OPERATE_RET http_gw_update_version_v41(VOID);

OPERATE_RET http_gw_get_replay_limit(OUT cJSON **result);

typedef OPERATE_RET (*HTTP_GW_GET_FILE_DATA_CB)(IN PVOID_T priv_data, IN CONST UINT_T total_len,IN CONST UINT_T offset,\
                                            IN CONST BYTE_T *data,IN CONST UINT_T len,OUT UINT_T *remain_len);
OPERATE_RET http_gw_download_file(IN CONST CHAR_T *url,IN CONST UINT_T mlk_buf_len,\
                               IN CONST HTTP_GW_GET_FILE_DATA_CB gfd_cb, IN PVOID_T priv_data, IN CONST UINT_T total_len);

OPERATE_RET wechat_gw_set_cloud_url(VOID);

OPERATE_RET http_put_rst_log(VOID);
OPERATE_RET http_get_self_fm_ug_info(OUT cJSON **result);
OPERATE_RET http_dynamic_cfg_get(OUT cJSON **result);
OPERATE_RET http_dynamic_cfg_ack(IN CONST CHAR_T *ackid);

OPERATE_RET http_get_dev_list_v20(OUT cJSON **result);

OPERATE_RET http_get_dueros_config_v10(OUT cJSON **result);

#ifdef __cplusplus
}
#endif

#endif /* _GW_SPECIFIC_IMPL_H */
