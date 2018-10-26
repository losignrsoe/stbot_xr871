/**
 * Copyright (2017) Baidu Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * File: lightduer_dcs_system.c
 * Auth: Gang Chen (chengang12@baidu.com)
 * Desc: apply APIs to support DCS audio system interface.
 */

#include "lightduer_dcs_local.h"
#include "lightduer_log.h"
#include "lightduer_ca.h"
#include "lightduer_event_emitter.h"
#include "lightduer_dcs_router.h"
#include "lightduer_timers.h"
#include "lightduer_connagent.h"
#include "lightduer_timestamp.h"
#include "lightduer_ds_log_dcs.h"

static duer_timer_handler s_activity_timer;
static const int INACTIVIRY_REPORT_PERIOD = 3600 * 1000; // 1 hour
static volatile int s_last_activity_time; // latest user activity time in seconds
static const char *SYSTEM_INTERFACE_NAMESPACE = "ai.dueros.device_interface.system";

int duer_dcs_sync_state(void)
{
    baidu_json *event = NULL;
    baidu_json *data = NULL;
    baidu_json *client_context = NULL;
    duer_status_t ret = DUER_OK;

    data = baidu_json_CreateObject();
    if (data == NULL) {
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        ret = DUER_ERR_MEMORY_OVERLOW;
        goto RET;
    }

    client_context = duer_get_client_context_internal();
    if (client_context) {
        baidu_json_AddItemToObject(data, "clientContext", client_context);
    }

    event = duer_create_dcs_event(SYSTEM_INTERFACE_NAMESPACE,
                                  "SynchronizeState",
                                  true);
    if (event == NULL) {
        ret = DUER_ERR_FAILED;
        goto RET;
    }

    baidu_json_AddItemToObject(data, "event", event);

    ret = duer_dcs_data_report_internal(data, true);

RET:
    if (ret != DUER_OK) {
        duer_ds_log_dcs_event_report_fail("SynchronizeState");
    }

    if (data) {
        baidu_json_Delete(data);
    }

    return ret;
}

int duer_report_exception_internal(const char *directive, const char *type, const char *msg)
{
    baidu_json *event = NULL;
    baidu_json *data = NULL;
    baidu_json *payload = NULL;
    baidu_json *error = NULL;
    baidu_json *client_context = NULL;
    duer_status_t ret = DUER_OK;

    if (!directive || !type || !msg) {
        DUER_DS_LOG_REPORT_DCS_PARAM_ERROR();
        ret = DUER_ERR_INVALID_PARAMETER;
        goto RET;
    }

    data = baidu_json_CreateObject();
    if (data == NULL) {
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        ret = DUER_ERR_MEMORY_OVERLOW;
        goto RET;
    }

    client_context = duer_get_client_context_internal();
    if (client_context) {
        baidu_json_AddItemToObject(data, "clientContext", client_context);
    }

    event = duer_create_dcs_event(SYSTEM_INTERFACE_NAMESPACE,
                                  "ExceptionEncountered",
                                  true);
    if (event == NULL) {
        ret = DUER_ERR_FAILED;
        goto RET;
    }
    baidu_json_AddItemToObject(data, "event", event);

    payload = baidu_json_GetObjectItem(event, "payload");
    if (!payload) {
        ret = DUER_ERR_FAILED;
        goto RET;
    }
    baidu_json_AddStringToObject(payload, "unparsedDirective", directive);

    error = baidu_json_CreateObject();
    if (!error) {
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        ret = DUER_ERR_MEMORY_OVERLOW;
        goto RET;
    }
    baidu_json_AddItemToObject(payload, "error", error);
    baidu_json_AddStringToObject(error, "type", type);
    baidu_json_AddStringToObject(error, "message", msg);

    ret = duer_dcs_data_report_internal(data, true);
RET:
    if (ret != DUER_OK) {
        duer_ds_log_dcs_event_report_fail("ExceptionEncountered");
    }

    if (data) {
        baidu_json_Delete(data);
    }

    return ret;
}

