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
 * File: lightduer_engine.c
 * Auth: Su Hao (suhao@baidu.com)
 * Desc: Light duer IoT CA engine.
 */

#include "lightduer_connagent.h"
#include "lightduer_engine.h"
#include "lightduer_log.h"
#include "lightduer_timers.h"
#include "lightduer_queue_cache.h"
#include "lightduer_ca.h"
#include "lightduer_memory.h"
#include "lightduer_mutex.h"
#include "lightduer_ds_report.h"
#include "lightduer_event_emitter.h"
#include "lightduer_timestamp.h"
#include "lightduer_ds_log_ca.h"
#include "lightduer_ds_report_ca.h"
#include "lightduer_data_cache.h"

#define DUER_MOST_BIT_MASK  (0x80000000)

extern int duer_data_send(void);
static void duer_engine_notify(int event, int status, int what, void *object);
static void duer_engine_timer_handler(int what, void *data);
static void duer_transport_notify_handler(int what, void *data);
void duer_engine_send_directly(int what, void *object);

#define DUER_NOTIFY_ONLY(_evt, _status) duer_engine_notify(_evt, _status, 0, NULL)

static duer_handler g_handler = NULL;
static duer_engine_notify_func g_notify_func = NULL;
static duer_timer_handler g_timer = NULL;

static duer_qcache_handler g_qcache_handler = NULL;
static duer_mutex_t g_qcache_mutex = NULL;

static duer_u32_t g_recv_timestamp = 0;

#define DUER_KEEPALIVE_INTERVAL (55 * 1000)
#define DUER_START_TIMEOUT (1 * 8000) // 8s(notify DUER_ERR_TRANS_TIMEOUT after about 16s)
#define DUER_START_INITIAL_RETRY_INTERVAL (1 * 500) // retry on 500ms, 1s, 2s, 4s, 8s
#define DUER_RECV_TIMEOUT_THRESHOLD (DUER_KEEPALIVE_INTERVAL + 3000)
#define DUER_COAP_EXEC_INTERVAL (5 * 1000)

static void duer_timer_expired(void *param)
{
    duer_emitter_emit(duer_engine_timer_handler, 0, NULL);
}

static void duer_transport_notify(duer_transevt_e event)
{
    duer_emitter_emit(duer_transport_notify_handler, event, NULL);
}

static void duer_engine_notify(int event, int status, int what, void *object)
{
    if (g_notify_func) {
        g_notify_func(event, status, what, object);
    } else {
        DUER_LOGI("g_notify_func is not set");
    }
}

static void duer_transport_notify_handler(int what, void *data)
{
    duer_transevt_e event = (duer_transevt_e) what;
    switch (event) {
    case DUER_TEVT_RECV_RDY:
        duer_data_available();
        break;
    case DUER_TEVT_SEND_RDY:
    {
        duer_bool is_started = baidu_ca_is_started(g_handler);
        if (is_started) {
            duer_emitter_emit(duer_engine_send_directly, 0, NULL);
//            duer_data_send();
        } else if (baidu_ca_is_stopped(g_handler) == DUER_FALSE) {
            DUER_NOTIFY_ONLY(DUER_EVT_START, DUER_ERR_CONNECT_TIMEOUT);// trigger reconnection
        }
        break;
    }
    case DUER_TEVT_SEND_TIMEOUT:
        DUER_LOGI("duer_transport_notify send timeout!");
        DUER_NOTIFY_ONLY(DUER_EVT_SEND_DATA, DUER_ERR_TRANS_TIMEOUT);
        break;
    default:
        DUER_LOGI("duer_transport_notify unknow event:%d", event);
        break;
    }
}

static void duer_engine_timer_handler(int what, void *data)
{
    duer_bool is_started = baidu_ca_is_started(g_handler);
    if (is_started) {
        baidu_json *alive = baidu_json_CreateObject();
        if (alive) {
            baidu_json_AddNumberToObject(alive, "alive", 0);
            duer_data_report(alive);
            baidu_json_Delete(alive);
        } else {
            DUER_LOGW("create alive fail(maybe memory not available now)!");
        }
    } else if (baidu_ca_is_stopped(g_handler) == DUER_FALSE) {
        DUER_NOTIFY_ONLY(DUER_EVT_START, DUER_ERR_CONNECT_TIMEOUT);// trigger reconnection
    }
}

