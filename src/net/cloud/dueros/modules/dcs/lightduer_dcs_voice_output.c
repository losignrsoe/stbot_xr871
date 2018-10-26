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
 * File: lightduer_dcs_voice_output.c
 * Auth: Gang Chen (chengang12@baidu.com)
 * Desc: apply APIs to support DCS voice output interface
 */

#include "lightduer_dcs.h"
#include "lightduer_dcs_router.h"
#include "lightduer_dcs_local.h"
#include "lightduer_connagent.h"
#include "lightduer_memory.h"
#include "lightduer_log.h"
#include "lightduer_ds_log_dcs.h"
#include "lightduer_ds_log_e2e.h"

static char *s_latest_token = NULL;

enum _play_state {
    PLAYING,
    FINISHED,
    MAX_PLAY_STATE,
};

static const char *VOICE_OUTPUT_NAMESPACE = "ai.dueros.device_interface.voice_output";
static const char* const s_player_activity[MAX_PLAY_STATE] = {"PLAYING", "FINISHED"};
static volatile int s_play_state = FINISHED;

static int duer_report_speech_event(const char *name)
{
    baidu_json *data = NULL;
    baidu_json *event = NULL;
    baidu_json *payload = NULL;
    int rs = DUER_OK;

    if (!s_latest_token) {
        DUER_LOGW("No token was stored!");
        rs = DUER_ERR_FAILED;
        goto RET;
    }

    data = baidu_json_CreateObject();
    if (data == NULL) {
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        rs = DUER_ERR_MEMORY_OVERLOW;
        goto RET;
    }

    event = duer_create_dcs_event(VOICE_OUTPUT_NAMESPACE,
                                  name,
                                  true);
    if (event == NULL) {
        rs = DUER_ERR_FAILED;
        goto RET;
    }

    baidu_json_AddItemToObject(data, "event", event);
    payload = baidu_json_GetObjectItem(event, "payload");
    if (!payload) {
        rs = DUER_ERR_FAILED;
        goto RET;
    }
    baidu_json_AddStringToObject(payload, "token", s_latest_token);

    duer_dcs_data_report_internal(data, false);

RET:
    if (rs != DUER_OK) {
        duer_ds_log_dcs_event_report_fail(name);
    }

    if (data) {
        baidu_json_Delete(data);
    }

    return rs;
}

static int duer_report_speech_started_event()
{
    return duer_report_speech_event("SpeechStarted");
}

static int duer_report_speech_finished_event()
{
    return duer_report_speech_event("SpeechFinished");
}

extern void duer_speak_directive_finished();

void duer_dcs_speech_on_finished()
{
    duer_speak_directive_finished();

    if (s_play_state == PLAYING) {
        s_play_state = FINISHED;
        duer_report_speech_finished_event();
    }
    duer_play_channel_control_internal(DCS_SPEECH_FINISHED);
}

void duer_speech_on_stop_internal()
{
    s_play_state = FINISHED;
}

static duer_status_t duer_speak_cb(const baidu_json *directive)
{
    baidu_json *payload = NULL;
    baidu_json *url = NULL;
    baidu_json *token = NULL;

    DUER_LOGV("Entry");

    duer_ds_e2e_event(DUER_E2E_RESPONSE);

    payload = baidu_json_GetObjectItem(directive, "payload");
    if (!payload) {
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    url = baidu_json_GetObjectItem(payload, "url");
    token = baidu_json_GetObjectItem(payload, "token");

    if (!url || !token) {
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    if (s_latest_token) {
        DUER_FREE(s_latest_token);
    }
    s_latest_token = duer_strdup_internal(token->valuestring);
    if (!s_latest_token) {
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        DUER_LOGE("Memory too low");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    s_play_state = PLAYING;

    duer_play_channel_control_internal(DCS_SPEECH_NEED_PLAY);
    duer_report_speech_started_event();

    duer_dcs_speak_handler(url->valuestring);

    return DUER_OK;
}

static duer_status_t duer_stop_speak_cb(const baidu_json *directive)
{
    duer_dcs_stop_speak_handler();
    if (s_play_state == PLAYING) {
        s_play_state = FINISHED;
        duer_play_channel_control_internal(DCS_SPEECH_FINISHED);
    }

    return DUER_OK;
}

bool duer_speech_need_play_internal(void)
{
    return s_play_state == PLAYING;
}

baidu_json* duer_get_speech_state_internal()
{
    baidu_json *state = NULL;
    baidu_json *payload = NULL;

    state = duer_create_dcs_event(VOICE_OUTPUT_NAMESPACE, "SpeechState", false);
    if (!state) {
        DUER_LOGE("Filed to create SpeechState event");
        goto error_out;
    }

    payload = baidu_json_GetObjectItem(state, "payload");
    if (!payload) {
        DUER_LOGE("Filed to get payload item");
        goto error_out;
    }

    baidu_json_AddStringToObject(payload, "token", s_latest_token ? s_latest_token : "");
    baidu_json_AddNumberToObject(payload, "offsetInMilliseconds", 0);
    baidu_json_AddStringToObject(payload, "playerActivity", s_player_activity[s_play_state]);

    return state;

error_out:
    if (state) {
        baidu_json_Delete(state);
    }

    return NULL;
}

void duer_dcs_voice_output_init(void)
{
    static bool is_first_time = true;

    if (!is_first_time) {
        return;
    }

    is_first_time = false;

    duer_directive_list res[] = {
        {"Speak", duer_speak_cb},
        {"StopSpeak", duer_stop_speak_cb},
    };

    duer_add_dcs_directive(res, sizeof(res) / sizeof(res[0]), VOICE_OUTPUT_NAMESPACE);
    duer_reg_client_context_cb(duer_get_speech_state_internal);
}

