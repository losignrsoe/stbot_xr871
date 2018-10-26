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
 * File: lightduer_dcs_audio_player.c
 * Auth: Gang Chen (chengang12@baidu.com)
 * Desc: apply APIs to support DCS audio player interface
 */

#include "lightduer_dcs.h"
#include <stdbool.h>
#include "lightduer_dcs_router.h"
#include "lightduer_dcs_local.h"
#include "lightduer_memory.h"
#include "lightduer_log.h"
#include "lightduer_ca.h"
#include "lightduer_queue_cache.h"
#include "lightduer_mutex.h"
#include "lightduer_connagent.h"
#include "lightduer_ds_log_dcs.h"
#include "lightduer_ds_log_e2e.h"
#include "lightduer_timers.h"
#include "lightduer_event_emitter.h"

typedef struct _play_item_t {
    char *url;
    char *token;
    char *audio_item_id;
    int offset;
    int report_delay;
    int report_interval;
    bool is_progress_delay_elapsed;
} play_item_t;

enum _play_state {
    PLAYING,
    STOPPED,
    PAUSED,
    BUFFER_UNDERRUN,
    FINISHED,
    MAX_PLAY_STATE,
};

typedef enum {
    DCS_PLAY_STARTTED,
    DCS_PLAY_FINISHED,
    DCS_PLAY_NEARLY_FINISHED,
    DCS_PLAY_STOPPED,
    DCS_PLAY_PAUSED,
    DCS_PLAY_RESUMED,
    DCS_PLAY_QUEUE_CLEARED,
    DCS_AUDIO_PLAY_FAILED,
    DCS_METADATA_EXTRACTED,
    DCS_PROGRESS_REPORT_DELAY,
    DCS_PROGRESS_REPORT_INTERVAL,
    DCS_STUTTER_STARTED,
    DCS_STUTTER_FINISHED,
    DCS_AUDIO_EVENT_NUMBER,
} duer_dcs_audio_event_t;

static const char* const s_player_activity[MAX_PLAY_STATE] = {"PLAYING", "STOPPED", "PAUSED",
                                                              "BUFFER_UNDERRUN", "FINISHED"};

static duer_qcache_handler s_play_queue = NULL;
static char *s_latest_token = NULL;
static const char* const s_event_name_tab[] = {"PlaybackStarted",
                                               "PlaybackFinished",
                                               "PlaybackNearlyFinished",
                                               "PlaybackStopped",
                                               "PlaybackPaused",
                                               "PlaybackResumed",
                                               "PlaybackQueueCleared",
                                               "PlaybackFailed",
                                               "StreamMetadataExtracted",
                                               "ProgressReportDelayElapsed",
                                               "ProgressReportIntervalElapsed",
                                               "PlaybackStutterStarted",
                                               "PlaybackStutterFinished"};

static const char* const s_audio_error_tab[] = {"MEDIA_ERROR_UNKNOWN",
                                                "MEDIA_ERROR_INVALID_REQUEST",
                                                "MEDIA_ERROR_SERVICE_UNAVAILABLE",
                                                "MEDIA_ERROR_INTERNAL_SERVER_ERROR",
                                                "MEDIA_ERROR_INTERNAL_DEVICE_ERROR"};

static volatile int s_play_state = FINISHED;
static volatile int s_play_offset = 0;
static duer_mutex_t s_queue_lock;
static const char *AUDIO_PLAYER_NAMESPACE = "ai.dueros.device_interface.audio_player";
static duer_timer_handler s_progress_report_timer;
static duer_timer_handler s_delay_elapse_report_timer;
static volatile bool s_is_stuttured_reported;

static void duer_destroy_play_item(play_item_t *item)
{
    if (!item) {
        return;
    }

    if (item->url) {
        DUER_FREE(item->url);
    }

    if (item->token) {
        DUER_FREE(item->token);
    }

    if (item->audio_item_id) {
        DUER_FREE(item->audio_item_id);
    }

    DUER_FREE(item);
}