int duer_dcs_close_multi_dialog(void)
{
    baidu_json *event = NULL;
    baidu_json *data = NULL;
    int rs = DUER_OK;

    do {
        data = baidu_json_CreateObject();
        if (data == NULL) {
            DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
            rs = DUER_ERR_FAILED;
            break;
        }

        event = duer_create_dcs_event(SYSTEM_INTERFACE_NAMESPACE, "Exited", true);
        if (event == NULL) {
            rs = DUER_ERR_FAILED;
            break;
        }

        baidu_json_AddItemToObject(data, "event", event);
        rs = duer_dcs_data_report_internal(data, true);
    } while (0);

    if (rs != DUER_OK) {
        duer_ds_log_dcs_event_report_fail("Exited");
    }

    if (data) {
        baidu_json_Delete(data);
    }

    return rs;
}

static duer_status_t duer_exception_cb(const baidu_json *directive)
{
    baidu_json *payload = NULL;
    baidu_json *error_code = NULL;
    baidu_json *description = NULL;
    duer_status_t ret = DUER_OK;

    payload = baidu_json_GetObjectItem(directive, "payload");
    if (!payload) {
        DUER_LOGE("Failed to parse payload\n");
        ret = DUER_MSG_RSP_BAD_REQUEST;
        goto RET;
    }

    error_code = baidu_json_GetObjectItem(payload, "code");
    if (!error_code) {
        DUER_LOGE("Failed to get erorr code\n");
        ret = DUER_MSG_RSP_BAD_REQUEST;
        goto RET;
    }

    description = baidu_json_GetObjectItem(payload, "description");
    if (!description) {
        DUER_LOGE("Failed to get erorr description\n");
        ret = DUER_MSG_RSP_BAD_REQUEST;
        goto RET;
    }

    DUER_LOGE("error code: %s", error_code->valuestring);
    DUER_LOGE("error: %s", description->valuestring);

RET:
    return ret;
}

static void duer_inactivity_report_handler(int what, void *param)
{
    int inactivity_time = 0;
    baidu_json *event = NULL;
    baidu_json *data = NULL;
    baidu_json *payload = NULL;

    data = baidu_json_CreateObject();
    if (data == NULL) {
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        goto RET;
    }

    event = duer_create_dcs_event(SYSTEM_INTERFACE_NAMESPACE,
                                  "SynchronizeState",
                                  true);
    if (event == NULL) {
        goto RET;
    }

    baidu_json_AddItemToObject(data, "event", event);
    payload = baidu_json_GetObjectItem(event, "payload");
    if (!payload) {
        goto RET;
    }

    inactivity_time = duer_timestamp() / 1000 - s_last_activity_time;

    inactivity_time = inactivity_time > 0 ? inactivity_time : 0;
    baidu_json_AddNumberToObject(payload, "inactiveTimeInSeconds", inactivity_time);

    duer_dcs_data_report_internal(data, true);

RET:
    if (data) {
        baidu_json_Delete(data);
    }
}

static void duer_inactivity_report(void *param)
{
    duer_emitter_emit(duer_inactivity_report_handler, 0, NULL);
}

void duer_user_activity_internal(void)
{
    s_last_activity_time = duer_timestamp() / 1000;
}

static duer_status_t duer_inactivity_reset_cb(const baidu_json *directive)
{
    duer_user_activity_internal();

    return DUER_OK;
}

static duer_status_t duer_nop_cb(const baidu_json *directive)
{
    DUER_LOGI("Nop directive received");
    return DUER_OK;
}

void duer_declare_sys_interface_internal(void)
{
    static bool is_first_time = true;

    if (!is_first_time) {
        return;
    }

    is_first_time = false;

    s_activity_timer = duer_timer_acquire(duer_inactivity_report, NULL, DUER_TIMER_PERIODIC);
    duer_timer_start(s_activity_timer, INACTIVIRY_REPORT_PERIOD);

    duer_directive_list res[] = {
        {"ThrowException", duer_exception_cb},
        {"ResetUserInactivity", duer_inactivity_reset_cb},
        {"Nop", duer_nop_cb},
    };

    duer_add_dcs_directive(res, sizeof(res) / sizeof(res[0]), SYSTEM_INTERFACE_NAMESPACE);
}

