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
 * File: lightduer_ota_verification.c
 * Auth: Zhong Shuai (zhongshuai@baidu.com)
 * Desc: OTA verification
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "mbedtls/sha1.h"
#include "lightduer_memory.h"
#include "lightduer_ota_pack_info.h"
#include "lightduer_ota_verification.h"

#define KEY_LEN	 128
#define SHA1_LEN 20

#define RSA_N   "b7699a83da6100367cd9f43ac124" \
                "dd0f4fce5a4d39c02a2f844fb7867f" \
                "869cb535373d0022039bf5cb58e869" \
                "b041998baaf10d08705d043227eb4b" \
                "20204e63c0d409f3225c6eb013abc6" \
                "fef7d89435258eaa872c52020c0f07" \
                "fd3ce59fb6a042f6de4b2c26e50732" \
                "c8691cb744df77856b8b2a59066cca" \
                "ccd4e289d0440fe34b"

#define RSA_E   "10001"

#define MBEDPACK_MPI_CHK(ret) do { if( ret != 0 ) goto cleanup; } while( 0 )

#define offset_of(T, M) ((size_t)(&(((T*)0)->M)))

int duer_ota_unpack_rsa_ca_pkcs1_init(mbedtls_rsa_context *rsa_ctx)
{
    if (!rsa_ctx) {
        DUER_LOGE("Verification: Invalid rsa context");

        return -1;
    }

    mbedtls_rsa_init(rsa_ctx, MBEDTLS_RSA_PKCS_V15, 0);

    rsa_ctx->len = KEY_LEN;
    MBEDPACK_MPI_CHK(mbedtls_mpi_read_string( &rsa_ctx->N , 16, RSA_N));
    MBEDPACK_MPI_CHK(mbedtls_mpi_read_string( &rsa_ctx->E , 16, RSA_E));
    if(mbedtls_rsa_check_pubkey(rsa_ctx) != 0) {
        DUER_LOGE("Verification: Check key-pair failed");

        mbedtls_rsa_free(rsa_ctx);

        return -1;
    }

    return 0;
cleanup:
    mbedtls_rsa_free(rsa_ctx);

    return -1;
}

void duer_ota_unpack_rsa_ca_pkcs1_uninit(mbedtls_rsa_context *rsa_ctx)
{
   mbedtls_rsa_free(rsa_ctx);
}

verification_context_t *duer_ota_unpack_hash_init(void)
{
    verification_context_t *ctx = NULL;
    uint32_t sig_part_sz = sizeof(duer_ota_package_header_t);

    ctx = (verification_context_t *)DUER_MALLOC(sizeof(verification_context_t));
    if (!ctx) {
        DUER_LOGE("Verification: Malloc failed");

        return NULL;
    }

    // gloabl verification footprint
    ctx->stream_recieved_sz = 0;
    ctx->stream_processed_sz = 0;
    ctx->stream_sig_stored_sz = 0;

    ctx->pck_header_sig_part = (unsigned char *)DUER_MALLOC(sig_part_sz);
    if (ctx->pck_header_sig_part == NULL) {
        DUER_LOGE("Verification: Malloc failed");

        goto free_ctx;
    }

    ctx->ctx = (mbedtls_sha1_context *)DUER_MALLOC(sizeof(mbedtls_sha1_context));
    if (ctx->ctx == NULL) {
        DUER_LOGE("Verification: Malloc failed");

        goto free_pck_header;
    }
    memset(ctx->pck_header_sig_part, 0, sig_part_sz);

    if (!ctx->ctx) {
        DUER_FREE(ctx);

        return NULL;
    }

    mbedtls_sha1_starts(ctx->ctx);

    return ctx;

free_pck_header:
    DUER_FREE(ctx->pck_header_sig_part);
free_ctx:
    DUER_FREE(ctx);

    return NULL;
}
void duer_ota_unpack_hash_update_key(
        verification_context_t *ctx,
        unsigned char *buffer,
        uint32_t buffer_size)
{
    unsigned char *sig_buffer = NULL;
    uint16_t pck_header_sz = sizeof(duer_ota_package_header_t);

    if (ctx == NULL || buffer == NULL) {

        DUER_LOGE("Verification: Argument Error");

        return;
    }

    ctx->stream_recieved_sz += buffer_size;

    if (ctx->stream_sig_stored_sz < pck_header_sz) {
        if (ctx->stream_recieved_sz <= pck_header_sz) {
            sig_buffer = ctx->pck_header_sig_part + ctx->stream_sig_stored_sz;
            memcpy(sig_buffer, buffer, buffer_size);
            ctx->stream_sig_stored_sz += buffer_size;

            return;
        } else {
            sig_buffer = ctx->pck_header_sig_part + ctx->stream_sig_stored_sz;
            memcpy(sig_buffer, buffer, pck_header_sz - ctx->stream_sig_stored_sz);
            buffer = buffer + (pck_header_sz - ctx->stream_sig_stored_sz);
            buffer_size = buffer_size - (pck_header_sz - ctx->stream_sig_stored_sz);
            ctx->stream_sig_stored_sz = pck_header_sz;
        }
    }

    mbedtls_sha1_update(ctx->ctx, buffer, buffer_size);
    ctx->stream_processed_sz += buffer_size;
}

int duer_ota_rsa_ca_pkcs1_verify(mbedtls_rsa_context *rsa, verification_context_t *ctx)
{
    int ret = 0;
    int index = 0;
    unsigned char sha1sum[SHA1_LEN];
    unsigned char rsa_ciphertext[KEY_LEN];

    memcpy(rsa_ciphertext,
            ctx->pck_header_sig_part + offset_of(duer_ota_package_header_t, package_sig),
            KEY_LEN);

    for (index = 0; index < KEY_LEN; index++) {
        DUER_LOGD("%.2x", rsa_ciphertext[index]);
    }

    mbedtls_sha1_finish(ctx->ctx, sha1sum);
    for (index = 0; index < SHA1_LEN; index++) {
        DUER_LOGD("%.2x", sha1sum[index]);
    }

    if((ret = mbedtls_rsa_pkcs1_verify(rsa,
                    NULL,
                    NULL,
                    MBEDTLS_RSA_PUBLIC,
                    MBEDTLS_MD_SHA1,
                    0,
                    sha1sum,
                    rsa_ciphertext)) != 0 ) {
        DUER_LOGE("Verification: verify failed, ret:%d", ret);

        return -1;
    }

    DUER_LOGD("Verification: %s", rsa_ciphertext);

    return 0;
}

void duer_ota_unpack_hash_uninit(verification_context_t *ctx)
{
    if (!ctx) {
        DUER_LOGE("Verification: Argument Error");

        return;
    }

    if (ctx->pck_header_sig_part) {
        DUER_FREE(ctx->pck_header_sig_part);
    }

    if (ctx->ctx) {
        DUER_FREE(ctx->ctx);
    }

    DUER_FREE(ctx);
}
