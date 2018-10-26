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
 * File: lightduer_ota_package_api.c
 * Auth: Zhong Shuai (zhongshuai@baidu.com)
 * Desc: OTA package api
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "lightduer_log.h"
#include "lightduer_memory.h"
#include "lightduer_ota_verification.h"
#include "lightduer_ota_decompress.h"

void *duer_ota_unpack_verification_init(void)
{
    return (void *)duer_ota_unpack_hash_init();
}

void duer_ota_unpack_verification_update_ctx(
        void *ctx,
        unsigned char *buffer,
        uint32_t buffer_size)
{
    duer_ota_unpack_hash_update_key(ctx, buffer, buffer_size);
}

int duer_ota_unpack_verification(void *ctx)
{
    int ret = 0;
    mbedtls_rsa_context rsa_ctx = {0};

    if (duer_ota_unpack_rsa_ca_pkcs1_init(&rsa_ctx) != 0) {
        DUER_LOGE("Package: init rsa context failed");

        duer_ota_unpack_hash_uninit(ctx);

        return -1;
    }

    ret = duer_ota_rsa_ca_pkcs1_verify(&rsa_ctx, ctx);
    if (ret == -1) {
        DUER_LOGE("Package: verify failed, illegal package");

        duer_ota_unpack_rsa_ca_pkcs1_uninit(&rsa_ctx);
        duer_ota_unpack_hash_uninit(ctx);

        return -1;
    }
    duer_ota_unpack_rsa_ca_pkcs1_uninit(&rsa_ctx);
    duer_ota_unpack_hash_uninit(ctx);

    return ret;
}


void *duer_ota_unpack_decompress_init(void)
{
    return duer_ota_unpack_zlibstream_decompress_init();
}

int duer_ota_unpack_decompress_process(
        void *verify_cxt,
        void *ctx,
        unsigned char *buffer,
        uint32_t buffer_size,
        duer_ota_installer_t *installer,
        void *update_cxt)
{
    return duer_ota_unpack_zlibstream_decompress_process(
            verify_cxt,
            ctx,
            buffer,
            buffer_size,
            installer,
            update_cxt);
}

void duer_ota_unpack_decompress_uninit(void *ctx)
{
    duer_ota_unpack_zlibstream_decompress_uninit(ctx);
}
