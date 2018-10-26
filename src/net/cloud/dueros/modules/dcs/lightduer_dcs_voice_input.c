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
 * File: lightduer_dcs_voice_input.c
 * Auth: Gang Chen (chengang12@baidu.com)
 * Desc: apply APIs to support DCS voice input interface
 */

#include "lightduer_dcs.h"
#include "lightduer_dcs_router.h"
#include "lightduer_dcs_local.h"
#include "lightduer_connagent.h"
#include "lightduer_event_emitter.h"
#include "lightduer_log.h"
#include "lightduer_timers.h"
#include "lightduer_mutex.h"
#include "lightduer_ds_log_dcs.h"
#include "lightduer_voice.h"

static int s_wait_time = 0;
static duer_timer_handler s_listen_timer;
static volatile bool s_is_multiple_dialog = false;

#define LIMIT_LISTEN_TIME

static const char *VOICE_INPUT_NAMESPACE = "ai.dueros.device_interface.voice_input";
#ifdef LIMIT_LISTEN_TIME
// If no listen stop directive received in 20s, close the recorder
static const int MAX_LISTEN_TIME = 20000;
static duer_timer_handler s_close_mic_timer;
#endif
static volatile bool s_is_listening;
static duer_mutex_t s_mic_lock;

static baidu_json *s_initiator;

bool duer_is_recording_internal(void)
{
    return s_is_listening;
}

int duer_dcs_on_listen_started(void)
{
    baidu_json *event = NULL;
    baidu_json *header = NULL;
    baidu_json *payload = NULL;
    baidu_json *data = NULL;
    baidu_json *client_context = NULL;
    char buf[20];
    int rs = DUER_OK;
    duer_voice_mode user_mode = DUER_VOICE_MODE_DEFAULT;

    user_mode = duer_voice_get_mode();
    duer_mutex_lock(s_mic_lock);

#ifdef LIMIT_LISTEN_TIME
    if (s_close_mic_timer) {
        if (s_is_listening) {
            duer_timer_stop(s_close_mic_timer);
        }
        duer_timer_start(s_close_mic_timer, MAX_LISTEN_TIME);
    }
#endif
    s_is_listening = true;
    duer_mutex_unlock(s_mic_lock);

    if (s_is_multiple_dialog && (s_wait_time != 0) && (s_listen_timer != NULL)) {
        duer_timer_stop(s_listen_timer);
    }

    if (user_mode == DUER_VOICE_MODE_DEFAULT || user_mode == DUER_VOICE_MODE_C2E_BOT) {
        duer_user_activity_internal();
    }
    s_is_multiple_dialog = false;
    duer_play_channel_control_internal(DCS_RECORD_STARTED);

    data = baidu_json_CreateObject();
    if (data == NULL) {
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        rs = DUER_ERR_FAILED;
        goto RET;
    }

    client_context = duer_get_client_context_internal();
    if (client_context) {
        baidu_json_AddItemToObject(data, "clientContext", client_context);
    }

    event = duer_create_dcs_event(VOICE_INPUT_NAMESPACE,
                                  "ListenStarted",
                                  true);
    if (event == NULL) {
        rs = DUER_ERR_FAILED;
        goto RET;
    }

    baidu_json_AddItemToObject(data, "event", event);

    header = baidu_json_GetObjectItem(event, "header");
    snprintf(buf, sizeof(buf), "%u", duer_create_request_id_internal());
    baidu_json_AddStringToObject(header, "dialogRequestId", buf);

    payload = baidu_json_GetObjectItem(event, "payload");
    baidu_json_AddStringToObject(payload, "format", "AUDIO_L16_RATE_16000_CHANNELS_1");

    duer_mutex_lock(s_mic_lock);
    if (s_initiator) {
        baidu_json_AddItemReferenceToObject(payload, "initiator", s_initiator);
    }
    rs = duer_dcs_data_report_internal(data, false);
    if (s_initiator) {
        baidu_json_Delete(s_initiator);
        s_initiator = NULL;
    }
    duer_mutex_unlock(s_mic_lock);

RET:
    if (rs != DUER_OK) {
        duer_ds_log_dcs_event_report_fail("ListenStarted");
    }

    if (data) {
        baidu_json_Delete(data);
    }

    return rs;
}

static int duer_dcs_report_listen_timeout_event(void)
{
    baidu_json *event = NULL;
    baidu_json *data = NULL;
    int rs = DUER_OK;

    data = baidu_json_CreateObject();
    if (data == NULL) {
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        rs = DUER_ERR_FAILED;
        goto RET;
    }

    event = duer_create_dcs_event(VOICE_INPUT_NAMESPACE,
                                  "ListenTimedOut",
                                  true);
    if (event == NULL) {
        rs = DUER_ERR_FAILED;
        goto RET;
    }

    baidu_json_AddItemToObject(data, "event", event);
    rs = duer_dcs_data_report_internal(data, false);
RET:
    if (rs != DUER_OK) {
        duer_ds_log_dcs_event_report_fail("ListenTimedOut");
    }

    if (data) {
        baidu_json_Delete(data);
    }

    return rs;
}

