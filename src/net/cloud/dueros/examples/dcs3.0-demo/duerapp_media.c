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
 * File: duerapp_media.c
 * Auth: Renhe Zhang (v_zhangrenhe@baidu.com)
 * Desc: Media module function implementation.
 */

#include <gst/gst.h>
#include <assert.h>

#include "duerapp_media.h"
#include "duerapp_config.h"
#include "lightduer_dcs.h"

#define VOLUME_MAX (10.0)
#define VOLUME_MIX (0.000001)
#define VOLUME_INIT (2.0)

typedef struct duer_speak_param_t{
    duer_speak_state_t state;
    pthread_t threadID;
    GstElement *pip;
    GMainLoop *loop;
}duer_speak_param_t;

typedef struct duer_audio_param_t{
    duer_audio_state_t state;
    pthread_t threadID;
    GstElement *pip;
    GMainLoop *loop;
    char *url;
    int seek;
}duer_audio_param_t;

static duer_speak_param_t *s_speak = NULL;
static duer_audio_param_t *s_audio = NULL;
static double s_vol = VOLUME_INIT;
static bool s_mute = false;

void duer_media_init()
{
    s_speak = (duer_speak_param_t *)malloc(sizeof(duer_speak_param_t));
    if (s_speak) {
        memset(s_speak, 0, sizeof(duer_speak_param_t));
    } else {
        DUER_LOGE("malloc s_spead failed!");
        exit(1);
    }

    s_audio = (duer_audio_param_t *)malloc(sizeof(duer_audio_param_t));
    if (s_audio) {
        memset(s_audio, 0, sizeof(duer_audio_param_t));
    } else {
        DUER_LOGE("malloc s_audio failed!");
        exit(1);
    }

    s_speak->state = MEDIA_SPEAK_STOP;
    s_speak->pip = NULL;
    s_speak->loop = NULL;

    s_audio->state = MEDIA_AUDIO_STOP;
    s_audio->pip = NULL;
    s_audio->loop = NULL;
    s_audio->url = NULL;
    s_audio->seek = 0;

    gst_init(NULL, NULL);
}

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
    GMainLoop *loop = (GMainLoop *)data;

    switch (GST_MESSAGE_TYPE(msg)) {

        case GST_MESSAGE_EOS:
            g_main_loop_quit(loop);
            break;

        case GST_MESSAGE_ERROR: {
                gchar  *debug = NULL;
                GError *error = NULL;

                gst_message_parse_error(msg, &error, &debug);
                g_free(debug);

                DUER_LOGE("gstreamer play : %s\n", error->message);
                duer_dcs_audio_play_failed(DCS_MEDIA_ERROR_INTERNAL_DEVICE_ERROR,
                    error->message);
                g_error_free(error);

                g_main_loop_quit(loop);
            }
            break;
        default:
            break;
    }
}

static void speak_thread()
{
    pthread_detach(pthread_self());
    if (s_mute) {
        g_object_set(G_OBJECT(s_speak->pip), "volume", 0.0, NULL);
    } else {
        g_object_set(G_OBJECT(s_speak->pip), "volume", s_vol, NULL);
    }
    GstBus *speak_bus = gst_pipeline_get_bus(GST_PIPELINE(s_speak->pip));
    s_speak->loop = g_main_loop_new(NULL, FALSE);

    guint speak_bus_watch_id = gst_bus_add_watch(speak_bus, bus_call, s_speak->loop);
    gst_object_unref(speak_bus);
    gst_element_set_state(s_speak->pip, GST_STATE_PLAYING);
    g_main_loop_run(s_speak->loop);
    gst_element_set_state(s_speak->pip, GST_STATE_NULL);

    gst_object_unref(GST_OBJECT(s_speak->pip));
    s_speak->pip = NULL;
    g_source_remove(speak_bus_watch_id);
    g_main_loop_unref(s_speak->loop);
    s_speak->loop = NULL;

    if (MEDIA_SPEAK_PLAY == s_speak->state) {
        s_speak->state = MEDIA_SPEAK_STOP;
        duer_dcs_speech_on_finished();
    }
}

