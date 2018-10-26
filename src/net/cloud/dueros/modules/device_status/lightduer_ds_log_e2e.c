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
/*
 * Auth: Su Hao(suhao@baidu.com)
 * Desc: Record the end to end delay
 */

#include "lightduer_ds_log_e2e.h"
#include "lightduer_log.h"
#include "lightduer_timestamp.h"
#include "lightduer_lib.h"
#include "lightduer_ds_log.h"
#include "baidu_json.h"

#ifdef DUER_STATISTICS_E2E

enum {
    DUER_DS_LOG_END2END_DELAY = 0x101
};

typedef struct _duer_ds_e2e_struct {
    duer_ds_e2e_event_t _event;
    duer_u32_t          _timestamp[DUER_E2E_EVENT_TOTAL];
    duer_u32_t          _segment;
} duer_ds_e2e_t;

static duer_ds_e2e_t g_ds_e2e = {0};

static const char * const g_tags[DUER_E2E_EVENT_TOTAL - 1] = {
    "request",
    "response",
    "play"
};

static void duer_ds_e2e_result(void) {
    baidu_json *msg = NULL;
    int i;

    msg = baidu_json_CreateObject();
    if (msg == NULL) {
        DUER_LOGE("Memory overflow!");
        return;
    }

    for (i = 0; i < DUER_E2E_EVENT_TOTAL - 1; i++) {
        baidu_json_AddNumberToObject(msg, g_tags[i], g_ds_e2e._timestamp[i + 1] - g_ds_e2e._timestamp[i]);
    }

    duer_ds_log(DUER_DS_LOG_LEVEL_INFO, DUER_DS_LOG_MODULE_ANALYSIS, DUER_DS_LOG_END2END_DELAY, msg);
}

void duer_ds_e2e_update_latest_request(duer_ds_e2e_event_t evt, duer_u32_t segment) {
    static duer_ds_e2e_t backup;

    if (evt == DUER_E2E_REQUEST && segment == 0) {
        DUER_MEMSET(&g_ds_e2e, 0, sizeof(g_ds_e2e));
        DUER_MEMSET(&backup, 0, sizeof(backup));
    }

    DUER_LOGD("event %d -> %d", backup._event, evt);

    if (evt > DUER_E2E_SENT || evt < 0) {
        DUER_LOGE("error event!!!");
        return;
    } else if (g_ds_e2e._event > DUER_E2E_SENT) {
        DUER_LOGW("Skip the dropped event");
        return;
    } else if (evt == DUER_E2E_SENT && backup._segment != segment) {
        DUER_LOGW("Skip the dropped segment");
        return;
    }

    backup._event = evt;
    backup._timestamp[evt] = duer_timestamp();

    if (evt == DUER_E2E_REQUEST) {
        backup._segment = segment;
    } else {
        DUER_MEMCPY(&g_ds_e2e, &backup, sizeof(g_ds_e2e));
    }
}

void duer_ds_e2e_event(duer_ds_e2e_event_t evt) {
    if (evt >= DUER_E2E_EVENT_TOTAL || evt < 0) {
        DUER_LOGE("error event!!!");
        return;
    }

    DUER_LOGD("event %d -> %d", g_ds_e2e._event, evt);

    if (evt > 0 && evt != g_ds_e2e._event + 1) {
        DUER_LOGI("event %d -> %d, invalid in e2e statistic!!!", g_ds_e2e._event, evt);
        return;
    }

    if (evt == DUER_E2E_REQUEST) {
        DUER_MEMSET(&g_ds_e2e, 0, sizeof(g_ds_e2e));
    }

    g_ds_e2e._event = evt;
    g_ds_e2e._timestamp[evt] = duer_timestamp();

    if (evt == DUER_E2E_PLAY) {
        duer_ds_e2e_result();
    }
}

#endif