/***********************************************************
*  File: tuya_main.c
*  Author: luowq
*  Date: 20180911
***********************************************************/
#include "adapter_platform.h"
#include "tuya_iot_wifi_api.h"
#include "mf_test.h"
#include "tuya_uart.h"
#include "tuya_gpio.h"
#include "gw_intf.h"
#include "wf_basic_intf.h"

#include "sys/image.h"		//aw871
#include "sys/ota.h"
#include "console/console.h"
//#include "common/board/board.h"

#include "tuya_st_scene.h"	//luowq add
#include "tuya_st_prj_cfg.h"


/* function define */
#define TEST_SSID "tuya_mdev_test1"

/* AW add Define */
typedef struct xr_ota_parameter {
	const image_ota_param_t *iop;
	uint32_t get_size; 	/* data length */
	uint32_t seq;
} xr_ota_parameter_t;

#define OTA_UPDATE_DEBUG_SIZE_UNIT		(50 * 1024)
static xr_ota_parameter_t xr_ota;
static uint32_t debug_size = OTA_UPDATE_DEBUG_SIZE_UNIT;

/* normal define */
extern VOID app_init(VOID);
typedef VOID (*APP_PROD_CB)(BOOL_T flag, CHAR_T rssi);	//lql

STATIC APP_PROD_CB app_prod_test = NULL;	//lql
STATIC GW_WF_CFG_MTHD_SEL gwcm_mode = GWCM_OLD;	//lql

/* function define */
extern VOID pre_device_init(VOID);
extern OPERATE_RET device_init(VOID);

/* local function extern */
STATIC VOID __gw_ug_inform_cb(IN CONST FW_UG_S *fw);
STATIC OPERATE_RET __gw_upgrage_process_cb(IN CONST FW_UG_S *fw, IN CONST UINT_T total_len,IN CONST UINT_T offset,\
                                                      IN CONST BYTE_T *data,IN CONST UINT_T len,OUT UINT_T *remain_len, IN PVOID_T pri_data);
STATIC VOID __gw_upgrade_notify_cb(IN CONST FW_UG_S *fw, IN CONST INT_T download_result, IN PVOID_T pri_data);

STATIC void __mf_gw_ug_inform_cb(void);
STATIC OPERATE_RET __mf_gw_upgrade_notify_cb(VOID);

/***********************************************************
*  Function: uart_init
*  Input: baud, bufsz
*  Output: none
*  Return: none
***********************************************************/
#define TUYA_USE_UART	TY_UART0

STATIC VOID __tuya_mf_uart_init(UINT_T baud,UINT_T bufsz)
{
    ty_uart_init(TUYA_USE_UART,baud,TYWL_8B,TYP_NONE,TYS_STOPBIT1,bufsz);
}

STATIC VOID __tuya_mf_uart_free(VOID)
{
    ty_uart_free(TUYA_USE_UART);
}

STATIC VOID __tuya_mf_send(IN CONST BYTE_T *data,IN CONST UINT_T len)
{
    ty_uart_send_data(TUYA_USE_UART,data,len);
}

STATIC UINT_T __tuya_mf_recv(OUT BYTE_T *buf,IN CONST UINT_T len)
{
    return ty_uart_read_data(TUYA_USE_UART,buf,len);
}

STATIC BOOL_T scan_test_ssid(VOID)
{
	OPERATE_RET op_ret = OPRT_OK;

	/* special for GWCM_OLD_PROD.......only do prodtesting when in smartlink or ap mode */
	if(gwcm_mode == GWCM_OLD_PROD ) {
        op_ret = wd_gw_wsm_read(&(get_gw_cntl()->gw_wsm));
        if(get_gw_cntl()->gw_wsm.nc_tp >= GWNS_TY_SMARTCFG) {
            return false;
        }
	}

	wf_wk_mode_set(WWM_STATION);
	AP_IF_S *ap = NULL;
	BOOL_T flag = TRUE;
	op_ret = wf_assign_ap_scan(TEST_SSID, &ap);	//lql
	wf_station_disconnect();
	if(OPRT_OK != op_ret) {
	    PR_NOTICE("wf_assign_ap_scan failed(%d)",op_ret);
		return FALSE;
	}
    /* check if has authorized */
    op_ret = wd_gw_base_if_read(&(get_gw_cntl()->gw_base));
    if(OPRT_OK != op_ret) {
        PR_DEBUG("read flash err");
        flag = FALSE;
    }
    /* gateway base info verify */
    if(0 == get_gw_cntl()->gw_base.auth_key[0] || \
       0 == get_gw_cntl()->gw_base.uuid[0]) {
        PR_DEBUG("please write uuid and auth_key first");
        flag = FALSE;
    }

	if(app_prod_test) {
		app_prod_test(flag, ap->rssi);
	}
	return TRUE;
}