static duer_status_t duer_engine_transmit_handler(duer_trans_handler hdlr, const void *data, duer_size_t size, duer_addr_t *addr) {
    duer_status_t rs = duer_dcache_push(data, size);

    if (rs == 1) {
        duer_emitter_emit(duer_engine_send_directly, 0, NULL);
    }

    return rs;
}

void duer_engine_register_notify(duer_engine_notify_func func)
{
    if (g_notify_func != func) {
        g_notify_func = func;
    } else {
        DUER_LOGI("g_notify_func is update to func:%p", func);
    }
}

void duer_engine_create(int what, void *object)
{
    if (g_handler == NULL) {
        g_handler = baidu_ca_acquire(duer_transport_notify);
    }
    if (g_qcache_handler == NULL) {
        g_qcache_handler = duer_qcache_create();
    }
    if (g_qcache_mutex == NULL) {
        if ((g_qcache_mutex = duer_mutex_create()) == NULL) {
            DUER_LOGW("Create mutex failed!!");
        }
    }
    if (g_timer == NULL) {
        g_timer = duer_timer_acquire(duer_timer_expired, NULL, DUER_TIMER_ONCE);
    }

    duer_dcache_initialize();

    DUER_NOTIFY_ONLY(DUER_EVT_CREATE, DUER_OK);
}

void duer_engine_set_response_callback(int what, void *object)
{
    if (g_handler && object) {
        DUER_LOGD("set the coap response callback %p", object);
        baidu_ca_report_set_response_callback(g_handler, (duer_notify_f)object, NULL);
    }
}

void duer_engine_start(int what, void *object)
{
    static int start_timeout;
    duer_status_t rs = DUER_OK;
    int status = DUER_OK;
    duer_bool is_started = baidu_ca_is_started(g_handler);
    do {
        if (is_started) {
            DUER_LOGI("already started, what:%d, object:%p", what, object);
            status = DUER_ERR_HAS_STARTED;
            break;
        }

        DUER_LOGI("duer_engine_start, g_handler:%p, length:%d, profile:%p",
                g_handler, what, object);
        // add this check. so duer_engine_start become re-enterable without the profile param.
        if (what > 0 && object != NULL) {

            start_timeout = DUER_START_INITIAL_RETRY_INTERVAL;

            // object contains the profile content, and what is the length of the profile.
            //DUER_LOGI("the length of profile: %p is %d", (const char *)object, (size_t)what);
            rs = baidu_ca_load_configuration(g_handler, (const void *)object, (size_t)what);
            if (rs < DUER_OK) {
                status = DUER_ERR_PROFILE_INVALID;
                duer_ds_log_ca_cache_error(DUER_DS_LOG_CA_START_PROFILE_ERR);
                break;
            }

            rs = baidu_ca_report_set_data_tx_callback(g_handler, duer_engine_transmit_handler);
        }

        rs = baidu_ca_start(g_handler);
        if (rs == DUER_ERR_TRANS_WOULD_BLOCK) {
            status = DUER_ERR_TRANS_WOULD_BLOCK;
        } else if (rs != DUER_OK) {
            DUER_LOGI("start fail, status:%d, rs:%d", status, rs);
            status = DUER_ERR_FAILED;
        } else {
            // record the registered timestamp
            g_recv_timestamp = duer_timestamp();
            DUER_LOGD("Started recv_timestamp = %u", g_recv_timestamp);
        }
    } while (0);


    if (g_timer != NULL) {
        if (status == DUER_OK) {
            duer_timer_start(g_timer, DUER_KEEPALIVE_INTERVAL);
        } else if (status == DUER_ERR_TRANS_WOULD_BLOCK) {
            if (start_timeout > DUER_START_TIMEOUT) {
                status = DUER_ERR_TRANS_TIMEOUT;// start timeout
                start_timeout = DUER_START_INITIAL_RETRY_INTERVAL;
            } else {
                duer_timer_start(g_timer, start_timeout);
                start_timeout <<= 1;
            }
        } else {
            // do nothing
        }
    } else {
        DUER_LOGW("g_timer is NULL");
    }

    duer_engine_notify(DUER_EVT_START, status, 0, object);
}

