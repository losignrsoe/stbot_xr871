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
 * File: lightduer_dcs_router.c
 * Auth: Gang Chen (chengang12@baidu.com)
 * Desc: Light duer dcs directive router.
 */

#include "lightduer_dcs_router.h"
#include "lightduer_dcs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lightduer_dcs_local.h"
#include "lightduer_log.h"
#include "lightduer_memory.h"
#include "lightduer_connagent.h"
#include "lightduer_lib.h"
#include "lightduer_ca.h"
#include "lightduer_mutex.h"
#include "lightduer_ds_log_dcs.h"
#include "lightduer_timestamp.h"
#include "lightduer_random.h"
#include "lightduer_queue_cache.h"
#include "lightduer_event_emitter.h"

#define MSG_ID_LENGTH 36

typedef struct {
    const char *name_space;
    duer_directive_list *list;
    size_t count;
} directive_namespace_list_t;

static volatile size_t s_namespace_count;
static volatile directive_namespace_list_t *s_directive_namespace_list;
static duer_mutex_t s_dcs_router_lock;
static volatile size_t s_client_context_cb_count;
static volatile dcs_client_context_handler *s_dcs_client_context_cb;
static duer_mutex_t s_dcs_client_contex_cb_lock;
static duer_qcache_handler s_directive_queue;
static duer_mutex_t s_directive_queue_lock;

extern int duer_dcs_close_multi_dialog(void);

duer_status_t duer_reg_client_context_cb(dcs_client_context_handler cb)
{
    size_t size = 0;
    duer_status_t ret = DUER_OK;
    dcs_client_context_handler *list = NULL;

    if (!cb) {
        DUER_LOGE("Failed to add client context handler: param error\n");
        DUER_DS_LOG_REPORT_DCS_PARAM_ERROR();
        ret = DUER_ERR_INVALID_PARAMETER;
        return ret;
    }

    duer_mutex_lock(s_dcs_client_contex_cb_lock);
    if (!s_dcs_client_context_cb) {
        s_dcs_client_context_cb = DUER_MALLOC(sizeof(dcs_client_context_handler));
        if (!s_dcs_client_context_cb) {
            duer_mutex_unlock(s_dcs_client_contex_cb_lock);
            DUER_LOGE("Failed to add dcs client context cb: no memory\n");
            DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
            ret = DUER_ERR_MEMORY_OVERLOW;
            return ret;
        }
    } else {
        size = sizeof(dcs_client_context_handler) * (s_client_context_cb_count + 1);
        list = (dcs_client_context_handler *)DUER_REALLOC((void *)s_dcs_client_context_cb, size);
        if (list) {
            s_dcs_client_context_cb = list;
        } else {
            duer_mutex_unlock(s_dcs_client_contex_cb_lock);
            DUER_LOGE("Failed to add dcs client context cb: no memory\n");
            DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
            ret = DUER_ERR_MEMORY_OVERLOW;
            return ret;
        }
    }

    s_dcs_client_context_cb[s_client_context_cb_count] = cb;
    s_client_context_cb_count++;
    duer_mutex_unlock(s_dcs_client_contex_cb_lock);

    return ret;
}

static void duer_generate_msg_id(char *buf)
{
    int eof_pos = 0;
    int rand_num = 0;
    static int count = 0;

    srand(duer_timestamp());
    rand_num = rand();
    snprintf(buf + eof_pos, MSG_ID_LENGTH - eof_pos + 1, "%08x-", rand_num);
    eof_pos += 9;

    srand(++count);
    rand_num = rand();
    snprintf(buf + eof_pos, MSG_ID_LENGTH - eof_pos + 1, "%04x-", rand_num & 0xffff);
    eof_pos += 5;

    srand(duer_get_request_id_internal());
    rand_num = rand();
    snprintf(buf + eof_pos, MSG_ID_LENGTH - eof_pos + 1, "%04x-", rand_num & 0xffff);
    eof_pos += 5;

    rand_num = duer_random();
    snprintf(buf + eof_pos, MSG_ID_LENGTH - eof_pos + 1, "%04x-%04x",
             (rand_num >> 16) & 0xffff, rand_num & 0xffff);
    eof_pos += 9;

    rand_num = duer_random();
    snprintf(buf + eof_pos, MSG_ID_LENGTH - eof_pos + 1, "%08x", rand_num);
}