void app_cfg_set(IN CONST GW_WF_CFG_MTHD_SEL mthd, APP_PROD_CB callback)
{
	app_prod_test = callback;
	gwcm_mode = mthd;
}

static BOOL_T other_some_test(VOID)
{
	return TRUE;
}

/***********************************************************
*  Function: user_main
*  Input: none
*  Output: none
*  Return: none
***********************************************************/
extern int stdout_init(void);
extern int stdout_deinit(void);

int user_main(void)
{
    OPERATE_RET op_ret = OPRT_OK;

    op_ret = tuya_iot_init(NULL);
    if(OPRT_OK != op_ret) {
        PR_ERR("tuya_iot_init err:%d",op_ret);
        return -1;
    }
    pre_device_init();

#if USE_TUYA_PRODUCT_TEST
    /* to add prodect test code */
	if (TUYA_USE_UART == TY_UART0) {
		op_ret = stdout_deinit();
		if (op_ret != 0) {
			PR_ERR("stdout_deinit err:%d",op_ret);
		}
	}

    mf_reg_gw_ug_cb(__mf_gw_ug_inform_cb, __gw_upgrage_process_cb, __mf_gw_upgrade_notify_cb);
    MF_IMPORT_INTF_S intf = {
        __tuya_mf_uart_init,
        __tuya_mf_uart_free,
        __tuya_mf_send,
        __tuya_mf_recv,
        other_some_test,
    };
    op_ret = mf_init(&intf, APP_BIN_NAME, USER_SW_VER, TRUE);
    if(OPRT_OK != op_ret) {
        PR_ERR("mf_init err:%d",op_ret);
        return -2;
    }
    __tuya_mf_uart_free();

	if (TUYA_USE_UART == TY_UART0) {
		op_ret = stdout_init();
		if (op_ret != 0) {
			PR_ERR("stdout_init err:%d",op_ret);
		}
	}
    PR_NOTICE("mf_init succ");
#endif

#if 1
    /* register gw upgrade inform callback */
    tuya_iot_force_reg_gw_ug_cb(__gw_ug_inform_cb);
#endif

/* for debug if not authorize */
#if !USE_TUYA_PRODUCT_TEST
    WF_GW_PROD_INFO_S wf_prod_info = {
    	//"003tuyatestf7f149189","NeA8Wc7srpAZHEMuru867oblOLN2QCC5",NULL,NULL
    	"002debug", "WbqlfSpmlwwAplW8ltDzoApxRqWJt5D1", NULL, NULL
    };
    op_ret = tuya_iot_set_wf_gw_prod_info(&wf_prod_info);
    if(OPRT_OK != op_ret) {
        PR_ERR("tuya_iot_set_wf_gw_prod_info err:%d",op_ret);
        return -3;
    }
#endif

    app_init();
    PR_DEBUG("gwcm_mode %d",gwcm_mode);

    if (gwcm_mode != GWCM_OLD) {
        PR_DEBUG("low_power select");
    	if (true == scan_test_ssid()) {
    		PR_DEBUG("prodtest");
    		return 1;
    	}

        PR_DEBUG("no tuya_mdev_test1!");
    }

	PR_DEBUG("device_init in");
    op_ret = device_init();
    if (OPRT_OK != op_ret) {
        PR_ERR("device_init err:%d",op_ret);
        return -4;
    }

	return 0;
}