static play_item_t *duer_create_play_item(const play_item_t *play_info)
{
    play_item_t *item = NULL;
    duer_status_t rs = DUER_OK;

    item = DUER_MALLOC(sizeof(play_item_t));
    if (!item) {
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        rs = DUER_ERR_MEMORY_OVERLOW;
        goto RET;
    }

    item->offset = play_info->offset;
    item->report_delay = play_info->report_delay;
    item->report_interval = play_info->report_interval;
    item->is_progress_delay_elapsed = play_info->is_progress_delay_elapsed;

    item->url = duer_strdup_internal(play_info->url);
    if (!item->url) {
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        rs = DUER_ERR_MEMORY_OVERLOW;
        goto RET;
    }

    item->token = duer_strdup_internal(play_info->token);
    if (!item->token) {
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        rs = DUER_ERR_MEMORY_OVERLOW;
        goto RET;
    }

    item->audio_item_id = duer_strdup_internal(play_info->audio_item_id);
    if (!item->audio_item_id) {
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        rs = DUER_ERR_MEMORY_OVERLOW;
        goto RET;
    }
RET:
    if (rs != DUER_OK) {
        duer_destroy_play_item(item);
        item = NULL;
    }

    return item;
}

static void duer_empty_play_queue()
{
    play_item_t *item = NULL;

    while ((item = duer_qcache_pop(s_play_queue)) != NULL) {
        duer_destroy_play_item(item);
    }
}

static baidu_json* duer_get_playback_state_internal()
{
    baidu_json *state = NULL;
    baidu_json *payload = NULL;

    state = duer_create_dcs_event("ai.dueros.device_interface.audio_player",
                                  "PlaybackState",
                                  false);
    if (!state) {
        DUER_LOGE("Failed to create playback state");
        goto error_out;
    }

    payload = baidu_json_GetObjectItem(state, "payload");
    if (!payload) {
        DUER_LOGE("Failed to get payload item");
        goto error_out;
    }

    baidu_json_AddStringToObject(payload, "token", s_latest_token ? s_latest_token : "");
    if (s_play_state == PLAYING || s_play_state == BUFFER_UNDERRUN) {
        s_play_offset = duer_dcs_audio_get_play_progress();
    }
    baidu_json_AddNumberToObject(payload,
                                 "offsetInMilliseconds",
                                 s_play_offset < 0 ? 0 : s_play_offset);
    baidu_json_AddStringToObject(payload, "playerActivity", s_player_activity[s_play_state]);

    return state;

error_out:
    if (state) {
        baidu_json_Delete(state);
    }

    duer_ds_log_dcs_event_report_fail("PlaybackState");

    return NULL;
}