baidu_json *duer_create_dcs_event(const char *name_space, const char *name, bool need_msg_id)
{
    baidu_json *event = NULL;
    baidu_json *header = NULL;
    baidu_json *payload = NULL;
    char msg_id[MSG_ID_LENGTH + 1];

    event = baidu_json_CreateObject();
    if (event == NULL) {
        goto error_out;
    }

    header = baidu_json_CreateObject();
    if (header == NULL) {
        goto error_out;
    }

    baidu_json_AddStringToObject(header, "namespace", name_space);
    baidu_json_AddStringToObject(header, "name", name);

    if (need_msg_id) {
        duer_generate_msg_id(msg_id);
        baidu_json_AddStringToObject(header, "messageId", msg_id);
        DUER_LOGD("msg_id: %s", msg_id);
    }

    baidu_json_AddItemToObject(event, "header", header);

    payload = baidu_json_CreateObject();
    if (payload == NULL) {
       goto error_out;
    }

    baidu_json_AddItemToObject(event, "payload", payload);

    return event;

error_out:
    DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
    if (event) {
        baidu_json_Delete(event);
    }
    return NULL;
}

baidu_json* duer_get_client_context_internal(void)
{
    baidu_json *client_context = NULL;
    baidu_json *state = NULL;
    int i = 0;

    client_context = baidu_json_CreateArray();
    if (!client_context) {
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        DUER_LOGE("Memory overlow");
        return NULL;
    }

    duer_mutex_lock(s_dcs_client_contex_cb_lock);

    while ((i < s_client_context_cb_count) && (s_dcs_client_context_cb[i])) {
        state = s_dcs_client_context_cb[i]();
        if (state) {
            baidu_json_AddItemToArray(client_context, state);
        }
        i++;
    }

    duer_mutex_unlock(s_dcs_client_contex_cb_lock);

    return client_context;
}

static directive_namespace_list_t *duer_get_namespace_node(const char *name_space)
{
    int i = 0;
    size_t size = 0;
    char *new_namespace = NULL;
    directive_namespace_list_t *list = NULL;

    if (!s_directive_namespace_list) {
        s_directive_namespace_list = DUER_MALLOC(sizeof(*s_directive_namespace_list));
        if (!s_directive_namespace_list) {
            return NULL;
        }

        memset((void *)s_directive_namespace_list, 0, sizeof(*s_directive_namespace_list));
        s_directive_namespace_list->name_space = duer_strdup_internal(name_space);
        if (!s_directive_namespace_list->name_space) {
            DUER_FREE((void *)s_directive_namespace_list);
            s_directive_namespace_list = NULL;
            return NULL;
        }

        s_namespace_count++;
        return (directive_namespace_list_t *)s_directive_namespace_list;
    }

    for (i = 0; i < s_namespace_count; i++) {
        if (strcmp(name_space, s_directive_namespace_list[i].name_space) == 0) {
            return (directive_namespace_list_t *)&s_directive_namespace_list[i];
        }
    }

    new_namespace = duer_strdup_internal(name_space);
    if (!new_namespace) {
        return NULL;
    }

    size = sizeof(*s_directive_namespace_list) * (s_namespace_count + 1);
    list = DUER_REALLOC((void *)s_directive_namespace_list, size);
    if (!list) {
        DUER_FREE(new_namespace);
        return NULL;
    }

    s_directive_namespace_list = list;
    memset((void *)&s_directive_namespace_list[s_namespace_count],
            0,
            sizeof(*s_directive_namespace_list));
    s_directive_namespace_list[s_namespace_count].name_space = new_namespace;

    return (directive_namespace_list_t *)&s_directive_namespace_list[s_namespace_count++];
}

