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
 * File: lightduer_ota_unpack.c
 * Auth: Zhong Shuai (zhongshuai@baidu.com)
 * Desc: OTA Unpack Framework
 */

#include "lightduer_ota_unpack.h"
#include <string.h>
#include <stdint.h>
#include "lightduer_ota_installer.h"
#include "lightduer_ota_package_api.h"
#include "lightduer_ota_verification.h"
#include "lightduer_log.h"
#include "lightduer_types.h"
#include "lightduer_mutex.h"
#include "lightduer_memory.h"
#include "lightduer_ota_downloader.h"

int duer_ota_unpack_decompress_data_begin(duer_ota_unpacker_t *unpacker)
{
    int ret = DUER_OK;
    int lock_ret = DUER_OK;

    if (unpacker == NULL
        || unpacker->installer == NULL
        || unpacker->installer->notify_data_begin == NULL) {
        DUER_LOGE("OTA Unpack: Argument Error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    lock_ret = duer_mutex_lock(unpacker->lock);
    if (lock_ret != DUER_OK) {
        DUER_LOGE("OTA Unpack: Lock OTA unpacker failed");

        ret = DUER_ERR_FAILED;

        goto out;
    }

    ret = unpacker->installer->notify_data_begin(unpacker->private_data);

    lock_ret = duer_mutex_unlock(unpacker->lock);
    if (lock_ret != DUER_OK) {
        DUER_LOGE("OTA Unpack: Unlock OTA unpacker failed");

        ret = DUER_ERR_FAILED;

        goto out;
    }
out:
    return ret;
}

int duer_ota_unpack_decompress_data_end(duer_ota_unpacker_t *unpacker)
{
    int ret = DUER_OK;
    int lock_ret = DUER_OK;

    if (unpacker == NULL
        || unpacker->installer == NULL
        || unpacker->installer->notify_data_end == NULL) {
        DUER_LOGE("OTA Unpack: Argument Error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    lock_ret = duer_mutex_lock(unpacker->lock);
    if (lock_ret != DUER_OK) {
        DUER_LOGE("OTA Unpack: Lock OTA unpacker failed");

        ret = DUER_ERR_FAILED;

        goto out;
    }

    ret = unpacker->installer->notify_data_end(unpacker->private_data);

    lock_ret = duer_mutex_unlock(unpacker->lock);
    if (lock_ret != DUER_OK) {
        DUER_LOGE("OTA Unpack: Unlock OTA unpacker failed");

        ret = DUER_ERR_FAILED;

        goto out;
    }
out:
    return ret;
}

int duer_ota_unpack_update_image_begin(duer_ota_unpacker_t *unpacker)
{
    int ret = DUER_OK;
    int lock_ret = DUER_OK;

    if (unpacker == NULL
        || unpacker->installer == NULL
        || unpacker->installer->update_img_begin == NULL) {
        DUER_LOGE("OTA Unpack: Argument Error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    lock_ret = duer_mutex_lock(unpacker->lock);
    if (lock_ret != DUER_OK) {
        DUER_LOGE("OTA Unpack: Lock OTA unpacker failed");

        ret = DUER_ERR_FAILED;

        goto out;
    }

    ret = unpacker->installer->update_img_begin(unpacker->private_data);

    lock_ret = duer_mutex_unlock(unpacker->lock);
    if (lock_ret != DUER_OK) {
        DUER_LOGE("OTA Unpack: Unlock OTA unpacker failed");

        ret = DUER_ERR_FAILED;

        goto out;
    }
out:
    return ret;
}

int duer_ota_unpack_update_image(duer_ota_unpacker_t *unpacker)
{
    int ret = DUER_OK;
    int lock_ret = DUER_OK;

    if (unpacker == NULL
        || unpacker->installer == NULL
        || unpacker->installer->update_img == NULL) {
        DUER_LOGE("OTA Unpack: Argument Error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    lock_ret = duer_mutex_lock(unpacker->lock);
    if (lock_ret != DUER_OK) {
        DUER_LOGE("OTA Unpack: Lock OTA unpacker failed");

        ret = DUER_ERR_FAILED;

        goto out;
    }

    ret = unpacker->installer->update_img(unpacker->private_data);

    lock_ret = duer_mutex_unlock(unpacker->lock);
    if (lock_ret != DUER_OK) {
        DUER_LOGE("OTA Unpack: Unlock OTA unpacker failed");

        ret = DUER_ERR_FAILED;

        goto out;
    }

out:
    return ret;
}

int duer_ota_unpack_update_image_end(duer_ota_unpacker_t *unpacker)
{
    int ret = DUER_OK;
    int lock_ret = DUER_OK;

    if (unpacker == NULL
        || unpacker->installer == NULL
        || unpacker->installer->update_img_end == NULL) {
        DUER_LOGE("OTA Unpack: Argument Error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    lock_ret = duer_mutex_lock(unpacker->lock);
    if (lock_ret != DUER_OK) {
        DUER_LOGE("OTA Unpack: Lock OTA unpacker failed");

        ret = DUER_ERR_FAILED;

        goto out;
    }

    ret = unpacker->installer->update_img_end(unpacker->private_data);

    lock_ret = duer_mutex_unlock(unpacker->lock);
    if (lock_ret != DUER_OK) {
        DUER_LOGE("OTA Unpack: Unlock OTA unpacker failed");

        ret = DUER_ERR_FAILED;

        goto out;
    }
out:
    return ret;
}

int duer_ota_unpack_decompress_data(duer_ota_unpacker_t *unpacker, unsigned char *data, size_t size)
{
    int ret = DUER_OK;

    duer_ota_unpack_verification_update_ctx(unpacker->verification, data, size);

    ret = duer_ota_unpack_decompress_process(
            unpacker->verification,
            unpacker->decompress,
            data,
            size,
            unpacker->installer,
            unpacker->private_data);
    if (ret != 0) {
        DUER_LOGE("OTA Unpack: Decompress failed ret: %d", ret);

        ret = DUER_ERR_FAILED;
    } else {
        ret = DUER_OK;
    }

    return ret;
}

duer_ota_unpacker_t *duer_ota_unpack_create_unpacker(void)
{
    duer_ota_unpacker_t *unpacker = NULL;

    unpacker = (duer_ota_unpacker_t *)DUER_MALLOC(sizeof(*unpacker));
    if (unpacker == NULL) {
        DUER_LOGE("OTA Unpack: Malloc failed");

        return NULL;
    }

    memset(unpacker, 0, sizeof(*unpacker));

    unpacker->lock = duer_mutex_create();
    if (unpacker->lock == NULL) {
        DUER_LOGE("OTA Unpack: Create lock failed");

        goto create_mutex_err;
    }

    unpacker->verification = duer_ota_unpack_verification_init();
    if (unpacker->verification == NULL) {
        DUER_LOGE("OTA Unpack: Init verification failed");

        goto init_verification_err;
    }

    unpacker->decompress = duer_ota_unpack_decompress_init();
    if (unpacker->decompress == NULL) {
        DUER_LOGE("OTA Unpack: Init decompress failed");

        goto init_decomprss_err;
    }

    return unpacker;

init_decomprss_err:
init_verification_err:
    duer_mutex_destroy(unpacker->lock);
create_mutex_err:
    DUER_FREE(unpacker);

    return NULL;
}

void duer_ota_unpack_destroy_unpacker(duer_ota_unpacker_t *unpacker)
{
    if (unpacker == NULL) {
        DUER_LOGE("OTA Unpack: Argument Error");

        return;
    }

    duer_ota_unpack_decompress_uninit(unpacker->decompress);
    duer_ota_unpack_hash_uninit(unpacker->verification);
    duer_mutex_destroy(unpacker->lock);
    DUER_FREE(unpacker);
}

void *duer_ota_unpack_get_private_data(duer_ota_unpacker_t *unpacker)
{
    int ret = DUER_OK;
    void *private_data = NULL;

    if (unpacker == NULL) {
        DUER_LOGE("OTA Unpack: Argument Error");

        goto out;
    }

    ret = duer_mutex_lock(unpacker->lock);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Unpack: Lock OTA unpacker failed");

        goto out;
    }

    private_data = unpacker->private_data;

    ret = duer_mutex_unlock(unpacker->lock);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Unpack: Unlock OTA unpacker failed");

        // What should I returrn ?
        goto out;
    }
out:
    return private_data;
}

int duer_ota_unpack_set_private_data(duer_ota_unpacker_t *unpacker, void *private_data)
{
    int ret = DUER_OK;
    int lock_ret = DUER_OK;

    if (unpacker == NULL || private_data == NULL) {
        DUER_LOGE("OTA Unpack: Argument Error");

        ret = DUER_ERR_INVALID_PARAMETER;

        goto out;
    }

    lock_ret = duer_mutex_lock(unpacker->lock);
    if (lock_ret != DUER_OK) {
        DUER_LOGE("OTA Unpack: Lock OTA unpacker failed");

        ret = DUER_ERR_FAILED;

        goto out;
    }

    unpacker->private_data = private_data;

    lock_ret = duer_mutex_unlock(unpacker->lock);
    if (lock_ret != DUER_OK) {
        DUER_LOGE("OTA Unpack: Unlock OTA unpacker failed");

        ret = DUER_ERR_FAILED;

        goto out;
    }
out:
    return ret;
}

int duer_ota_unpack_register_installer(duer_ota_unpacker_t *unpacker, duer_ota_installer_t *installer)
{
    int ret = DUER_OK;

    if (unpacker == NULL
        || installer == NULL
        || installer->notify_data_begin == NULL
        || installer->notify_meta_data == NULL
        || installer->notify_data_end == NULL
        || installer->notify_module_data == NULL
        || installer->update_img_begin == NULL
        || installer->update_img == NULL
        || installer->update_img_end == NULL) {
        DUER_LOGE("OTA Unpack: Argument Error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    unpacker->installer = installer;

    return ret;
}