static int duer_dcs_report_play_event(duer_dcs_audio_event_t type, baidu_json *msg)
{
    baidu_json *data = NULL;
    baidu_json *event = NULL;
    baidu_json *payload = NULL;
    int rs = DUER_OK;
    baidu_json *client_context = NULL;
    baidu_json *state = NULL;

    if ((int)type < 0 || type >= sizeof(s_event_name_tab) / sizeof(s_event_name_tab[0])) {
        DUER_DS_LOG_REPORT_DCS_PARAM_ERROR();
        DUER_LOGE("Invalid event type!");
        rs = DUER_ERR_INVALID_PARAMETER;
        goto RET;
    }

    if ((type != DCS_PLAY_QUEUE_CLEARED) && !s_latest_token) {
        DUER_LOGE("No token was stored!");
        rs = DUER_ERR_FAILED;
        goto RET;
    }

    data = baidu_json_CreateObject();
    if (data == NULL) {
        DUER_LOGE("Failed to create json object!");
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        rs = DUER_ERR_MEMORY_OVERLOW;
        goto RET;
    }

    event = duer_create_dcs_event(AUDIO_PLAYER_NAMESPACE,
                                  s_event_name_tab[type],
                                  true);
    if (event == NULL) {
        rs = DUER_ERR_FAILED;
        goto RET;
    }

    baidu_json_AddItemToObject(data, "event", event);

    if (type != DCS_PLAY_STARTTED) {
        s_play_offset = duer_dcs_audio_get_play_progress();
    }

    if (type != DCS_PLAY_QUEUE_CLEARED) {
        payload = baidu_json_GetObjectItem(event, "payload");
        if (!payload) {
            rs = DUER_ERR_FAILED;
            goto RET;
        }
        baidu_json_AddStringToObject(payload, "token", s_latest_token);
        baidu_json_AddNumberToObject(payload, "offsetInMilliseconds",
                                     s_play_offset > 0 ? s_play_offset : 0);
    }

    if (type == DCS_AUDIO_PLAY_FAILED) {
        baidu_json_AddItemReferenceToObject(payload, "error", msg);
        client_context = baidu_json_CreateArray();
        if (!client_context) {
            DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
            DUER_LOGE("Memory overlow");
            goto RET;
        }
        baidu_json_AddItemToObject(data, "clientContext", client_context);
        state = duer_get_playback_state_internal();
        if (state) {
            baidu_json_AddItemToArray(client_context, state);
        }
    }

    if (type == DCS_METADATA_EXTRACTED) {
        baidu_json_AddItemReferenceToObject(payload, "metadata", msg);
    }

    rs = duer_dcs_data_report_internal(data, true);

RET:
    if (data) {
        baidu_json_Delete(data);
    }

    if ((rs != DUER_OK) && (rs != DUER_ERR_INVALID_PARAMETER)) {
        duer_ds_log_dcs_event_report_fail(s_event_name_tab[type]);
    }

    return rs;
}

static void duer_delay_elapse_report(int what, void *object)
{
    duer_dcs_report_play_event(DCS_PROGRESS_REPORT_DELAY, NULL);
}

static void duer_delay_elapse_report_cb(void *param)
{
    play_item_t *first_item = NULL;

    duer_emitter_emit(duer_delay_elapse_report, 0, NULL);
    first_item = duer_qcache_top(s_play_queue);
    if (first_item) {
        first_item->is_progress_delay_elapsed = true;
    }
}

static void duer_start_progress_report()
{
    play_item_t *first_item = NULL;

    first_item = duer_qcache_top(s_play_queue);
    if (first_item == NULL) {
        return;
    }

    if (first_item->report_interval != 0) {
        duer_timer_start(s_progress_report_timer, first_item->report_interval);
    }

    if (first_item->is_progress_delay_elapsed) {
        return;
    }

    if (first_item->report_delay > 0) {
        if (first_item->offset + first_item->report_delay > s_play_offset) {
            duer_timer_start(s_delay_elapse_report_timer,
                             first_item->offset + first_item->report_delay - s_play_offset);
        } else {
            duer_delay_elapse_report_cb(NULL);
        }
    }
}

static void duer_stop_progress_report()
{
    play_item_t *first_item = NULL;

    first_item = duer_qcache_top(s_play_queue);
    if (first_item == NULL) {
        return;
    }

    if (first_item->report_interval != 0) {
        duer_timer_stop(s_progress_report_timer);
    }

    if (!first_item->is_progress_delay_elapsed) {
        duer_timer_stop(s_delay_elapse_report_timer);
    }
}

bool duer_audio_is_playing_internal(void)
{
    return (s_play_state == PLAYING) || (s_play_state == BUFFER_UNDERRUN);
}

bool duer_audio_is_paused_internal(void)
{
    return s_play_state == PAUSED;
}