static duer_status_t duer_directive_duplicate(directive_namespace_list_t *node,
                                              const duer_directive_list *directive)
{
    int i = 0;
    char *name = NULL;
    duer_directive_list *list = NULL;

    for (i = 0; i < node->count; i++) {
        if (strcmp(node->list[i].directive_name, directive->directive_name) == 0) {
            node->list[i].cb = directive->cb;
            return DUER_OK;
        }
    }

    name = duer_strdup_internal(directive->directive_name);
    if (!name) {
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        return DUER_ERR_MEMORY_OVERLOW;
    }

    if (node->list) {
        list = (duer_directive_list *)DUER_REALLOC(node->list,
                                                   sizeof(*directive) * (node->count + 1));
    } else {
        list = (duer_directive_list *)DUER_MALLOC(sizeof(*directive));
    }

    if (!list) {
        DUER_FREE(name);
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        return DUER_ERR_MEMORY_OVERLOW;
    }

    list[node->count].cb = directive->cb;
    list[node->count].directive_name = name;
    node->list = list;
    node->count++;

    return DUER_OK;
}

duer_status_t duer_add_dcs_directive(const duer_directive_list *directive,
                                     size_t count,
                                     const char *name_space)
{
    int i = 0;
    duer_status_t ret = DUER_OK;
    directive_namespace_list_t *node;
    DUER_LOGI("namespace: %s", name_space);

    if (!directive || (count == 0) || !name_space) {
        DUER_LOGE("Failed to add dcs directive: param error");
        DUER_DS_LOG_REPORT_DCS_PARAM_ERROR();
        ret = DUER_ERR_INVALID_PARAMETER;
        goto exit;
    }

    duer_mutex_lock(s_dcs_router_lock);

    node = duer_get_namespace_node(name_space);
    if (!node) {
        duer_mutex_unlock(s_dcs_router_lock);
        ret = DUER_ERR_MEMORY_OVERLOW;
        DUER_LOGE("Failed to add dcs directive: no memory");
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        goto exit;
    }

    for (i = 0; i < count; i++) {
        ret = duer_directive_duplicate(node, &directive[i]);
        if (ret != DUER_OK) {
            DUER_LOGE("Failed to add dcs directive: %s, errcode: %d",
                      directive[i].directive_name,
                      ret);
            break;
        }
    }
    duer_mutex_unlock(s_dcs_router_lock);

exit:
    if ((ret != DUER_OK) && (ret != DUER_ERR_INVALID_PARAMETER)) {
        while (i < count) {
            duer_ds_log_dcs_add_directive_fail(directive[i++].directive_name);
        }
    }

    return ret;
}

