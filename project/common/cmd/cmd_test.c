#include "cmd_util.h"
#include "common/framework/net_ctrl.h"
#include "lwip/netif.h"
#include "net/mbedtls/aes.h"

#include <stdio.h>
#include <string.h>

#include "kernel/os/os.h"

#include "tuya_storybot/st_inc/func/tuya_st_rgb_led.h"
#include "tuya_storybot/st_inc/func/cloud_app.h"
#include "tuya_storybot/st_inc/func/tuya_st_duer.h"
#include "tuya_storybot/st_inc/func/tuya_st_tone.h"

#include "sys/image.h"
#include "sys/ota.h"


#define TEST_THREAD_STACK_SIZE		(2 * 1024)
OS_Thread_t g_test_thread;

rgb_mode rgb_m;
rgb_status rgb_s;
extern cloud_base *cloud;

static void tone_callback(void *arg)
{
	printf("tone_callback\n");
}


void test_run(void *cmd)
{
/*
	// rgb led test
	static int a = 0;
	if (!a) {
		tuya_st_rgb_init();
		a = 1;
	}

	char* argv[3];
	cmd_parse_argv(cmd, argv, cmd_nitems(argv));

	if (!strcmp(argv[0], "r")) {
		rgb_m = R_LED;
	} else if (!strcmp(argv[0], "g")) {
		rgb_m = G_LED;
	} else if (!strcmp(argv[0], "b")) {
		rgb_m = B_LED;
	} else if (!strcmp(argv[0], "y")) {
		rgb_m = Y_LED;
	} else if (!strcmp(argv[0], "c")) {
		rgb_m = C_LED;
	} else if (!strcmp(argv[0], "f")) {
		rgb_m = F_LED;
	} else if (!strcmp(argv[0], "w")) {
		rgb_m = W_LED;
	} else if (!strcmp(argv[0], "a")) {
		rgb_m = COLO_LED;
	} else if (!strcmp(argv[0], "set")) {
		 tuya_st_rgb_light_ctrl(atoi(argv[1]));
		goto exit;
	}

	if (!strcmp(argv[1], "off")) {
		rgb_s = LED_OFF;
	} else if (!strcmp(argv[1], "on")) {
		rgb_s = LED_ON;
	} else if (!strcmp(argv[1], "s")) {
		rgb_s = LED_TWIK_S;
	} else if (!strcmp(argv[1], "f")) {
		rgb_s = LED_TWIK_F;
	} else if (!strcmp(argv[1], "ff")) {
		rgb_s = LED_TWIK_FF;
	} else if (!strcmp(argv[1], "br")) {
		rgb_s = LED_BREATH;
	}

	 tuya_st_rgb_mode_ctrl(rgb_m, rgb_s);
exit:
*/

/*
	// duer player test
	char* argv[3];
	cmd_parse_argv(cmd, argv, cmd_nitems(argv));

	if (!strcmp(argv[0], "rec")) {
		tuya_st_duer_input_event_set(TUYA_ST_REC_START);
	} else if (!strcmp(argv[0], "rec_stop")) {
		tuya_st_duer_input_event_set(TUYA_ST_REC_STOP);
	} else if (!strcmp(argv[0], "next")) {
		tuya_st_duer_input_event_set(TUYA_ST_NEXT);
	} else if (!strcmp(argv[0], "prev")) {
		tuya_st_duer_input_event_set(TUYA_ST_PREV);
	} else if (!strcmp(argv[0], "pause")) {
		tuya_st_duer_input_event_set(TUYA_ST_PAUSE_PLAY);
	}
*/
/*
	// tone test

	char* argv[3];
	cmd_parse_argv(cmd, argv, cmd_nitems(argv));

	if (!strcmp(argv[0], "1")) {
		tuya_st_tone_play_set(PAUSE_AUDIO_PLAY, ST_TONE_START_UP, tone_callback);
	} else if (!strcmp(argv[0], "2")) {
		tuya_st_tone_play_set(PAUSE_AUDIO_PLAY, ST_TONE_NET_CONTING, tone_callback);
	} else if (!strcmp(argv[0], "3")) {
		tuya_st_tone_play_set(PAUSE_AUDIO_PLAY, ST_TONE_NET_CON_SUCCESS, tone_callback);
	}
*/

	//OTA test
	const image_ota_param_t *iop;

	iop = image_get_ota_param();

	printf("ota_flash = %d\n", iop->ota_flash);
	printf("ota_size = %d\n", iop->ota_size);
	printf("ota_addr = %d\n", iop->ota_addr);
	printf("img_max_size = %d\n", iop->img_max_size);
	printf("bl_size = %d\n", iop->bl_size);
	printf("running_seq = %d\n", iop->running_seq);
	printf("flash[0] = %d flash[1] = %d\n", iop->flash[0], iop->flash[1]);
	printf("addr[0] = %d addr[1] = %d\n", iop->addr[0], iop->addr[1]);

	OS_ThreadDelete(&g_test_thread);
}

int test_start(void *param)
{
	if (OS_ThreadIsValid(&g_test_thread)) {
		CMD_ERR("TEST task is running\n");
		return -1;
	}
	if (OS_ThreadCreate(&g_test_thread,
				"test_thread",
				test_run,
				(void *)param,
				OS_THREAD_PRIO_APP,
				TEST_THREAD_STACK_SIZE) != OS_OK) {
		CMD_ERR("test task create failed\n");
		return -1;
	}
	return 0;
}


enum cmd_status cmd_test_exec(char *cmd)
{
	if (test_start(cmd) == -1)
		return CMD_STATUS_FAIL;

	return CMD_STATUS_OK;
}

