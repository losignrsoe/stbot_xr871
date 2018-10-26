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
 * File: lightduer_dcs_local.c
 * Auth: Gang Chen (chengang12@baidu.com)
 * Desc: Provide some functions for dcs module local.
 */

#include "lightduer_dcs_local.h"
#include <stdlib.h>
#include "lightduer_log.h"
#include "lightduer_memory.h"
#include "lightduer_ds_log_dcs.h"
#include "lightduer_mutex.h"
#include "lightduer_random.h"
#include "lightduer_voice.h"
#include "lightduer_connagent.h"

static volatile uint32_t s_dialog_req_id = 0;
static duer_mutex_t s_play_channel_lock;

int duer_dcs_data_report_internal(baidu_json *data, bool is_transparent)
{
    int rs = DUER_OK;

    if (!data) {
        return DUER_ERR_INVALID_PARAMETER;
    }

    if (!is_transparent) {
        duer_add_translate_flag(data);
    }

    rs = duer_data_report(data);

    return rs;
}

uint32_t duer_create_request_id_internal()
{
    s_dialog_req_id = (uint32_t)duer_random();
    duer_cancel_caching_directive();
    return s_dialog_req_id;
}

uint32_t duer_get_request_id_internal()
{
    return s_dialog_req_id;
}

char *duer_strdup_internal(const char *str)
{
    int len = 0;
    char *dest = NULL;

    if (!str) {
        return NULL;
    }

    len = strlen(str);
    dest = DUER_MALLOC(len + 1);
    if (!dest) {
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        return NULL;
    }

    snprintf(dest, len + 1, "%s", str);
    return dest;
}

void duer_play_channel_control_internal(duer_dcs_channel_switch_event_t event)
{
    duer_mutex_lock(s_play_channel_lock);

    switch (event) {
    case DCS_RECORD_STARTED:
        duer_speech_on_stop_internal();
        if (duer_audio_is_playing_internal()) {
            duer_pause_audio_internal(true);
        }
        break;
    case DCS_SPEECH_NEED_PLAY:
        if (duer_audio_is_playing_internal()) {
            duer_pause_audio_internal(true);
        }
        break;
    case DCS_SPEECH_FINISHED:
        if (duer_is_multiple_round_dialogue()) {
            duer_open_mic_internal();
        } else if (duer_audio_is_paused_internal()) {
            duer_resume_audio_internal();
        } else {
            // do nothing
        }
        break;
    case DCS_AUDIO_NEED_PLAY:
        if (duer_speech_need_play_internal() || duer_is_recording_internal()) {
            duer_pause_audio_internal(false);
        } else {
            duer_start_audio_play_internal();
        }
        break;
    case DCS_DIALOG_FINISHED:
        if (duer_speech_need_play_internal()
            || duer_is_recording_internal()
            || duer_is_multiple_round_dialogue()) {
            break;
        }
        if (duer_audio_is_paused_internal()) {
            duer_resume_audio_internal();
        }
        break;
    case DCS_AUDIO_FINISHED:
        break;
    case DCS_NEED_OPEN_MIC:
        if (duer_speech_need_play_internal()) {
            break;
        }
        if (duer_audio_is_playing_internal()) {
            duer_pause_audio_internal(true);
        }
        duer_open_mic_internal();
        break;
    case DCS_AUDIO_STOPPED:
        break;
    default:
        break;
    }

    duer_mutex_unlock(s_play_channel_lock);
}

void duer_dcs_local_init_internal(void)
{
    if (!s_play_channel_lock) {
        s_play_channel_lock = duer_mutex_create();
        if (!s_play_channel_lock) {
            DUER_LOGE("Failed to create play channel lock");
        }
    }
}