static duer_status_t duer_private_protocal_cb(duer_context ctx, duer_msg_t* msg, duer_addr_t* addr)
{
    uint32_t current_dialog_id = 0;
    uint32_t directive_dialog_id = 0;
    char *payload = NULL;
    duer_status_t rs = DUER_OK;
    baidu_json *value = NULL;
    baidu_json *name = NULL;
    baidu_json *header = NULL;
    baidu_json *directive = NULL;
    baidu_json *dialog_id = NULL;
    baidu_json *client_context = NULL;
    baidu_json *response_json = NULL;
    char *response_data = NULL;
    size_t response_data_size = 0;
    duer_msg_code_e msg_code = DUER_MSG_RSP_CHANGED;

    DUER_LOGV("Enter");

    if (!msg) {
        rs = DUER_MSG_RSP_BAD_REQUEST;
        goto RET;
    }

    if (!msg->payload || msg->payload_len <= 0) {
        rs = DUER_MSG_RSP_BAD_REQUEST;
        goto RET;
    }

    payload = (char *)DUER_MALLOC(msg->payload_len + 1);
    if (!payload) {
        DUER_LOGE("Memory not enough\n");
        rs = DUER_ERR_FAILED;
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        goto RET;
    }

    DUER_MEMCPY(payload, msg->payload, msg->payload_len);
    payload[msg->payload_len] = '\0';

    DUER_LOGD("payload: %s", payload);

    value = baidu_json_Parse(payload);
    if (value == NULL) {
        DUER_LOGE("Failed to parse payload");
        rs = DUER_ERR_FAILED;
        goto RET;
    }

    directive = baidu_json_GetObjectItem(value, "directive");
    if (!directive) {
        DUER_LOGE("Failed to parse directive");
        rs = DUER_MSG_RSP_BAD_REQUEST;
        goto RET;
    }

    header = baidu_json_GetObjectItem(directive, "header");
    if (!header) {
        DUER_LOGE("Failed to parse header");
        rs = DUER_MSG_RSP_BAD_REQUEST;
        goto RET;
    }

    dialog_id = baidu_json_GetObjectItem(header, "dialogRequestId");
    if (dialog_id) {
        current_dialog_id = duer_get_request_id_internal();
        if (sscanf(dialog_id->valuestring, "%u", &directive_dialog_id) == 0) {
            rs = DUER_MSG_RSP_BAD_REQUEST;
            goto RET;
        }
        if (current_dialog_id != directive_dialog_id) {
            DUER_LOGD("Drop the directive for old dialog_Id: %d, current dialog_Id: %d",
                      directive_dialog_id, current_dialog_id);
            name = baidu_json_GetObjectItem(header, "name");
            goto RET;
        }
    }

    name = baidu_json_GetObjectItem(header, "name");
    if (!name) {
        DUER_LOGE("Failed to parse directive name");
        rs = DUER_MSG_RSP_BAD_REQUEST;
        goto RET;
    }

    if (strcmp("DialogueFinished", name->valuestring) == 0) {
        duer_play_channel_control_internal(DCS_DIALOG_FINISHED);
    } else if (strcmp("GetStatus", name->valuestring) == 0) {
        client_context = duer_get_client_context_internal();
        if (client_context) {
            response_json = baidu_json_CreateObject();
            if (response_json) {
                baidu_json_AddItemToObject(response_json, "clientContext", client_context);
                response_data = baidu_json_PrintUnformatted(response_json);
                baidu_json_Delete(response_json);
                response_data_size = response_data ? DUER_STRLEN(response_data) : 0;
            } else {
                DUER_LOGE("Failed to create response msg: memory too low");
                baidu_json_Delete(client_context);
                rs = DUER_ERR_MEMORY_OVERLOW;
            }
        }
    } else {
        // do nothing
    }

RET:
    if (rs != DUER_OK) {
        if (rs == DUER_MSG_RSP_BAD_REQUEST) {
            msg_code = DUER_MSG_RSP_BAD_REQUEST;
        } else {
            msg_code = DUER_MSG_RSP_INTERNAL_SERVER_ERROR;
        }
    }

    duer_response(msg, msg_code, response_data, response_data_size);

    if (response_data) {
        DUER_FREE(response_data);
    }

    if (payload) {
        DUER_FREE(payload);
    }

    if (value) {
        baidu_json_Delete(value);
    }

    return rs;
}