void duer_engine_add_resources(int what, void *object)
{
    duer_status_t rs = baidu_ca_add_resources(g_handler, (const duer_res_t *)object, (size_t)what);

    duer_engine_notify(DUER_EVT_ADD_RESOURCES, rs < DUER_OK ? DUER_ERR_FAILED : DUER_OK, what, object);
}

void duer_engine_data_available(int what, void *object)
{
    duer_status_t rs = DUER_OK;
    int status = DUER_OK;
    duer_bool is_started;
    DUER_LOGD("duer_engine_data_available");
    do {
        is_started = baidu_ca_is_stopped(g_handler);
        if (is_started == DUER_TRUE) {
            DUER_LOGW("duer_engine_data_available has stopped");
            status = DUER_ERR_HAS_STOPPED;
            break;
        }

        duer_ds_report_ca_inc_recv_count();
        is_started = baidu_ca_is_started(g_handler);

        rs = baidu_ca_data_available(g_handler, NULL);
        if (rs == DUER_ERR_TRANS_WOULD_BLOCK) {
            status = DUER_ERR_TRANS_WOULD_BLOCK;
            DUER_LOGD("receive WOULDBLOCK!!");
        } else if (rs < DUER_OK) {
            status = DUER_ERR_FAILED;
        } else {
            g_recv_timestamp = duer_timestamp();
            DUER_LOGD("Received recv_timestamp = %u", g_recv_timestamp);
            duer_ds_report_ca_inc_valid_recv();
        }
    } while (0);

    if (is_started == DUER_TRUE) {
        DUER_NOTIFY_ONLY(DUER_EVT_DATA_AVAILABLE, status);
    } else {
        DUER_LOGD("status == %d && g_timer != %p", status, g_timer);
        if (status == DUER_OK && g_timer != NULL) {
            // maybe started to handle the connect timeout
            //duer_timer_stop(g_timer); // stop the timer before start
            duer_timer_start(g_timer, DUER_KEEPALIVE_INTERVAL);
        }
        DUER_NOTIFY_ONLY(DUER_EVT_START, status);
    }
}

void duer_engine_release_data(duer_msg_t *msg)
{
    if (msg) {
        if (msg->payload != NULL) {
            if (DUER_MESSAGE_IS_RESPONSE(msg->msg_code)) {
                DUER_FREE(msg->payload);
            } else {
                baidu_json_release(msg->payload);
            }
            msg->payload = NULL;
        }
        if (msg->context != NULL) {
            if (msg->context->_on_report_finish) {
                msg->context->_on_report_finish(msg->context);
                msg->context->_on_report_finish = NULL;
            }

            DUER_FREE(msg->context);
            msg->context = NULL;
        }
        if (g_handler == NULL) {
            DUER_LOGE("release data failed!");
        } else {
            baidu_ca_release_message(g_handler, msg);
        }
    }
}

void duer_engine_clear_data(int what, void *object)
{
    duer_msg_t *msg = NULL;

    duer_mutex_lock(g_qcache_mutex);
    while ((msg = duer_qcache_pop(g_qcache_handler)) != NULL) {
        if (what == 0
                || msg->create_timestamp < (duer_u32_t)what
                || ((msg->create_timestamp & DUER_MOST_BIT_MASK) != 0
                     && ((duer_u32_t)what) & DUER_MOST_BIT_MASK) == 0) {
            duer_engine_release_data(msg);
        } else {
            break;
        }
    }
    duer_mutex_unlock(g_qcache_mutex);
}

/** return the cache length if success, or error code
 */
