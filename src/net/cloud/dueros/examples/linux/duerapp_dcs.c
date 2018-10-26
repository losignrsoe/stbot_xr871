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
 * File: duerapp_dcs.c
 * Auth: Gang Chen(chengang12@baidu.com)
 * Desc: Implement DCS related APIs.
 */

#include "lightduer_log.h"
#include "lightduer_dcs.h"

void duer_dcs_stop_listen_handler(void)
{
    DUER_LOGI("Stop listen");
}

void duer_dcs_speak_handler(const char *url)
{
    DUER_LOGI("Speak url: %s", url);
    duer_dcs_speech_on_finished();
}

void duer_dcs_audio_play_handler(const duer_dcs_audio_info_t *audio_info)
{
    DUER_LOGI("Audio url: %s", audio_info->url);
}