void duer_start_audio_play_internal(void)
{
    play_item_t *first_item = NULL;
    duer_dcs_audio_info_t audio_info;

    first_item = duer_qcache_top(s_play_queue);
    if (!first_item || !first_item->token || !first_item->url) {
        s_play_state = FINISHED;
        if (!first_item) {
            duer_play_channel_control_internal(DCS_AUDIO_FINISHED);
        }
        return;
    }

    s_is_stuttured_reported = false;
    s_play_state = PLAYING;
    s_play_offset = first_item->offset;

    if (s_latest_token) {
        DUER_FREE(s_latest_token);
    }
    s_latest_token = duer_strdup_internal(first_item->token);

    audio_info.url = first_item->url;
    audio_info.offset = s_play_offset;
    audio_info.audio_item_id = first_item->audio_item_id;
    duer_dcs_audio_play_handler(&audio_info);
    duer_start_progress_report();
    duer_dcs_report_play_event(DCS_PLAY_STARTTED, NULL);
}

static void duer_replace_play_queue(const play_item_t *play_info)
{
    play_item_t *item = NULL;

    if (s_play_state == PLAYING || s_play_state == PAUSED || s_play_state == BUFFER_UNDERRUN) {
        duer_stop_progress_report();
        duer_dcs_audio_stop_handler();
        duer_dcs_report_play_event(DCS_PLAY_STOPPED, NULL);
    }

    duer_mutex_lock(s_queue_lock);

    duer_empty_play_queue();
    item = duer_create_play_item(play_info);
    duer_qcache_push(s_play_queue, item);
    duer_play_channel_control_internal(DCS_AUDIO_NEED_PLAY);

    duer_mutex_unlock(s_queue_lock);
}

static void duer_enqueue_play_item(const play_item_t *play_info)
{
    play_item_t *item = NULL;

    item = duer_create_play_item(play_info);

    duer_mutex_lock(s_queue_lock);

    duer_qcache_push(s_play_queue, item);

    if (s_play_state == FINISHED) {
        duer_play_channel_control_internal(DCS_AUDIO_NEED_PLAY);
    }

    duer_mutex_unlock(s_queue_lock);
}

static void duer_replace_enqueued_item(const play_item_t *play_info)
{
    play_item_t *item = NULL;

    duer_mutex_lock(s_queue_lock);

    item = duer_qcache_pop(s_play_queue);
    duer_empty_play_queue();
    duer_qcache_push(s_play_queue, item);

    item = duer_create_play_item(play_info);
    duer_qcache_push(s_play_queue, item);

    if (s_play_state == FINISHED) {
        duer_play_channel_control_internal(DCS_AUDIO_NEED_PLAY);
    }

    duer_mutex_unlock(s_queue_lock);
}