void duer_media_speak_play(const char *url)
{
    if (MEDIA_SPEAK_PLAY == s_speak->state) {
        duer_media_speak_stop();
    }

    s_speak->pip = gst_element_factory_make("playbin", "speak");
    assert(s_speak->pip != NULL);
    g_object_set(G_OBJECT(s_speak->pip), "uri", url, NULL);
    int ret = pthread_create(&s_speak->threadID, NULL, (void *)speak_thread, NULL);
    if(ret != 0)
    {
        DUER_LOGE("Create media speak pthread error!");
        exit(1);
    }
    s_speak->state = MEDIA_SPEAK_PLAY;
}

void duer_media_speak_stop()
{
    if (MEDIA_SPEAK_PLAY == s_speak->state) {
        s_speak->state = MEDIA_SPEAK_STOP;
        g_main_loop_quit(s_speak->loop);
    } else {
        DUER_LOGI("Speak stop state : %d", s_speak->state);
    }
}

static void audio_thread()
{
    pthread_detach(pthread_self());
    if (s_mute) {
        g_object_set(G_OBJECT(s_audio->pip), "volume", 0.0, NULL);
    } else {
        g_object_set(G_OBJECT(s_audio->pip), "volume", s_vol, NULL);
    }
    GstBus *audio_bus = gst_pipeline_get_bus(GST_PIPELINE(s_audio->pip));
    s_audio->loop = g_main_loop_new(NULL, FALSE);

    guint audio_bus_watch_id = gst_bus_add_watch(audio_bus, bus_call, s_audio->loop);
    gst_object_unref(audio_bus);
    gst_element_set_state(s_audio->pip, GST_STATE_PLAYING);
    g_main_loop_run(s_audio->loop);

    if (MEDIA_AUDIO_PLAY == s_audio->state) {
        gst_element_set_state(s_audio->pip, GST_STATE_NULL);
        gst_object_unref(GST_OBJECT(s_audio->pip));
        s_audio->pip = NULL;
        s_audio->state = MEDIA_AUDIO_STOP;
        duer_dcs_audio_on_finished();
        s_audio->seek = 0;
    } else if (MEDIA_AUDIO_STOP == s_audio->state) {
        gst_element_set_state(s_audio->pip, GST_STATE_NULL);
        gst_object_unref(GST_OBJECT(s_audio->pip));
        s_audio->pip = NULL;
        s_audio->seek = 0;
    } else if (MEDIA_AUDIO_PAUSE == s_audio->state) {
        gst_element_set_state(s_audio->pip, GST_STATE_PAUSED);
    } else {
        // do nothing
    }

    g_source_remove(audio_bus_watch_id);
    g_main_loop_unref(s_audio->loop);
    s_audio->loop = NULL;
}

void duer_media_audio_start(const char *url)
{
    if (MEDIA_AUDIO_STOP != s_audio->state) {
        gst_element_set_state(s_audio->pip, GST_STATE_NULL);
        gst_object_unref(GST_OBJECT(s_audio->pip));
        s_audio->pip = NULL;
        s_audio->state = MEDIA_AUDIO_STOP;
    }

    s_audio->pip = gst_element_factory_make("playbin", "audio");
    assert(s_audio->pip != NULL);
    g_object_set(G_OBJECT(s_audio->pip), "uri", url, NULL);
    int ret = pthread_create(&s_audio->threadID, NULL, (void *)audio_thread, NULL);
    if (ret != 0)
    {
        DUER_LOGE("Create media audio pthread error!");
        exit(1);
    }

    if (s_audio->url) {
        free(s_audio->url);
        s_audio->url = NULL;
    }
    s_audio->url = (char *)malloc(strlen(url) + 1);
    if (s_audio->url) {
        memcpy(s_audio->url, url, strlen(url));
        s_audio->url[strlen(url)] = '\0';
    } else {
        DUER_LOGE("malloc url failed!");
    }

    s_audio->state = MEDIA_AUDIO_PLAY;
}

void duer_media_audio_resume(const char *url, int offset)
{
    if (MEDIA_AUDIO_PAUSE == s_audio->state) {
        if ((offset == s_audio->seek) && (s_audio->state)) {
            int ret = pthread_create(&s_audio->threadID, NULL, (void *)audio_thread, NULL);
            if(ret != 0)
            {
                DUER_LOGE("Create media audio pthread error!");
                exit(1);
            }
            s_audio->state = MEDIA_AUDIO_PLAY;
        } else {
            duer_media_audio_start(url);
        }
    } else if (MEDIA_AUDIO_STOP == s_audio->state) {
        duer_media_audio_start(url);
    } else {
        DUER_LOGI("Audio seek state : %d", s_audio->state);
    }
}