int duer_engine_enqueue_report_data(duer_context_t *context, const baidu_json *data)
{
    int rs = DUER_ERR_INVALID_PARAMETER;
    char *content = NULL;
    baidu_json *payload = NULL;
    duer_msg_t *msg = NULL;

    do {
        if (baidu_ca_is_started(g_handler) == DUER_FALSE) {
            DUER_LOGW("duer_engine_send not started");
            rs = DUER_ERR_CA_NOT_CONNECTED;
            break;
        }

        if (data == NULL || g_qcache_handler == NULL || g_handler == NULL) {
            break;
        }

        payload = baidu_json_CreateObject();
        if (payload == NULL) {
            rs = DUER_ERR_MEMORY_OVERLOW;
            break;
        }

        baidu_json_AddItemReferenceToObject(payload, "data", (baidu_json *)data);
        content = baidu_json_PrintUnformatted(payload);
        DUER_LOGD("enqueue content:%s", content);
        baidu_json_Delete(payload);

        msg = baidu_ca_build_report_message(g_handler, DUER_TRUE);
        if (msg == NULL) {
            rs = DUER_ERR_MEMORY_OVERLOW;
            break;
        }

        msg->payload = (duer_u8_t*)content;
        msg->payload_len = DUER_STRLEN(content);

        if (context != NULL) {
            msg->context = DUER_MALLOC(sizeof(duer_context_t));
            if (msg->context) {
                DUER_MEMCPY(msg->context, context, sizeof(duer_context_t));
            }
        }

        duer_mutex_lock(g_qcache_mutex);
        rs = duer_qcache_push(g_qcache_handler, msg);
        if (rs == DUER_OK) {
            rs = duer_qcache_length(g_qcache_handler);
        }
        duer_mutex_unlock(g_qcache_mutex);
    } while (0);

    if (rs < 0) {
        DUER_LOGE("Report failed: rs = %d", rs);
        if (msg) {
            duer_engine_release_data(msg);
        } else if (content) {
            baidu_json_release(content);
        }
    }

    return rs;
}

/** return the cache length if success, or error code
 */
int duer_engine_enqueue_response(const duer_msg_t *req, int msg_code, const void *data, duer_size_t size)
{
    int rs = DUER_ERR_INVALID_PARAMETER;
    char *content = NULL;
    duer_msg_t *msg = NULL;

    do {
        if (baidu_ca_is_stopped(g_handler)) {
            DUER_LOGW("duer_engine_send has stopped");
            rs = DUER_ERR_CA_NOT_CONNECTED;
            break;
        }

        if (req == NULL || g_handler == NULL) {
            break;
        }

        msg = baidu_ca_build_response_message(g_handler, req, msg_code);
        if (msg == NULL) {
            rs = DUER_ERR_MEMORY_OVERLOW;
            break;
        }

        if (size > 0) {
            content = (char *)DUER_MALLOC(size);
            if (content == NULL) {
                rs = DUER_ERR_MEMORY_OVERLOW;
                DUER_DS_LOG_CA_MEMORY_OVERFLOW();
                break;
            }
            DUER_MEMCPY(content, data, size);
        }

        msg->payload = (duer_u8_t *)content;
        msg->payload_len = size;

        duer_mutex_lock(g_qcache_mutex);
        rs = duer_qcache_push(g_qcache_handler, msg);
        if (rs == DUER_OK) {
            rs =  duer_qcache_length(g_qcache_handler);
        }
        duer_mutex_unlock(g_qcache_mutex);
    } while (0);

    if (rs < 0) {
        DUER_LOGE("Response failed: rs = %d", rs);
        if (msg) {
            duer_engine_release_data(msg);
        } else if (content) {
            DUER_FREE(content);
        }
    }

    return rs;
}

/** return the cache length if success, or error code
 */