static duer_status_t duer_audio_play_cb(const baidu_json *directive)
{
    baidu_json *payload = NULL;
    baidu_json *url = NULL;
    baidu_json *token = NULL;
    baidu_json *stream = NULL;
    baidu_json *audio_item = NULL;
    duer_status_t ret = DUER_OK;
    baidu_json *behavior = NULL;
    baidu_json *offset = NULL;
    baidu_json *progress_report = NULL;
    baidu_json *audio_item_id = NULL;
    baidu_json *tmp = NULL;
    play_item_t play_item;

    DUER_LOGV("Enter");

    duer_ds_e2e_event(DUER_E2E_RESPONSE);

    payload = baidu_json_GetObjectItem(directive, "payload");
    if (!payload) {
        DUER_LOGE("Failed to parse payload\n");
        ret = DUER_MSG_RSP_BAD_REQUEST;
        goto RET;
    }

    audio_item = baidu_json_GetObjectItem(payload, "audioItem");
    behavior = baidu_json_GetObjectItem(payload, "playBehavior");
    if (!audio_item || !behavior) {
        DUER_LOGE("Failed to parse directive\n");
        ret = DUER_MSG_RSP_BAD_REQUEST;
        goto RET;
    }

    stream = baidu_json_GetObjectItem(audio_item, "stream");
    if (!stream) {
        DUER_LOGE("Failed to parse stream\n");
        ret = DUER_MSG_RSP_BAD_REQUEST;
        goto RET;
    }

    audio_item_id = baidu_json_GetObjectItem(audio_item, "audioItemId");
    token = baidu_json_GetObjectItem(stream, "token");
    url = baidu_json_GetObjectItem(stream, "url");
    offset = baidu_json_GetObjectItem(stream, "offsetInMilliseconds");
    if (!token || !url || !offset || !token->valuestring || !url->valuestring || !audio_item_id) {
        DUER_LOGE("Failed to parse directive\n");
        ret = DUER_MSG_RSP_BAD_REQUEST;
        goto RET;
    }

    play_item.offset = offset->valueint;
    play_item.url = url->valuestring;
    play_item.token = token->valuestring;
    play_item.report_delay = 0;
    play_item.report_interval = 0;
    play_item.is_progress_delay_elapsed = false;
    play_item.audio_item_id = audio_item_id->valuestring;

    progress_report = baidu_json_GetObjectItem(stream, "progressReport");
    if (progress_report) {
        tmp = baidu_json_GetObjectItem(progress_report, "progressReportDelayInMilliseconds");
        if (tmp) {
            play_item.report_delay = tmp->valueint;
            if (play_item.report_delay == 0) {
                play_item.is_progress_delay_elapsed = true;
            }
        }

        tmp = baidu_json_GetObjectItem(progress_report, "progressReportIntervalInMilliseconds");
        if (tmp) {
            play_item.report_interval = tmp->valueint;
        }
    }

    DUER_LOGD("url: %s", DUER_STRING_OUTPUT(url->valuestring));
    DUER_LOGI("token: %s", DUER_STRING_OUTPUT(token->valuestring));
    DUER_LOGD("behavior: %s", DUER_STRING_OUTPUT(behavior->valuestring));
    DUER_LOGD("offset: %d", offset->valueint);

    if (strcmp(behavior->valuestring, "REPLACE_ALL") == 0) {
        duer_replace_play_queue(&play_item);
    } else if (strcmp(behavior->valuestring, "ENQUEUE") == 0) {
        duer_enqueue_play_item(&play_item);
    } else if (strcmp(behavior->valuestring, "REPLACE_ENQUEUED") == 0) {
        duer_replace_enqueued_item(&play_item);
    } else {
        DUER_LOGE("Invalid playBehavior\n");
        ret = DUER_MSG_RSP_BAD_REQUEST;
        goto RET;
    }

RET:
    return ret;
}

static duer_status_t duer_queue_clear_cb(const baidu_json *directive)
{
    baidu_json *payload = NULL;
    baidu_json *behavior = NULL;
    duer_status_t ret = DUER_OK;
    play_item_t *item = NULL;

    DUER_LOGV("Enter");

    payload = baidu_json_GetObjectItem(directive, "payload");
    if (!payload) {
        DUER_LOGE("Failed to parse payload\n");
        ret = DUER_MSG_RSP_BAD_REQUEST;
        goto RET;
    }

    behavior = baidu_json_GetObjectItem(payload, "clearBehavior");
    if (!behavior) {
        DUER_LOGE("Failed to parse clearBehavior\n");
        ret = DUER_MSG_RSP_BAD_REQUEST;
        goto RET;
    }

    if (strcmp(behavior->valuestring, "CLEAR_ENQUEUED") == 0) {
        duer_mutex_lock(s_queue_lock);
        item = duer_qcache_pop(s_play_queue);
        duer_empty_play_queue();
        duer_qcache_push(s_play_queue, item);
        duer_mutex_unlock(s_queue_lock);
        duer_dcs_report_play_event(DCS_PLAY_QUEUE_CLEARED, NULL);
    } else if (strcmp(behavior->valuestring, "CLEAR_ALL") == 0) {
        if (s_play_state == PLAYING || s_play_state == PAUSED || s_play_state == BUFFER_UNDERRUN) {
            duer_stop_progress_report();
            duer_dcs_audio_stop_handler();
            s_play_state = STOPPED;
            duer_dcs_report_play_event(DCS_PLAY_STOPPED, NULL);
        }
        duer_mutex_lock(s_queue_lock);
        duer_empty_play_queue();
        duer_mutex_unlock(s_queue_lock);
        duer_dcs_report_play_event(DCS_PLAY_QUEUE_CLEARED, NULL);
    } else {
        DUER_LOGE("Invalid clearBehavior\n");
        ret = DUER_MSG_RSP_BAD_REQUEST;
        goto RET;
    }

RET:
    return ret;
}

