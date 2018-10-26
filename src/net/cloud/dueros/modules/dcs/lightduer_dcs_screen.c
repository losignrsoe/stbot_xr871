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
 * File: lightduer_dcs_screen.c
 * Auth: Gang Chen (chengang12@baidu.com)
 * Desc: apply APIs to support DCS screen interface
 */

#include "lightduer_dcs.h"
#include "lightduer_dcs_router.h"
#include "lightduer_dcs_local.h"
#include "lightduer_connagent.h"
#include "lightduer_log.h"
#include "lightduer_ds_log_dcs.h"

static const char *SCREEN_NAMESPACE = "ai.dueros.device_interface.screen";

static duer_status_t duer_render_input_text_cb(const baidu_json *directive)
{
    baidu_json *payload = NULL;
    baidu_json *text = NULL;
    baidu_json *type = NULL;
    duer_status_t ret = DUER_OK;

    DUER_LOGV("Entry");

    payload = baidu_json_GetObjectItem(directive, "payload");
    if (!payload) {
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    text = baidu_json_GetObjectItem(payload, "text");
    type = baidu_json_GetObjectItem(payload, "type");
    if (!text || !type) {
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    ret = duer_dcs_input_text_handler(text->valuestring, type->valuestring);

    return ret;
}

static duer_status_t duer_render_card_cb(const baidu_json *directive)
{
    baidu_json *payload = NULL;
    duer_status_t ret = DUER_OK;

    DUER_LOGV("Entry");

    payload = baidu_json_GetObjectItem(directive, "payload");
    if (!payload) {
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    ret = duer_dcs_render_card_handler(payload);

    return ret;
}

int duer_dcs_on_link_clicked(const char *url)
{
    baidu_json *data = NULL;
    baidu_json *event = NULL;
    baidu_json *payload = NULL;
    int rs = DUER_OK;

    do {
        if (!url) {
            rs = DUER_ERR_INVALID_PARAMETER;
            DUER_LOGE("Invalid param: url is null");
            break;
        }

        data = baidu_json_CreateObject();
        if (data == NULL) {
            rs = DUER_ERR_MEMORY_OVERLOW;
            DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
            break;
        }

        event = duer_create_dcs_event(SCREEN_NAMESPACE,
                                      "LinkClicked",
                                      true);
        if (event == NULL) {
            rs = DUER_ERR_FAILED;
            break;
        }

        baidu_json_AddItemToObject(data, "event", event);

        payload = baidu_json_GetObjectItem(event, "payload");
        if (!payload) {
            rs = DUER_ERR_FAILED;
            break;
        }
        baidu_json_AddStringToObject(payload, "url", url);

        rs = duer_dcs_data_report_internal(data, false);
    } while (0);

    if (data) {
        baidu_json_Delete(data);
    }

    return rs;
}

static duer_status_t duer_render_player_info_cb(const baidu_json *directive)
{
	baidu_json *payload = NULL;
	duer_status_t ret = DUER_OK;

	payload = baidu_json_GetObjectItem(directive, "payload");
	if (!payload) {
		return DUER_MSG_RSP_BAD_REQUEST;
	}
	
	ret = duer_dcs_render_player_info_handler(payload);

	return ret;
}

void duer_dcs_screen_init(void)
{
    static bool is_first_time = true;

    if (!is_first_time) {
        return;
    }

    is_first_time = false;

    duer_directive_list res[] = {
        {"RenderVoiceInputText", duer_render_input_text_cb},
        {"RenderCard", duer_render_card_cb},
    };

    duer_add_dcs_directive(res, sizeof(res) / sizeof(res[0]), SCREEN_NAMESPACE);
}

void duer_dcs_screen_info_init(void)
{
	static bool is_first_time = true;

    if (!is_first_time) {
        return;
    }
	is_first_time = false;

	duer_directive_list res[] = {
        {"RenderPlayerInfo",duer_render_player_info_cb},
    };

	duer_add_dcs_directive(res, sizeof(res) / sizeof(res[0]), 
					"ai.dueros.device_interface.screen_extended_card");
}

