#if 1
/***********************************************************
*  File: tuya_uart.c
*  Author: nzy
*  Date: 20171106
***********************************************************/
#define __TUYA_UART_GLOBALS
#include "tuya_uart.h"
#include "uni_log.h"
#include "driver/chip/hal_uart.h"
#include "tuya_cloud_types.h"
#include "driver/chip/hal_util.h"

/***********************************************************
*************************micro define***********************
***********************************************************/
typedef struct {
  //  serial_t sobj;
  	UART_T *uart;
  	TY_UART_PORT_E portid;
    UINT_T buf_len;
    BYTE_T *buf;
    USHORT_T in;
    USHORT_T out;
}TUYA_UART_S;

/***********************************************************
*************************variable define********************
***********************************************************/
STATIC TUYA_UART_S ty_uart[TY_UART_NUM];

/***********************************************************
*************************function define********************
***********************************************************/
STATIC UINT_T __ty_uart_read_data_size(IN CONST TY_UART_PORT_E port)
{
    UINT_T remain_buf_size = 0;

    if(ty_uart[port].in >= ty_uart[port].out) {
        remain_buf_size = ty_uart[port].in-ty_uart[port].out;
    }else {
        remain_buf_size = ty_uart[port].in + ty_uart[port].buf_len - ty_uart[port].out;
    }

    return remain_buf_size;
}

static void rx_callback(void *arg)
{
	TUYA_UART_S *uart_param;
	uint8_t data = 0;

	uart_param = (TUYA_UART_S*)arg;
	if (HAL_UART_IsRxReady(uart_param->uart)) {
		data = HAL_UART_GetRxData(uart_param->uart);
		//PR_DEBUG("data = %d", data);
	}else {
        return;
	}
	
	if(__ty_uart_read_data_size(uart_param->portid) < uart_param->buf_len - 1) {
    	uart_param->buf[uart_param->in++] = data;
        if(uart_param->in >= uart_param->buf_len) {
        	uart_param->in = 0;
        }
    }
}

/***********************************************************
*  Function: ty_uart_init
*  Input: port badu bits parity stop
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET ty_uart_init(IN CONST TY_UART_PORT_E port,IN CONST TY_UART_BAUD_E badu,\
                               IN CONST TY_DATA_BIT_E bits,IN CONST TY_PARITY_E parity,\
                               IN CONST TY_STOPBITS_E stop,IN CONST UINT_T bufsz)
{
	HAL_Status status = HAL_ERROR;
	UART_InitParam param;

    if((port >= TY_UART_NUM) || (bufsz == 0)) {
        return OPRT_INVALID_PARM;
    }

    if(ty_uart[port].buf == NULL) {
        memset(&ty_uart[port], 0, sizeof(TUYA_UART_S));
        ty_uart[port].buf = Malloc(bufsz);
        if(ty_uart[port].buf == NULL) {
            return OPRT_MALLOC_FAILED;
        }
        ty_uart[port].buf_len = bufsz;
        PR_DEBUG("uart buf size : %d",bufsz);
    }else {
        return OPRT_COM_ERROR;
    }
    
	param.baudRate = badu;
	param.dataBits = bits;
	param.stopBits = stop;
	param.parity = parity;
	param.isAutoHwFlowCtrl = 0;

	status = HAL_UART_Init(port, &param);
	if (status != HAL_OK) {
		PR_ERR("uart init error %d", status);
		return OPRT_COM_ERROR;
	}

	ty_uart[port].portid = port;
	ty_uart[port].uart = HAL_UART_GetInstance(port);
	HAL_UART_EnableRxCallback(port, rx_callback, &ty_uart[port]);

	//UART_EnableTxReadyIRQ(ty_uart[port].uart);

    ty_uart[port].in = 0;
    ty_uart[port].out = 0;

    return OPRT_OK;
}

/***********************************************************
*  Function: ty_uart_free
*  Input:free uart
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET ty_uart_free(IN CONST TY_UART_PORT_E port)

{
    if(port >= TY_UART_NUM) {
       return OPRT_INVALID_PARM;
    }

	// uart deinit
	//UART_DisableTxReadyIRQ(ty_uart[port].uart);
	HAL_UART_DisableRxCallback(port);
	HAL_UART_DeInit(port);

    if(ty_uart[port].buf != NULL) {
        Free(ty_uart[port].buf);
        ty_uart[port].buf = NULL;
    }
    ty_uart[port].buf_len = 0;

   return OPRT_OK;
}

/***********************************************************
*  Function: ty_uart_send_data
*  Input: port data len
*  Output: none
*  Return: none
***********************************************************/
VOID ty_uart_send_data(IN CONST TY_UART_PORT_E port,IN CONST BYTE_T *data,IN CONST UINT_T len)
{
    if(port >= TY_UART_NUM) {
        return;
    }

    UINT_T i = 0;
    #if 0
    for(i = 0;i < len;i++) {
       while(!HAL_UART_IsTxReady(ty_uart[port].uart));
       HAL_UART_PutTxData(ty_uart[port].uart, *(data+i));
    }
    #else
    HAL_UART_Transmit_IT(port, (uint8_t *)data , len);
    #endif
}