static duer_status_t duer_audio_stop_cb(const baidu_json *directive)
{
    if (s_play_state == PLAYING || s_play_state == PAUSED || s_play_state == BUFFER_UNDERRUN) {
        s_play_offset = duer_dcs_audio_get_play_progress();
        duer_stop_progress_report();
        duer_dcs_audio_stop_handler();
        s_play_state = STOPPED;
        duer_dcs_report_play_event(DCS_PLAY_STOPPED, NULL);
    }

    duer_play_channel_control_internal(DCS_AUDIO_STOPPED);

    return DUER_OK;
}

void duer_dcs_audio_on_stopped(void)
{
    if (s_play_state == PLAYING || s_play_state == PAUSED || s_play_state == BUFFER_UNDERRUN) {
        duer_stop_progress_report();
        s_play_state = STOPPED;
        duer_dcs_report_play_event(DCS_PLAY_STOPPED, NULL);
    }
}

void duer_pause_audio_internal(bool is_breaking)
{
    s_play_state = PAUSED;
    if (is_breaking) {
        duer_stop_progress_report();
        duer_dcs_audio_pause_handler();
        duer_dcs_report_play_event(DCS_PLAY_PAUSED, NULL);
    } else {
        s_play_offset = -1;
    }
}

void duer_resume_audio_internal()
{
    play_item_t *first_item = NULL;
    duer_dcs_audio_info_t audio_info;

    s_play_state = PLAYING;

    if (s_play_offset == -1) {
        // This audio had not been played
        duer_mutex_lock(s_queue_lock);
        duer_start_audio_play_internal();
        duer_mutex_unlock(s_queue_lock);
    } else {
        duer_mutex_lock(s_queue_lock);
        first_item = duer_qcache_top(s_play_queue);
        if (!first_item || !first_item->token || !first_item->url) {
            s_play_state = FINISHED;
            duer_mutex_unlock(s_queue_lock);
            return;
        }

        audio_info.audio_item_id = first_item->audio_item_id;
        audio_info.url = first_item->url;
        audio_info.offset = s_play_offset;
        duer_dcs_audio_resume_handler(&audio_info);
        duer_start_progress_report();
        duer_mutex_unlock(s_queue_lock);
        duer_dcs_report_play_event(DCS_PLAY_RESUMED, NULL);
    }
}

void duer_dcs_audio_on_finished()
{
    play_item_t *item;

    if (s_play_state != PLAYING) {
        return;
    }

    duer_stop_progress_report();

    duer_dcs_report_play_event(DCS_PLAY_NEARLY_FINISHED, NULL);
    duer_dcs_report_play_event(DCS_PLAY_FINISHED, NULL);

    s_play_offset = 0;

    duer_mutex_lock(s_queue_lock);

    item = duer_qcache_pop(s_play_queue);
    duer_destroy_play_item(item);

    duer_start_audio_play_internal();
    duer_mutex_unlock(s_queue_lock);
}

int duer_dcs_audio_report_metadata(baidu_json *metadata)
{
    if (!metadata) {
        DUER_DS_LOG_REPORT_DCS_PARAM_ERROR();
        return DUER_ERR_INVALID_PARAMETER;
    }

    return duer_dcs_report_play_event(DCS_METADATA_EXTRACTED, metadata);
}