void duer_open_mic_internal(void)
{
    if ((s_wait_time != 0) && (s_listen_timer != NULL)) {
        duer_timer_start(s_listen_timer, s_wait_time);
    }
#ifndef DISABLE_OPEN_MIC_AUTOMATICALLY
    duer_dcs_listen_handler();
#endif
}

static duer_status_t duer_listen_start_cb(const baidu_json *directive)
{
    baidu_json *payload = NULL;
    baidu_json *timeout = NULL;
    baidu_json *initiator = NULL;

    payload = baidu_json_GetObjectItem(directive, "payload");
    if (!payload) {
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    duer_mutex_lock(s_mic_lock);
    if (s_initiator) {
        baidu_json_Delete(s_initiator);
        s_initiator = NULL;
    }

    initiator = baidu_json_GetObjectItem(payload, "initiator");
    if (initiator) {
        s_initiator = baidu_json_Duplicate(initiator, 1);
    }
    duer_mutex_unlock(s_mic_lock);

    timeout = baidu_json_GetObjectItem(payload, "timeoutInMilliseconds");
    if (!timeout) {
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    s_wait_time = timeout->valueint;
    s_is_multiple_dialog = true;
    DUER_LOGD("%d", s_wait_time);

    duer_play_channel_control_internal(DCS_NEED_OPEN_MIC);

    return DUER_OK;
}

static duer_status_t duer_listen_stop_cb(const baidu_json *directive)
{
    duer_mutex_lock(s_mic_lock);
#ifdef LIMIT_LISTEN_TIME
    if (s_is_listening && s_close_mic_timer) {
        duer_timer_stop(s_close_mic_timer);
    }
#endif
    if (s_is_listening) {
        s_is_listening = false;
        duer_dcs_stop_listen_handler();
    }
    duer_mutex_unlock(s_mic_lock);

    return DUER_OK;
}

int duer_dcs_on_listen_stopped(void)
{
    duer_mutex_lock(s_mic_lock);
#ifdef LIMIT_LISTEN_TIME
    if (s_is_listening && s_close_mic_timer) {
        duer_timer_stop(s_close_mic_timer);
    }
#endif
    s_is_listening = false;
    duer_mutex_unlock(s_mic_lock);
    return DUER_OK;
}

#ifdef LIMIT_LISTEN_TIME
static void duer_stop_listen_missing_handler(int what, void *param)
{
    duer_ds_log_dcs(DUER_DS_LOG_DCS_STOP_LISTEN_MISSING, NULL);

    duer_mutex_lock(s_mic_lock);
    if (s_is_listening) {
        s_is_listening = false;
        duer_dcs_stop_listen_handler();
    }
    duer_mutex_unlock(s_mic_lock);
}

static void duer_stop_listen_missing(void *param)
{
    duer_emitter_emit(duer_stop_listen_missing_handler, 0, NULL);
}
#endif

static void duer_listen_timeout_handler(int what, void *param)
{
    duer_mutex_lock(s_mic_lock);
    if (s_initiator) {
        baidu_json_Delete(s_initiator);
        s_initiator = NULL;
    }
    duer_mutex_unlock(s_mic_lock);
    duer_dcs_report_listen_timeout_event();
}

static void duer_listen_timeout(void *param)
{
    duer_emitter_emit(duer_listen_timeout_handler, 0, NULL);
}

bool duer_is_multiple_round_dialogue()
{
    return s_is_multiple_dialog;
}

void duer_dcs_voice_input_init(void)
{
    static bool is_first_time = true;

    if (!is_first_time) {
        return;
    }

    is_first_time = false;
#ifdef LIMIT_LISTEN_TIME
    s_close_mic_timer = duer_timer_acquire(duer_stop_listen_missing, NULL, DUER_TIMER_ONCE);
    if (!s_close_mic_timer) {
        DUER_LOGE("Failed to create timer to close mic");
    }
#endif

    s_listen_timer = duer_timer_acquire(duer_listen_timeout, NULL, DUER_TIMER_ONCE);
    if (!s_listen_timer) {
        DUER_LOGE("Failed to create listen timeout timer");
    }

    s_mic_lock = duer_mutex_create();
    if (!s_mic_lock) {
        DUER_LOGE("Failed to create s_mic_lock");
    }

    duer_directive_list res[] = {
        {"Listen", duer_listen_start_cb},
        {"StopListen", duer_listen_stop_cb},
    };

    duer_add_dcs_directive(res, sizeof(res) / sizeof(res[0]), VOICE_INPUT_NAMESPACE);
}