/***********************************************************
*  Function: ty_uart_send_data
*  Input: len->data buf len
*  Output: buf->read data buf
*  Return: actual read data size
***********************************************************/
UINT_T ty_uart_read_data(IN CONST TY_UART_PORT_E port,OUT BYTE_T *buf,IN CONST UINT_T len)
{
     if(NULL == buf || 0 == len) {
        return 0;
    }

    UINT_T actual_size = 0;
    UINT_T cur_num = __ty_uart_read_data_size(port);
    if(cur_num > ty_uart[port].buf_len - 1) {
        PR_NOTICE("uart fifo is full! buf_zize:%d  len:%d",cur_num,len);
    }
    
    if(len > cur_num) {
        actual_size = cur_num;
    }else {
        actual_size = len;
    }
    //PR_NOTICE("uart_num = %d", cur_num);
    UINT_T i = 0;
    for(i = 0;i < actual_size;i++) {
        *buf++ = ty_uart[port].buf[ty_uart[port].out++];
        if(ty_uart[port].out >= ty_uart[port].buf_len) {
            ty_uart[port].out = 0;
        }
    }

    return actual_size;
}
#else
/***********************************************************
*  File: tuya_uart.c
*  Author: nzy
*  Date: 20171106
***********************************************************/
#define __TUYA_UART_GLOBALS
#include "tuya_uart.h"
#include "kernel/os/os.h"
#include "driver/chip/hal_uart.h"
#include <stdio.h>
//#include "objects.h"
//#include "serial_api.h"
#include "uni_log.h"
#include "tuya_cloud_types.h"

/***********************************************************
*************************micro define***********************
***********************************************************/
#if 0
#define UART_TX PA_23 //UART0 TX
#define UART_RX PA_18 //UART0 RX

typedef struct {
    serial_t sobj;
    UINT_T buf_len;
    BYTE_T *buf;
    USHORT_T in;
    USHORT_T out;
}TUYA_UART_S;
#endif
/***********************************************************
*************************variable define********************
***********************************************************/
//STATIC TUYA_UART_S ty_uart[TY_UART_NUM];

/***********************************************************
*************************function define********************
***********************************************************/
//STATIC VOID __uart_irq(UINT_T id, SerialIrq event);

/***********************************************************
*  Function: ty_uart_init
*  Input: port badu bits parity stop
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET ty_uart_init(IN CONST TY_UART_PORT_E port,IN CONST TY_UART_BAUD_E badu,\
                               IN CONST TY_DATA_BIT_E bits,IN CONST TY_PARITY_E parity,\
                               IN CONST TY_STOPBITS_E stop,IN CONST UINT_T bufsz)
{
#if 0
    if((port >= TY_UART_NUM) || (bufsz == 0)) {
        return OPRT_INVALID_PARM;
    }

    if(ty_uart[port].buf == NULL) {
        memset(&ty_uart[port], 0, sizeof(TUYA_UART_S));
        ty_uart[port].buf = Malloc(bufsz);
        if(ty_uart[port].buf == NULL) {
            return OPRT_MALLOC_FAILED;
        }
        ty_uart[port].buf_len = bufsz;
        PR_DEBUG("uart buf size : %d",bufsz);
    }else {
        return OPRT_COM_ERROR;
    }

    serial_init(&ty_uart[port].sobj,UART_TX,UART_RX);
    serial_baud(&ty_uart[port].sobj,badu);

    INT_T data_bit = 0;
    if(bits == TYWL_5B) {
        data_bit = 5;
    }else if(bits == TYWL_6B) {
        data_bit = 6;
    }else if(bits == TYWL_7B) {
        data_bit = 7;
    }else {
        data_bit = 8;
    }
    serial_format(&ty_uart[port].sobj,data_bit,parity,stop);

    serial_irq_handler(&ty_uart[port].sobj, __uart_irq, (uint32_t)&ty_uart[port].sobj);
    serial_irq_set(&ty_uart[port].sobj, RxIrq, 1);
    ty_uart[port].in = 0;
    ty_uart[port].out = 0;
#endif
	HAL_Status status = HAL_ERROR;
	UART_InitParam param;

    if(port >= TY_UART_NUM) {
        return OPRT_INVALID_PARM;
    }
	param.baudRate = badu;
	param.dataBits = bits;
	param.stopBits = stop;
	param.parity = parity;
	param.isAutoHwFlowCtrl = 0;

	status = HAL_UART_Init(port, &param);
	if (status != HAL_OK) {
		PR_ERR("uart init error %d", status);
		return OPRT_COM_ERROR;
	}

    return OPRT_OK;

}

/***********************************************************
*  Function: ty_uart_free
*  Input:free uart
*  Output: none
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET ty_uart_free(IN CONST TY_UART_PORT_E port)

{
#if 0
    if(port >= TY_UART_NUM) {
       return OPRT_INVALID_PARM;
    }
    //serial_free(&ty_uart[port].sobj);
    if(ty_uart[port].buf != NULL) {
        Free(ty_uart[port].buf);
        ty_uart[port].buf = NULL;
    }
    ty_uart[port].buf_len = 0;

   return OPRT_OK;
#endif
	HAL_Status status = HAL_ERROR;
	status = HAL_UART_DisableRxCallback(port);
	if (status != HAL_OK) {
		PR_DEBUG("uart RX disenable callback error %d", status);
		return OPRT_COM_ERROR;
	}

	status = HAL_UART_DeInit(port);
	if (status != HAL_OK) {
		PR_ERR("uart deinit error %d", status);
		return OPRT_COM_ERROR;
	}

	return OPRT_OK;
}

/***********************************************************
*  Function: ty_uart_send_data
*  Input: port data len
*  Output: none
*  Return: none
***********************************************************/
VOID ty_uart_send_data(IN CONST TY_UART_PORT_E port,IN CONST BYTE_T *data,IN CONST UINT_T len)
{
    if(port >= TY_UART_NUM) {
        return;
    }

	HAL_UART_Transmit_IT(port, (uint8_t *)data , len);
#if 0
    UINT_T i = 0;
    for(i = 0;i < len;i++) {
       serial_putc(&ty_uart[port].sobj, *(data+i));

    }
#endif
}