static duer_status_t duer_execute_directive(baidu_json *directive)
{
    int i = 0;
    char *payload = NULL;
    duer_status_t rs = DUER_OK;
    baidu_json *name = NULL;
    baidu_json *name_space = NULL;
    baidu_json *header = NULL;
    baidu_json *value = NULL;
    dcs_directive_handler handler = NULL;
    directive_namespace_list_t *node = NULL;

    header = baidu_json_GetObjectItem(directive, "header");
    if (!header) {
        DUER_LOGE("Failed to parse directive header");
        rs = DUER_MSG_RSP_BAD_REQUEST;
        goto RET;
    }

    name_space = baidu_json_GetObjectItem(header, "namespace");
    if (!name_space) {
        DUER_LOGE("Failed to parse directive namespace");
        rs = DUER_MSG_RSP_BAD_REQUEST;
        goto RET;
    }

    name = baidu_json_GetObjectItem(header, "name");
    if (!name) {
        DUER_LOGE("Failed to parse directive name");
        rs = DUER_MSG_RSP_BAD_REQUEST;
        goto RET;
    }

    DUER_LOGI("Directive name: %s", name->valuestring);
    duer_mutex_lock(s_dcs_router_lock);
    for (i = 0; i < s_namespace_count; i++) {
        if (strcmp(s_directive_namespace_list[i].name_space, name_space->valuestring) == 0) {
            node = (directive_namespace_list_t *)&s_directive_namespace_list[i];
        }
    }

    if (node) {
        for (i = 0; i < node->count; i++) {
            if (strcmp(node->list[i].directive_name, name->valuestring) == 0) {
                handler = node->list[i].cb;
            }
        }
    }

    if (handler) {
        rs = handler(directive);
    } else {
        DUER_LOGE("unrecognized directive: %s, namespace: %s\n",
                  name->valuestring,
                  name_space->valuestring);
        rs = DUER_MSG_RSP_NOT_FOUND;
    }

    duer_mutex_unlock(s_dcs_router_lock);

RET:
    if (rs == DUER_OK) {
        return rs;
    }

    value = baidu_json_CreateObject();
    if (!value) {
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        return rs;
    }

    baidu_json_AddItemReferenceToObject(value, "directive", directive);
    payload = baidu_json_PrintUnformatted(value);
    baidu_json_Delete(value);
    if (!payload) {
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        return rs;
    }

    if (rs == DUER_MSG_RSP_NOT_FOUND) {
        duer_report_exception_internal(payload, "UNSUPPORTED_OPERATION", "Unknow directive");
    } else if (rs == DUER_MSG_RSP_BAD_REQUEST) {
        duer_report_exception_internal(payload,
                                       "UNEXPECTED_INFORMATION_RECEIVED",
                                       "Bad directive");
    } else {
        duer_report_exception_internal(payload, "INTERNAL_ERROR", "Internal error");
    }

    if (payload) {
        DUER_FREE(payload);
    }

    return rs;
}

static duer_status_t duer_directive_enqueue(baidu_json *directive)
{
    duer_status_t rs = DUER_OK;
    baidu_json *directive_ref = NULL;

    directive_ref = baidu_json_Duplicate(directive, 1);
    if (directive_ref) {
        rs = duer_qcache_push(s_directive_queue, directive_ref);
        if (rs != DUER_OK) {
            baidu_json_release(directive_ref);
            DUER_LOGE("Failed to push directive to queue");
        }
    } else {
        rs = DUER_ERR_MEMORY_OVERLOW;
    }

    return rs;
}

static void duer_execute_caching_directive(int what, void *data)
{
    baidu_json *directive = NULL;
    baidu_json *name = NULL;
    baidu_json *header = NULL;

    duer_mutex_lock(s_directive_queue_lock);
    do {
        if (duer_qcache_length(s_directive_queue) == 0) {
            break;
        }

        directive = duer_qcache_top(s_directive_queue);
        if (!directive) {
            DUER_LOGE("Failed to get directive");
            break;
        }

        duer_execute_directive(directive);

        header = baidu_json_GetObjectItem(directive, "header");
        if (!header) {
            DUER_LOGE("Failed to get header");
            break;
        }

        name = baidu_json_GetObjectItem(header, "name");
        if (!name || !name->valuestring) {
            DUER_LOGE("Failed to get name");
            break;
        }

        if (strcmp(name->valuestring, "Speak") != 0) {
            duer_qcache_pop(s_directive_queue);
            baidu_json_Delete(directive);
            if (duer_qcache_length(s_directive_queue) > 0) {
                duer_emitter_emit(duer_execute_caching_directive, 0, NULL);
            }
        }
    } while(0);

    duer_mutex_unlock(s_directive_queue_lock);
}

