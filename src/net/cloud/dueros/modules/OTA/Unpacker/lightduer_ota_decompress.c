/*
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
 *
 * File: lightduer_ota_decompress.c
 * Auth: Zhong Shuai (zhongshuai@baidu.com)
 * Desc: OTA decompress
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "baidu_json.h"
#include "lightduer_log.h"
#include "lightduer_memory.h"
#include "lightduer_ota_decompress.h"
#include "lightduer_ota_verification.h"
#include "lightduer_ota_installer.h"

#define CHUNK 128

static void *my_alloc(void *opaque, unsigned items, unsigned size)
{
    if (opaque) {
        items += size - size;
    }

    return sizeof(uInt) > 2 ? (voidpf)DUER_MALLOC(items * size) :
                              (voidpf)DUER_CALLOC(items, size);
}

static void my_free(void *opaque, void *ptr)
{
    DUER_FREE(ptr);

    if (opaque) {
        return;
    }
}

void *duer_ota_unpack_zlibstream_decompress_init(void)
{
    int ret = 0;
    duer_ota_decompress_context_t *ctx = NULL;

    ctx = (duer_ota_decompress_context_t *)DUER_MALLOC(sizeof(duer_ota_decompress_context_t));
    if (!ctx) {
        DUER_LOGE("Decompress: Malloc failed");

        return NULL;
    }

    // global footprint init
    ctx->stream_recieved_sz = 0;
    ctx->stream_processed_sz = 0;

    ctx->meta_data = 0;
    ctx->meta_size = ctx->meta_stored_size = 0;

    ctx->write_offset = 0;

    ctx->strmp = (z_streamp)DUER_MALLOC(sizeof(z_stream));
    if (!ctx->strmp) {
        DUER_LOGE("Decompress: Malloc failed");

        DUER_FREE(ctx);

        return NULL;
    }

    // allocate inflate state
    ctx->strmp->zalloc = my_alloc;
    ctx->strmp->zfree = my_free;
    ctx->strmp->opaque = Z_NULL;
    ctx->strmp->avail_in = 0;
    ctx->strmp->next_in = Z_NULL;

    ret = inflateInit(ctx->strmp);
    if (ret != Z_OK) {
        DUER_LOGE("Decompress: Init flate failed");

        DUER_FREE(ctx);
        DUER_FREE(ctx->strmp);

        return NULL;
    }

    return ctx;
}

static int duer_ota_unpack_get_json_value(baidu_json *obj, const char *key, baidu_json **child_obj)
{
    int find = 1;
    baidu_json *child = NULL;

    if (obj == NULL || key == NULL) {
        DUER_LOGE("Decompress: Argument Error");

        return find;
    }

    if (obj && (child = obj->child)) {
        while (child != 0)
        {
            if (strcmp(child->string, key) == 0)
            {
                if (baidu_json_Object == child->type || baidu_json_Array == child->type)
                {
                    find = 0;
                    *child_obj = baidu_json_Duplicate(child, 1);
                }
                break;
            }
            child = child->next;
        }
    }
    return find;
}

static int duer_ota_unpack_get_plain_value(
        baidu_json *obj,
        const char *key,
        char **value,
        unsigned int *length)
{
    int find = 1;
    int type = 0;
    char buffer[20] = {0};
    baidu_json *child = NULL;

    if (obj && (child = obj->child)) {
        while (child != NULL) {
            DUER_LOGD("child->string:%s", child->string);
            if (strcmp(child->string, key) == 0) {
                type = child->type;
                find = 0;
                if (baidu_json_Number == type) {
                    if (child->valueint == child->valuedouble) {
                        snprintf(buffer, 20, "%d", child->valueint);
                    } else {
                        snprintf(buffer, 20, "%f", child->valuedouble);
                    }

                    *length = strlen(buffer);
                    *value = (char *)DUER_MALLOC(*length + 1);
                    if (*value == NULL) {
                        DUER_LOGE("Decompress: Malloc failed");

                        goto out;
                    }
                    strncpy(*value, buffer, *length);
                }
                else if (baidu_json_String == type) {
                    *length = strlen(child->valuestring);
                    *value = (char *)DUER_MALLOC(*length + 1);
                    if (*value == NULL) {
                        DUER_LOGE("Decompress: Malloc failed");

                        goto out;
                    }
                    memset(*value, 0, *length);
                    strncpy(*value, child->valuestring, *length);
                } else {
                    find = 1;
                }
                break;
            }
            child = child->next;
        }
    }
out:
    return find;
}

static int duer_ota_unpack_parse_meta_data(char *meta_data, duer_ota_package_meta_data_t *meta)
{
    char *value = NULL;
    unsigned int length = 0;
    unsigned int module_count = 0;
    baidu_json *meta_obj = NULL;
    baidu_json *module_list_obj = NULL;
    baidu_json *meta_basic_info_obj = NULL;
    baidu_json *meta_install_info_obj = NULL;

    if (meta_data == NULL || meta == NULL) {
        DUER_LOGE("Decompress: Argument Error");

        return -1;
    }

    meta_obj = baidu_json_Parse(meta_data);

    if (meta_obj) {
        if (0 == duer_ota_unpack_get_json_value(meta_obj, "basic_info", &meta_basic_info_obj)) {
            DUER_LOGD("get basic info");
            if (0 == duer_ota_unpack_get_plain_value(
                        meta_basic_info_obj,
                        "app_name",
                        &value,
                        &length)) {
                DUER_LOGD("app_name:%s", value);
                if (length <= PACKAGE_NAME_LENGTH_MAX) {
                    memcpy(meta->basic_info.package_name, value, length);
                    meta->basic_info.package_name[length] = 0;
                }
                DUER_FREE(value);
                value = 0;
                length = 0;
            }
            baidu_json_Delete(meta_basic_info_obj);
            meta_basic_info_obj = 0;
        }

        if (0 == duer_ota_unpack_get_json_value(
                    meta_obj,
                    "inst_and_uninst_info",
                    &meta_install_info_obj)) {
            if (0 == duer_ota_unpack_get_json_value(
                        meta_install_info_obj,
                        "module_list",
                        &module_list_obj)) {
                module_count = baidu_json_GetArraySize(module_list_obj);
                if (module_count > 0) {
                    int i = 0;
                    meta->install_info.module_count = module_count;
                    meta->install_info.module_list = (duer_ota_module_info_t *)DUER_MALLOC( \
                            module_count * sizeof(duer_ota_module_info_t));
                    if (meta->install_info.module_list == NULL) {
                        DUER_LOGE("Decompress: Malloc failed");

                        return -1;
                    }
                    for (i = 0; i < module_count; i++) {
                        baidu_json *item = baidu_json_GetArrayItem(module_list_obj, i);
                        if (item){
                            // get module name
                            if (0 == duer_ota_unpack_get_plain_value(
                                        item,
                                        "name",
                                        &value,
                                        &length)) {
                                if (length <= MODULE_NAME_LENGTH_MAX){
                                    memcpy(meta->install_info.module_list[i].module_name,
                                            value,
                                            length);
                                    meta->install_info.module_list[i].module_name[length] = 0;
                                }
                                DUER_FREE(value);
                                value = 0;
                                length = 0;
                            }

                            // get module size
                            if (0 == duer_ota_unpack_get_plain_value(
                                        item,
                                        "size",
                                        &value,
                                        &length)) {
                                meta->install_info.module_list[i].module_size = atoi(value);
                                DUER_FREE(value);
                                value = 0;
                                length = 0;
                            }

                            // get module version
                            if (0 == duer_ota_unpack_get_plain_value(
                                        item,
                                        "version",
                                        &value,
                                        &length)) {
                                if (length <= MODULE_VERSION_LENGTH_MAX) {
                                    memcpy(meta->install_info.module_list[i].module_version,
                                            value,
                                            length);
                                    meta->install_info.module_list[i].module_version[length] = 0;
                                    DUER_FREE(value);
                                    value = 0;
                                    length = 0;
                                }
                            }
                        }
                    }
                }

                baidu_json_Delete(module_list_obj);
                module_list_obj = 0;
            }

            baidu_json_Delete(meta_install_info_obj);
            meta_install_info_obj = 0;
        }
        baidu_json_Delete(meta_obj);
        meta_obj = 0;
    }

    return 0;
}

int duer_ota_unpack_zlibstream_decompress_process(
        void *verify_cxt,
        duer_ota_decompress_context_t *ctx,
        unsigned char *buffer,
        uint32_t buffer_size,
		duer_ota_installer_t *installer,
        void *update_cxt)
{
    int ret = 0;
    int debug = 0;
    unsigned have;
    unsigned char out[CHUNK];
    uint16_t pck_header_sz = sizeof(duer_ota_package_header_t);

    if (verify_cxt == NULL||
            ctx == NULL ||
            buffer == NULL ||
            buffer_size == 0 ||
            installer == NULL) {
        DUER_LOGE("Decompress: Argument Error");

        return -7;
    }

    ctx->stream_recieved_sz += buffer_size;
    if (ctx->stream_recieved_sz <= pck_header_sz) {
        return 0;
    } else {
        if (ctx->stream_processed_sz == 0) {
            buffer = buffer + (buffer_size - (ctx->stream_recieved_sz - pck_header_sz));
            buffer_size = ctx->stream_recieved_sz - pck_header_sz;
        }
    }

    // parse verification context
    if (0 == ctx->meta_size){
        verification_context_t *verify = (verification_context_t *)verify_cxt;
        duer_ota_package_header_t *pk_header = (duer_ota_package_header_t *)verify->pck_header_sig_part;
        DUER_LOGD("meta size:%d", pk_header->meta_size);
        if (pk_header->meta_size > 0) {
            ctx->meta_data = (char *)DUER_MALLOC(pk_header->meta_size);
            if (ctx->meta_data) {
                ctx->meta_size = pk_header->meta_size;
                ctx->meta_stored_size = 0;
            }
        }
    }
    ctx->strmp->avail_in = buffer_size;
    ctx->strmp->next_in = buffer;

    // run inflate() on input until output buffer not full
    do {
        ctx->strmp->avail_out = CHUNK;
        ctx->strmp->next_out = out;
        ret = inflate(ctx->strmp, Z_NO_FLUSH);

        // assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
        switch (ret) {
        case Z_NEED_DICT:
            ret = Z_DATA_ERROR;     // and fall through
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            DUER_LOGI("Decompress: DATA ERROR");
            (void)inflateEnd(ctx->strmp);
            ctx->stream_processed_sz += buffer_size;

            return ret;
        }
        have = CHUNK - ctx->strmp->avail_out;
        debug = 0;
        if (ctx->meta_stored_size < ctx->meta_size) {
            unsigned int copy_byte = (have > (ctx->meta_size - ctx->meta_stored_size)) ? \
                                     (ctx->meta_size - ctx->meta_stored_size) : have;
            memcpy(ctx->meta_data + ctx->meta_stored_size, out, copy_byte);
            ctx->meta_stored_size += copy_byte;
            debug = copy_byte;
            if (ctx->meta_stored_size == ctx->meta_size) {
                DUER_LOGD("meta data has been completely copied, notify meta info");
                duer_ota_package_meta_data_t meta;
                memset(&meta, 0, sizeof(meta));
                if (0 == duer_ota_unpack_parse_meta_data(ctx->meta_data, &meta)) {
                    DUER_LOGD("parse_meta_data succeed");
                    DUER_LOGD("meta.basic_info.package_name:%s", meta.basic_info.package_name);
                    DUER_LOGD("module_count=%d", meta.install_info.module_count);
                    DUER_LOGD("module_list[0]:\nname:%s,\nsize:%d,\nversion:%s\n",
                            meta.install_info.module_list[0].module_name,
                            meta.install_info.module_list[0].module_size,
                            meta.install_info.module_list[0].module_version);

                    if (installer && installer->notify_meta_data && update_cxt) {
                        ret = installer->notify_meta_data(update_cxt, &meta);
                        if (ret != 0) {
                            return ret;
                        }
                    }
                }
            }
        }

        if (debug < have) {
            if (installer && installer->notify_module_data && update_cxt) {
                ret = installer->notify_module_data(update_cxt,
                        ctx->write_offset,
                        out + debug,
                        have - debug);
                ctx->write_offset += (have - debug);
                if (ret != 0) {
                    return ret;
                }
            }
        }
    } while (ctx->strmp->avail_out == 0);

    // DUER_LOGI("decompress_ctx->strmp->total_out=%d\n", ctx->strmp->total_out);
    ctx->stream_processed_sz += buffer_size;

    return 0;
}

/*
 * package decompress uninit
 *
 * param ctx decompress context
 */
void duer_ota_unpack_zlibstream_decompress_uninit(duer_ota_decompress_context_t *ctx)
{
    if (!ctx) {
        DUER_LOGE("Decompress: Argument Error");

        return;
    }

    if (ctx->strmp) {
        (void)inflateEnd(ctx->strmp);
        DUER_FREE(ctx->strmp);
    }

    if (ctx->meta_data){
        DUER_FREE(ctx->meta_data);
        ctx->meta_data = 0;
        ctx->meta_size = ctx->meta_stored_size = 0;
    }

    DUER_FREE(ctx);
}
