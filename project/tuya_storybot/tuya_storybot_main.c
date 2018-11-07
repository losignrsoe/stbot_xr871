/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "common/framework/fs_ctrl.h"
#include "common/framework/net_ctrl.h"
#include "kernel/os/os.h"
#include "fs/fatfs/ff.h"

#include "player_app.h"
#include "tuya_st_key.h"
#include "tuya_st_knob.h"
#include "tuya_st_duer.h"
#include "tuya_st_tone.h"
#include "tuya_st_elect_det.h"
#include "tuya_st_charge_det.h"
#include "tuya_st_rgb_led.h"
#include "tuya_st_scene.h"
#include "tuya_st_arch.h"
#include "tuya_st_log.h"
#include "tuya_st_prj_cfg.h"


player_base *player;
volatile st_scene_m bot_scene;

extern void duer_media_set_volume(int volume);

static void key_callback(key_value_type_m key_type, key_trig_type_m trig_type, uint8_t cnt)
{
	tuya_arch_key_manage(key_type, trig_type);
}

static void knob_callback(uint32_t volume, void *arg)
{
	//TUYA_ST_LOG(DBG_DBG, "volume = %d\n", volume);
	duer_media_set_volume(volume);
}

static void elect_capacity_callback(uint8_t now_cap)
{
	tuya_arch_lpower_call(now_cap);
}

/* 必须在开机最开始(tuya-iot前面), 进行初始化的功能函数 */
void tuya_storybot_main_pre(void)
{
	int ret = 0;

	TUYA_ST_LOG(DBG_DBG, "tuya_storybot_main_pre!\n");
	ret = fs_mount_request(FS_MNT_DEV_TYPE_SDCARD, 0, FS_MNT_MODE_MOUNT);
	if (ret != FR_OK) {
		TUYA_ST_LOG(DBG_ERR, "[ERROR] !!!!!  can not mount\n");
	} else {
		TUYA_ST_LOG(DBG_INFO, "mount success\n");
	}

	player = awplayer_create();	//player最好最先创建
	player->setvol(player, 0);

	/* st function define */
	tuya_st_duer_start();

	tuya_st_tone_play_init();

	tuya_st_rgb_init();

	tuya_st_key_init();
	tuya_st_key_set_cb(key_callback);

#if USE_KNOB_TO_SET_VOICE
	tuya_st_knob_init();
	tuya_st_knob_set_cb(knob_callback);
#endif

	tuya_st_elect_capacity_init();
	tuya_st_elect_capacity_set_cb(elect_capacity_callback);
}

/* 在tuya-iot模块tuya_main.c user_main产测之后声明 */
void tuya_storybot_main_init(void)
{
	TUYA_ST_LOG(DBG_DBG, "tuya_storybot_main_init!\n");
	tuya_arch_led_sta_manage();

#if !USE_KNOB_TO_SET_VOICE
	duer_media_set_volume(SET_DEFAULT_VOICE);
#endif

	tuya_arch_dsleep_check();

	tuya_arch_time_1s_count();

	tuya_arch_reboot_sta_jude();

	tuya_arch_report_dective_task();
}

extern int user_main(void);

void tuya_iot_user_main(void)
{
	int ret = 0;

	TUYA_ST_LOG(DBG_DBG, "tuya_iot_user_main!\n");
	ret = user_main();
	if (ret < 0) {
		TUYA_ST_LOG(DBG_ERR, "tuya_iot_user_main Err, ret = %d!\n", ret);
		return;
	}
}
