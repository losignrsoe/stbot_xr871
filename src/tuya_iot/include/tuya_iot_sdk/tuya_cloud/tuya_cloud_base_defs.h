/*!
\file tuya_cloud_base_defs.h
Copyright(C),2017, 涂鸦科技 www.tuya.comm
*/

#ifndef TUYA_CLOUD_BASE_DEFS_H
#define TUYA_CLOUD_BASE_DEFS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_cloud_types.h"

// non wifi config network status
typedef BYTE_T GW_BASE_NW_STAT_T;
#define GB_STAT_LAN_UNCONN 0
#define GB_STAT_LAN_CONN 1
#define GB_STAT_CLOUD_CONN 2

typedef struct {
    CHAR_T *uuid; // strlen(uuid) <= 16,must not be null
    CHAR_T *auth_key; // strlen(auth_key) <= 32,must not be null
}GW_PROD_INFO_S;

typedef VOID (*GET_NW_STAT_CB)(IN CONST GW_BASE_NW_STAT_T stat);

#ifdef __cplusplus
}
#endif

#endif