void duer_cancel_caching_directive()
{
    baidu_json *directive = NULL;
    baidu_json *name = NULL;
    baidu_json *header = NULL;

    duer_mutex_lock(s_directive_queue_lock);
    directive = duer_qcache_top(s_directive_queue);
    if (directive) {
        header = baidu_json_GetObjectItem(directive, "header");
        if (header) {
            name = baidu_json_GetObjectItem(header, "name");
            if (name && name->valuestring) {
                if (strcmp(name->valuestring, "Speak") == 0) {
                    duer_dcs_stop_speak_handler();
                }
            }
        }
    }
    do {
        directive = duer_qcache_pop(s_directive_queue);
        baidu_json_Delete(directive);
    } while (duer_qcache_length(s_directive_queue) > 0);
    duer_mutex_unlock(s_directive_queue_lock);
}

void duer_speak_directive_finished()
{
    baidu_json *directive = NULL;
    baidu_json *name = NULL;
    baidu_json *header = NULL;

    duer_mutex_lock(s_directive_queue_lock);
    do {
        if (duer_qcache_length(s_directive_queue) == 0) {
            break;
        }

        directive = duer_qcache_top(s_directive_queue);
        if (!directive) {
            DUER_LOGE("Failed to get directive");
            break;
        }

        header = baidu_json_GetObjectItem(directive, "header");
        if (!header) {
            DUER_LOGE("Failed to get header");
            break;
        }

        name = baidu_json_GetObjectItem(header, "name");
        if (!name || !name->valuestring) {
            DUER_LOGE("Failed to get name");
            break;
        }

        if (strcmp(name->valuestring, "Speak") == 0) {
            duer_qcache_pop(s_directive_queue);
            baidu_json_Delete(directive);
        }

        if (duer_qcache_length(s_directive_queue) > 0) {
            duer_emitter_emit(duer_execute_caching_directive, 0, NULL);
        }
    } while (0);
    duer_mutex_unlock(s_directive_queue_lock);
}