int duer_dcs_audio_play_failed(duer_dcs_audio_error_t type, const char *msg)
{
    int rs = DUER_OK;
    baidu_json *error_msg = NULL;

    if ((int)type < 0 || type > sizeof(s_audio_error_tab) / sizeof(s_audio_error_tab[0])) {
        rs = DUER_ERR_INVALID_PARAMETER;
        DUER_DS_LOG_REPORT_DCS_PARAM_ERROR();
        goto RET;
    }

    duer_stop_progress_report();
    error_msg = baidu_json_CreateObject();
    if (!error_msg) {
        rs = DUER_ERR_MEMORY_OVERLOW;
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        DUER_LOGE("Memory not enough");
        goto RET;
    }

    baidu_json_AddStringToObject(error_msg, "type", s_audio_error_tab[type]);
    if (msg) {
        baidu_json_AddStringToObject(error_msg, "message", msg);
    } else {
        baidu_json_AddStringToObject(error_msg, "message", "");
    }

    rs = duer_dcs_report_play_event(DCS_AUDIO_PLAY_FAILED, error_msg);

RET:
    if ((rs != DUER_OK) && (rs != DUER_ERR_INVALID_PARAMETER)) {
        duer_ds_log_dcs_event_report_fail(s_audio_error_tab[type]);
    }

    if (error_msg) {
        baidu_json_Delete(error_msg);
    }

    return rs;
}

static void duer_progress_report(int what, void *object)
{
    duer_dcs_report_play_event(DCS_PROGRESS_REPORT_INTERVAL, NULL);
}

static void duer_progress_report_cb(void *param)
{
    duer_emitter_emit(duer_progress_report, 0 ,NULL);
}

int duer_dcs_audio_on_stuttered(bool is_stuttered)
{
    int rs = DUER_OK;
    duer_dcs_audio_event_t event;

    if (s_is_stuttured_reported) {
        return rs;
    }

    if (is_stuttered) {
        if (s_play_state == PLAYING) {
            s_play_state = BUFFER_UNDERRUN;
            event = DCS_STUTTER_STARTED;
            rs = duer_dcs_report_play_event(event, NULL);
        }
    } else {
        if (s_play_state == BUFFER_UNDERRUN) {
            s_play_state = PLAYING;
            event = DCS_STUTTER_FINISHED;
            s_is_stuttured_reported = true;
            rs = duer_dcs_report_play_event(event, NULL);
        }
    }

    return rs;
}

void duer_dcs_audio_player_init(void)
{
    static bool is_first_time = true;

    if (!is_first_time) {
        return;
    }

    is_first_time = false;

    s_play_queue = duer_qcache_create();
    if (!s_play_queue) {
        DUER_LOGE("Failed to create play queue");
    }

    s_queue_lock = duer_mutex_create();
    if (!s_queue_lock) {
        DUER_LOGE("Failed to create queue lock");
    }

    s_progress_report_timer = duer_timer_acquire(duer_progress_report_cb, NULL, DUER_TIMER_PERIODIC);
    if (!s_progress_report_timer) {
        DUER_LOGE("Failed to create progress report timer");
    }

    s_delay_elapse_report_timer = duer_timer_acquire(duer_delay_elapse_report_cb,
                                                     NULL,
                                                     DUER_TIMER_ONCE);
    if (!s_delay_elapse_report_timer) {
        DUER_LOGE("Failed to create delay elapse report timer");
    }

    duer_directive_list res[] = {
        {"Play", duer_audio_play_cb},
        {"Stop", duer_audio_stop_cb},
        {"ClearQueue", duer_queue_clear_cb},
    };

    duer_add_dcs_directive(res, sizeof(res) / sizeof(res[0]), AUDIO_PLAYER_NAMESPACE);
    duer_reg_client_context_cb(duer_get_playback_state_internal);
}