#if 0
STATIC UINT_T __ty_uart_read_data_size(IN CONST TY_UART_PORT_E port)
{
    UINT_T remain_buf_size = 0;

    if(ty_uart[port].in >= ty_uart[port].out) {
        remain_buf_size = ty_uart[port].in-ty_uart[port].out;
    }else {
        remain_buf_size = ty_uart[port].in + ty_uart[port].buf_len - ty_uart[port].out;
    }

    return remain_buf_size;
}


STATIC VOID __uart_irq(UINT_T id, SerialIrq event)
{
    serial_t *sobj = (void*)id;
    INT_T rc = 0;

    INT_T i = 0;
    for(i = 0;i < TY_UART_NUM;i++) {
        if(&ty_uart[i].sobj == sobj) {
            break;
        }
    }

    if(event == RxIrq) {
        rc = serial_getc(sobj);
        //PR_NOTICE("rc = %d", rc);
        if(__ty_uart_read_data_size(i) < ty_uart[i].buf_len - 1) {
            ty_uart[i].buf[ty_uart[i].in++] = rc;
            if(ty_uart[i].in >= ty_uart[i].buf_len){
                ty_uart[i].in = 0;
            }
        }else {
            PR_ERR("uart fifo is overflow! buf_zize:%d  data_get:%d",ty_uart[i].buf_len,__ty_uart_read_data_size(i));
        }
    }
}
#endif

/***********************************************************
*  Function: ty_uart_send_data
*  Input: len->data buf len
*  Output: buf->read data buf
*  Return: actual read data size
***********************************************************/
#define UART_RECV_TIMEOUT_MS 10
UINT_T ty_uart_read_data(IN CONST TY_UART_PORT_E port,OUT BYTE_T *buf,IN CONST UINT_T len)
{
#if 0
    if(NULL == buf || 0 == len) {
        return 0;
    }

    UINT_T actual_size = 0;
    UINT_T cur_num = __ty_uart_read_data_size(port);

    if(len > cur_num) {
        actual_size = cur_num;
    }else {
        actual_size = len;
    }
    //PR_NOTICE("uart_num = %d", cur_num);
    UINT_T i = 0;
    for(i = 0;i < actual_size;i++) {
        *buf++ = ty_uart[port].buf[ty_uart[port].out++];
        if(ty_uart[port].out >= ty_uart[port].buf_len) {
            ty_uart[port].out = 0;
        }
    }

    return actual_size;
#endif
	UINT_T rx_len;

    if(NULL == buf || 0 == len) {
        return 0;
    }

	rx_len = HAL_UART_Receive_IT(port, buf, len, UART_RECV_TIMEOUT_MS);
	//if (rx_len != len) {
		//PR_DEBUG("please check the uart recv wait timeout value\n");
	//}
	return rx_len;
}





#endif