static duer_status_t duer_handle_directive_by_dialog_id(baidu_json *directive)
{
    baidu_json *dialog_id = NULL;
    baidu_json *name = NULL;
    baidu_json *header = NULL;
    uint32_t current_dialog_id = 0;
    uint32_t directive_dialog_id = 0;
    duer_status_t rs = DUER_OK;

    header = baidu_json_GetObjectItem(directive, "header");
    if (!header) {
        DUER_LOGE("Failed to parse header");
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    dialog_id = baidu_json_GetObjectItem(header, "dialogRequestId");
    // execute the directive immediately if no dialog_id item
    if (!dialog_id) {
        rs = duer_execute_directive(directive);
        return rs;
    }

    name = baidu_json_GetObjectItem(header, "name");
    if (!name || !name->valuestring) {
        DUER_LOGE("Failed to get name");
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    current_dialog_id = duer_get_request_id_internal();
    if (sscanf(dialog_id->valuestring, "%u", &directive_dialog_id) == 0) {
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    if (current_dialog_id != directive_dialog_id) {
        DUER_LOGI("Drop the directive for old dialog_Id: %u, current dialog_Id: %u",
                  directive_dialog_id, current_dialog_id);
        duer_ds_log_dcs_directive_drop(current_dialog_id,
                                       directive_dialog_id,
                                       name->valuestring);
        return DUER_OK;
    }

    duer_mutex_lock(s_directive_queue_lock);

    rs = duer_directive_enqueue(directive);
    if ((rs == DUER_OK) && (duer_qcache_length(s_directive_queue) == 1)) {
        rs = duer_emitter_emit(duer_execute_caching_directive, 0, NULL);
    }

    DUER_LOGI("queue len: %d", duer_qcache_length(s_directive_queue));
    duer_mutex_unlock(s_directive_queue_lock);

    return rs;
}

static duer_status_t duer_dcs_router(duer_context ctx, duer_msg_t* msg, duer_addr_t* addr)
{
    char *payload = NULL;
    duer_status_t rs = DUER_OK;
    baidu_json *value = NULL;
    baidu_json *directive = NULL;
    duer_msg_code_e msg_code = DUER_MSG_RSP_CHANGED;

    DUER_LOGV("Enter");

    do {
        if (!msg) {
            rs = DUER_MSG_RSP_BAD_REQUEST;
            break;
        }

        if (!msg->payload || msg->payload_len <= 0) {
            rs = DUER_MSG_RSP_BAD_REQUEST;
            break;
        }

        payload = (char *)DUER_MALLOC(msg->payload_len + 1);
        if (!payload) {
            DUER_LOGE("Memory not enough\n");
            rs = DUER_ERR_FAILED;
            DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
            break;
        }

        DUER_MEMCPY(payload, msg->payload, msg->payload_len);
        payload[msg->payload_len] = '\0';
        DUER_LOGD("payload: %s", payload);

        value = baidu_json_Parse(payload);
        if (value == NULL) {
            DUER_LOGE("Failed to parse payload");
            rs = DUER_ERR_FAILED;
            break;
        }

        directive = baidu_json_GetObjectItem(value, "directive");
        if (!directive) {
            DUER_LOGE("Failed to parse directive");
            rs = DUER_MSG_RSP_BAD_REQUEST;
            break;
        }

        rs = duer_handle_directive_by_dialog_id(directive);
    } while (0);

    if (rs != DUER_OK) {
        if (rs == DUER_MSG_RSP_NOT_FOUND) {
            duer_report_exception_internal(payload, "UNSUPPORTED_OPERATION", "Unknow directive");
            msg_code = DUER_MSG_RSP_NOT_FOUND;
        } else if (rs == DUER_MSG_RSP_BAD_REQUEST) {
            duer_report_exception_internal(payload,
                                           "UNEXPECTED_INFORMATION_RECEIVED",
                                           "Bad directive");
            msg_code = DUER_MSG_RSP_BAD_REQUEST;
        } else {
            duer_report_exception_internal(payload, "INTERNAL_ERROR", "Internal error");
            msg_code = DUER_MSG_RSP_INTERNAL_SERVER_ERROR;
        }
    }

    duer_response(msg, msg_code, NULL, 0);

    if (payload) {
        DUER_FREE(payload);
    }

    if (value) {
        baidu_json_Delete(value);
    }

    return rs;
}

void duer_dcs_dialog_cancel(void)
{
    duer_create_request_id_internal();
    duer_cancel_caching_directive();
}

void duer_dcs_framework_init()
{
    static bool is_first_time = true;

    if (is_first_time) {
        is_first_time = false;
        duer_dcs_close_multi_dialog();
    }

    duer_res_t res[] = {
        {DUER_RES_MODE_DYNAMIC, DUER_RES_OP_PUT, "duer_directive", duer_dcs_router},
        {DUER_RES_MODE_DYNAMIC, DUER_RES_OP_PUT, "duer_private", duer_private_protocal_cb},
    };

    duer_add_resources(res, sizeof(res) / sizeof(res[0]));

    if (!s_dcs_router_lock) {
        s_dcs_router_lock = duer_mutex_create();
        if (!s_dcs_router_lock) {
            DUER_LOGE("Failed to create s_dcs_router_lock");
        }
    }

    if (!s_dcs_client_contex_cb_lock) {
        s_dcs_client_contex_cb_lock = duer_mutex_create();
        if (!s_dcs_client_contex_cb_lock) {
            DUER_LOGE("Failed to create s_dcs_client_contex_cb_lock");
        }
    }

    if (!s_directive_queue) {
        s_directive_queue = duer_qcache_create();
        if (!s_directive_queue) {
            DUER_LOGE("Failed to create DCS directive queue");
        }
    }

    if (!s_directive_queue_lock) {
        s_directive_queue_lock = duer_mutex_create();
        if (!s_directive_queue_lock) {
            DUER_LOGE("Failed to create s_directive_queue_lock");
        }
    }

    duer_dcs_local_init_internal();
    duer_declare_sys_interface_internal();
}

