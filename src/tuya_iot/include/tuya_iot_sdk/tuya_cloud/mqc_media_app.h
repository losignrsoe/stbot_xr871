#ifndef _MQC_MEDIA_APP_H
#define _MQC_MEDIA_APP_H

#include "tuya_cloud_types.h"
#include "tuya_cloud_com_defs.h"
#include "mqtt_client.h"
#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

OPERATE_RET mqc_media_init(VOID);
BOOL_T get_mqc_media_conn_stat(VOID);

OPERATE_RET mqc_media_pub_async(IN CONST FLOW_BODY_ST *dt_body,
                                IN CONST UINT_T to_lmt, IN CONST MQ_PUB_ASYNC_IFM_CB cb,IN VOID *prv_data);
OPERATE_RET mqc_media_pub_sync(IN CONST FLOW_BODY_ST *dt_body,IN CONST UINT_T timeout);

typedef OPERATE_RET (*mqc_media_handler_cb)(IN cJSON *root_json);
OPERATE_RET mqc_media_register_cb(UINT_T mq_pro, mqc_media_handler_cb handler);



#ifdef __cplusplus
}
#endif
#endif