void duer_media_audio_stop()
{
    if (MEDIA_AUDIO_PLAY == s_audio->state) {
        s_audio->state = MEDIA_AUDIO_STOP;
        g_main_loop_quit(s_audio->loop);
    } else if (MEDIA_AUDIO_PAUSE == s_audio->state) {
        gst_element_set_state(s_audio->pip, GST_STATE_NULL);
        gst_object_unref(GST_OBJECT(s_audio->pip));
        s_audio->pip = NULL;
        s_audio->state = MEDIA_AUDIO_STOP;
    } else {
        DUER_LOGI("Audio stop state : %d", s_audio->state);
    }
}

void duer_media_audio_pause()
{
    if (MEDIA_AUDIO_PLAY == s_audio->state) {
        s_audio->state = MEDIA_AUDIO_PAUSE;
        g_main_loop_quit(s_audio->loop);
    } else {
        DUER_LOGI("Audio pause state : %d", s_audio->state);
    }
}

int duer_media_audio_get_position()
{
    gint64 pos = 0;
    if (!s_audio->pip) {
        return 0;
    }
    if (gst_element_query_position(s_audio->pip, GST_FORMAT_TIME, &pos)) {
    s_audio->seek = pos / GST_MSECOND;
    }
    return s_audio->seek;
}

duer_audio_state_t duer_media_audio_state()
{
    return s_audio->state;
}

void duer_media_volume_change(int volume)
{
    if (s_mute) {
        return;
    }
    s_vol += volume / 10.0;
    if (s_vol < VOLUME_MIX) {
        s_vol = 0.0;
    } else if (s_vol > VOLUME_MAX){
        s_vol = VOLUME_MAX;
    } else {
        // do nothing
    }

    if (s_speak->pip) {
        g_object_set(G_OBJECT(s_speak->pip), "volume", s_vol, NULL);
    }

    if (s_audio->pip) {
        g_object_set(G_OBJECT(s_audio->pip), "volume", s_vol, NULL);
    }
    DUER_LOGI("volume : %.1f", s_vol);
    if (DUER_OK == duer_dcs_on_volume_changed()) {
        DUER_LOGI("volume change OK");
    }
}

void duer_media_set_volume(int volume)
{
    if (s_mute) {
        return;
    }
    s_vol = volume / 10.0;
    if (s_speak->pip) {
        g_object_set(G_OBJECT(s_speak->pip), "volume", s_vol, NULL);
    }

    if (s_audio->pip) {
        g_object_set(G_OBJECT(s_audio->pip), "volume", s_vol, NULL);
    }
    DUER_LOGI("volume : %.1f", s_vol);
    if (DUER_OK == duer_dcs_on_volume_changed()) {
        DUER_LOGI("volume change OK");
    }
}

int duer_media_get_volume()
{
    return (int)(s_vol * 10);
}

void duer_media_set_mute(bool mute)
{
    s_mute = mute;

    if (mute) {
        if (s_speak->pip) {
            g_object_set(G_OBJECT(s_speak->pip), "volume", 0.0, NULL);
        }

        if (s_audio->pip) {
            g_object_set(G_OBJECT(s_audio->pip), "volume", 0.0, NULL);
        }
    } else {
        if (s_speak->pip) {
            g_object_set(G_OBJECT(s_speak->pip), "volume", s_vol, NULL);
        }

        if (s_audio->pip) {
            g_object_set(G_OBJECT(s_audio->pip), "volume", s_vol, NULL);
        }
    }
    duer_dcs_on_mute();
}

bool duer_media_get_mute()
{
    return s_mute;
}

void duer_media_destroy()
{
    // Clear
    duer_media_speak_stop();
    duer_media_audio_stop();

    if (s_audio->url) {
        free(s_audio->url);
        s_audio->url = NULL;
    }

    if (s_audio) {
        free(s_audio);
        s_audio = NULL;
    }

    if (s_speak) {
        free(s_speak);
        s_speak = NULL;
    }
}