int duer_engine_enqueue_seperate_response(const char *token,
                                          duer_size_t token_len,
                                          int msg_code,
                                          const baidu_json *data)
{
    int rs = DUER_ERR_INVALID_PARAMETER;
    char *content = NULL;
    duer_msg_t *msg = NULL;
    char *data_str = NULL;
    duer_u32_t str_size = 0;

    do {
        if (baidu_ca_is_stopped(g_handler)) {
            DUER_LOGW("duer_engine_send has stopped");
            rs = DUER_ERR_CA_NOT_CONNECTED;
            break;
        }

        if (g_handler == NULL) {
            break;
        }

        msg = baidu_ca_build_seperate_response_message(g_handler,
                                                       token, token_len,
                                                       msg_code, DUER_TRUE);
        if (msg == NULL) {
            rs = DUER_ERR_MEMORY_OVERLOW;
            break;
        }

        if (data) {
            data_str = baidu_json_PrintUnformatted(data);
            DUER_LOGD("enqueue content:%s", data_str);
            baidu_json_Delete((baidu_json*)data);

            str_size = DUER_STRLEN(data_str);
            content = (char *)DUER_MALLOC(str_size);// response message see duer_engine_release_data
            if (content == NULL) {
                rs = DUER_ERR_MEMORY_OVERLOW;
                DUER_DS_LOG_CA_MEMORY_OVERFLOW();
                break;
            }
            DUER_MEMCPY(content, data_str, str_size);
        }

        msg->payload = (duer_u8_t *)content;
        msg->payload_len = str_size;

        duer_mutex_lock(g_qcache_mutex);
        rs = duer_qcache_push(g_qcache_handler, msg);
        if (rs == DUER_OK) {
            rs =  duer_qcache_length(g_qcache_handler);
        }
        duer_mutex_unlock(g_qcache_mutex);
    } while (0);

    if (rs < 0) {
        DUER_LOGE("Response failed: rs = %d", rs);
        if (msg) {
            duer_engine_release_data(msg);
            msg = NULL;
        }
        if (content) {
            DUER_FREE(content);
            content = NULL;
        }
    }

    if (data_str) {
        baidu_json_release(data_str);
    }

    return rs;
}

void duer_engine_send(int what, void *object)
{
    duer_status_t rs = DUER_OK;
    int status = DUER_OK;
    duer_msg_t *msg = NULL;
    duer_u32_t timestamp;
    static duer_u32_t last_coap_exec_time = 0;

    do {
        if (baidu_ca_is_stopped(g_handler)) {
            DUER_LOGW("duer_engine_send has stopped");
            status = DUER_ERR_HAS_STOPPED;
            break;
        }

        if (duer_dcache_length() > 0) {
            if (duer_qcache_length(g_qcache_handler) > 0) {
                DUER_LOGW("data cache has not sent, pending..., dcache_len:%d, qcache_len:%d",
                        duer_dcache_length(), duer_qcache_length(g_qcache_handler));
                status = DUER_ERR_TRANS_WOULD_BLOCK;
            }
            break;
        }

        timestamp = duer_timestamp();
        if (timestamp - last_coap_exec_time > DUER_COAP_EXEC_INTERVAL) {
            baidu_ca_exec(g_handler);
            last_coap_exec_time = timestamp;
        }

        duer_mutex_lock(g_qcache_mutex);
        msg = duer_qcache_top(g_qcache_handler);
        duer_mutex_unlock(g_qcache_mutex);
        if (msg == NULL) {
            break;
        }

        if (msg->context && msg->context->_on_report_start) {
            msg->context->_on_report_start(msg->context);
            msg->context->_on_report_start = NULL;
        }

        rs = baidu_ca_send_data(g_handler, msg, NULL); // always success now

        if (msg->context) {
            msg->context->_status = rs;
        }

        if (rs == DUER_ERR_TRANS_WOULD_BLOCK) {
            status = DUER_ERR_TRANS_WOULD_BLOCK;
        } else if (rs < DUER_OK) {
            status = DUER_ERR_FAILED;
        } else {
            int latency = 0;
            DUER_LOGV("old:timestamp:%u,cts:%u", timestamp, msg->create_timestamp);
            timestamp = duer_timestamp();
            if (timestamp > msg->create_timestamp) {
                latency = timestamp - msg->create_timestamp;
                duer_ds_report_ca_update_avg_latency(latency);
            }
            DUER_LOGV("new_timestamp:%u, latency:%u", timestamp, latency);
#ifdef DUER_DEBUG_AFTER_SEND
            DUER_LOGI("data sent: %.*s", msg->payload_len,
                    DUER_STRING_OUTPUT((const char *)msg->payload));
#endif
            duer_engine_release_data(msg);
            msg = NULL;
            duer_mutex_lock(g_qcache_mutex);
            duer_qcache_pop(g_qcache_handler);
            duer_mutex_unlock(g_qcache_mutex);
        }

        if (rs != DUER_ERR_TRANS_WOULD_BLOCK && msg && msg->context
                && msg->context->_on_report_finish) {
            msg->context->_on_report_finish(msg->context); //Note: message not send out really.
            msg->context->_on_report_finish = NULL;
        }

    } while (0);

    //duer_engine_notify(DUER_EVT_SEND_DATA, status, what, object);
}