/* gateway upgrade start */
STATIC VOID __gw_ug_inform_cb(IN CONST FW_UG_S *fw)
{
#if 1
	OPERATE_RET op_ret = OPRT_OK;
	image_seq_t seq;

	int ret = 0;	//add by luowq
	tuya_scene_ota_check(&ret);
	if (ret != 0) {
		PR_ERR("Can't Not OTA!! ret = %d \n", ret);
		return;
	}

	PR_INFO("ota_notify_data_begin enter");
	ota_init();
	PR_INFO("flash erasing ...");
	memset(&xr_ota, 0, sizeof(xr_ota_parameter_t));

	xr_ota.iop = image_get_ota_param();
	xr_ota.seq = (xr_ota.iop->running_seq + 1) % IMAGE_SEQ_NUM;
	seq = xr_ota.seq;
	PR_INFO("image seq = %d", xr_ota.seq);

	PR_INFO("seq %d, flash %u, addr %#x",seq,xr_ota.iop->flash[seq], xr_ota.iop->addr[seq]);
	PR_INFO("erase img_max_size = %#x", xr_ota.iop->img_max_size);

	if (uni_flash_erase(xr_ota.iop->addr[seq], xr_ota.iop->img_max_size) != 0) {
		PR_ERR("flash_erase failed");
		return;
	}

	PR_INFO("erase flash success");
    op_ret = tuya_iot_upgrade_gw(fw,__gw_upgrage_process_cb,__gw_upgrade_notify_cb,NULL);
    if(OPRT_OK != op_ret) {
        PR_ERR("tuya_iot_upgrade_gw err:%d",op_ret);
        return;
    }
#endif
}

/* gateway upgrade result notify */
STATIC VOID __gw_upgrade_notify_cb(IN CONST FW_UG_S *fw, IN CONST INT_T download_result, IN PVOID_T pri_data)
{
#if 1
    if(OPRT_OK == download_result) { // update success
        ota_set_get_size(xr_ota.get_size);
        // checksum
		if (image_check_sections(xr_ota.seq) == IMAGE_INVALID) {
			PR_ERR("ota check image failed");
			return;
		}

		PR_INFO("OTA: finish checking image.");
		// verify
		if (ota_verify_image(OTA_VERIFY_NONE, NULL) != OTA_STATUS_OK) {
			PR_ERR("OTA http verify image failed");
			return;
		}

        PR_INFO("the gateway upgrade success");
		tuya_scene_ota_is_ok(true, ota_reboot);	//add by luowq
    }
	else {
        PR_ERR("the gateway upgrade failed");
		tuya_scene_ota_is_ok(false, NULL);		//add by luowq
    }
#endif
}

/* gateway upgrade process */
STATIC OPERATE_RET __gw_upgrage_process_cb(IN CONST FW_UG_S *fw, IN CONST UINT_T total_len,IN CONST UINT_T offset,\
                                                      IN CONST BYTE_T *data,IN CONST UINT_T len,OUT UINT_T *remain_len, IN PVOID_T pri_data)
{
#if 1
	OPERATE_RET ret = OPRT_OK;
	uint32_t _off_set = 0;

	static char prtf_first_time = 1;
	if (prtf_first_time) {
		prtf_first_time = 0;
		PR_INFO("+++++upgrage_process_cb+++++\n");
	}

	if(total_len > (xr_ota.iop->img_max_size + xr_ota.iop->bl_size)) {
		PR_ERR("total_len is too big, total_len %u maxz_size %u",total_len, (xr_ota.iop->img_max_size + xr_ota.iop->bl_size));
		return OPRT_INVALID_PARM;
	}

	if(xr_ota.get_size + len < xr_ota.iop->bl_size) {
		;	//do nothing, for skip the bootload part
	} else if(xr_ota.get_size >= xr_ota.iop->bl_size) {
		_off_set = xr_ota.get_size - xr_ota.iop->bl_size;
		_off_set += xr_ota.iop->addr[xr_ota.seq];
	    ret = uni_flash_write(_off_set, data, len);
		if (ret != 0) {
			PR_ERR("[tuya ota] data flash write err!");
			return OPRT_WR_FLASH_ERROR;
		}
	} else {
		int rem;

		rem = xr_ota.get_size + len - xr_ota.iop->bl_size;
		_off_set = 0;
		_off_set += xr_ota.iop->addr[xr_ota.seq];
		ret = uni_flash_write(_off_set, &data[len-rem], rem);
		if(ret != 0) {
			PR_ERR("[duer ota] data flash write err!");
			return OPRT_WR_FLASH_ERROR;
		}
	}

	xr_ota.get_size += len;
	*remain_len = 0;

	if (xr_ota.get_size >= debug_size) {
		PR_INFO("OTA: loading image (%u KB)...",xr_ota.get_size / 1024);
		debug_size += OTA_UPDATE_DEBUG_SIZE_UNIT;
	}
#endif
	 return OPRT_OK;
}

STATIC void __mf_gw_ug_inform_cb(void)
{
	return;
}

STATIC OPERATE_RET __mf_gw_upgrade_notify_cb(VOID)
{
	return OPRT_OK;
}