void duer_engine_send_directly(int what, void *object)
{
    duer_status_t rs = DUER_OK;
    int status = DUER_OK;
    duer_dcache_item *cache = NULL;
    duer_u32_t timestamp;

    do {
        if (baidu_ca_is_stopped(g_handler)) {
            DUER_LOGW("duer_engine_send has stopped");
            status = DUER_ERR_HAS_STOPPED;
            break;
        }

        timestamp = duer_timestamp();
        if (g_recv_timestamp) {
            if (timestamp > g_recv_timestamp) {
                if (timestamp - g_recv_timestamp > DUER_RECV_TIMEOUT_THRESHOLD) {
                    DUER_LOGE("duer has stopped! timestamp = %u, recv_timestamp = %u",
                                                         timestamp, g_recv_timestamp);
                    status = DUER_ERR_TRANS_TIMEOUT;
                    break;
                }
            } else {
                g_recv_timestamp = 0;
            }
        }

        cache = duer_dcache_top();
        if (cache == NULL) {
            break;
        }

        if (g_timer != NULL) {
            duer_timer_stop(g_timer);
        }

        rs = baidu_ca_send_data_directly(g_handler, cache->data, cache->size, NULL);

        if (rs == DUER_ERR_TRANS_WOULD_BLOCK) {
            status = DUER_ERR_TRANS_WOULD_BLOCK;
        } else if (rs < DUER_OK) {
            status = DUER_ERR_FAILED;
        } else {
            if (g_timer != NULL) {
                duer_timer_start(g_timer, DUER_KEEPALIVE_INTERVAL);
            }
            duer_dcache_pop();

            if (duer_dcache_length() > 0) { // can not enter here
                duer_emitter_emit(duer_engine_send_directly, 0, NULL);
            }
        }
    } while (0);

    duer_engine_notify(DUER_EVT_SEND_DATA, status, what, object);
}

void duer_engine_stop(int what, void *object)
{
    duer_status_t rs = DUER_OK;

    do {
        if (baidu_ca_is_stopped(g_handler)) {
            DUER_LOGW("duer_engine_stop has stopped");
            rs = DUER_ERR_HAS_STOPPED;
            break;
        }

        g_recv_timestamp = 0;

        rs = baidu_ca_stop(g_handler);
        if (rs == DUER_ERR_TRANS_WOULD_BLOCK) {
            DUER_LOGW("baidu_ca_stop DUER_ERR_TRANS_WOULD_BLOCK");
        } else if (rs < DUER_OK) {
            DUER_LOGW("baidu_ca_stop rs:%d", rs);
        } else {
            DUER_LOGI("no action");
            // do nothing
        }
    } while (0);
    duer_dcache_clear();
    DUER_NOTIFY_ONLY(DUER_EVT_STOP, what);
}

void duer_engine_destroy(int what, void *object)
{
    if (g_timer != NULL) {
        duer_timer_release(g_timer);
        g_timer = NULL;
    }
    if (g_qcache_handler != NULL) {
        duer_engine_clear_data(0, NULL);
        duer_mutex_lock(g_qcache_mutex);
        duer_qcache_destroy(g_qcache_handler);
        duer_mutex_unlock(g_qcache_mutex);
        g_qcache_handler = NULL;
    }
    if (g_qcache_mutex != NULL) {
        duer_mutex_destroy(g_qcache_mutex);
        g_qcache_mutex = NULL;
    }
    if (g_handler != NULL) {
        baidu_ca_release(g_handler);
        g_handler = NULL;
    }
    duer_dcache_finalize();

    DUER_NOTIFY_ONLY(DUER_EVT_DESTROY, DUER_OK);
}

int duer_engine_qcache_length()
{
    int len = 0;

    if (g_qcache_handler == NULL) {
        return 0;
    }

    duer_mutex_lock(g_qcache_mutex);
    len =  duer_qcache_length(g_qcache_handler);
    duer_mutex_unlock(g_qcache_mutex);

    return len;
}

duer_bool duer_engine_is_started()
{
    return baidu_ca_is_started(g_handler);
}
