/***********************************************************
*  File: aes_inf.h 
*  Author: nzy
*  Date: 20161027

封装好的aes加解密头文件
***********************************************************/
#ifndef _AES_INF_H
#define _AES_INF_H

#include "adapter_platform.h"
#include "tuya_cloud_error_code.h"

#ifdef __cplusplus
	extern "C" {
#endif

#ifdef  __AES_INF_GLOBALS
    #define __AES_INF_EXT
#else
    #define __AES_INF_EXT extern
#endif

/***********************************************************
*************************micro define***********************
***********************************************************/
typedef VOID (*AES128_ECB_ENC)(IN BYTE_T *input,IN CONST BYTE_T *key,OUT BYTE_T *output);
typedef VOID (*AES128_ECB_DEC)(IN BYTE_T *input,IN CONST BYTE_T *key,OUT BYTE_T *output);
typedef VOID (*AES128_CBC_ENC_BUF)(OUT BYTE_T *output,IN BYTE_T *input,IN UINT_T length,IN CONST BYTE_T *key,IN CONST BYTE_T *iv);
typedef VOID (*AES128_CBC_DEC_BUF)(OUT BYTE_T *output,IN BYTE_T *input,IN UINT_T length,IN CONST BYTE_T *key,IN CONST BYTE_T *iv);

typedef struct {
    AES128_ECB_ENC ecb_enc;
    AES128_ECB_DEC ecb_dec;
    AES128_CBC_ENC_BUF cbc_enc;
    AES128_CBC_DEC_BUF cbc_dec;
}AES_METHOD_REG_S;

/***********************************************************
*************************variable define********************
***********************************************************/

/***********************************************************
*************************function define********************
***********************************************************/
/***********************************************************
*  Function: ase_method_register
*  Input: aes->AES_METHOD_REG_S
*  Output: none
*  Return: OPERATE_RET
*  Note:if no register aes method then use inner software aes default
***********************************************************/
__AES_INF_EXT \
OPERATE_RET aes_method_register(IN CONST AES_METHOD_REG_S *aes);

/***********************************************************
*  Function: aes_method_unregister
*  Input: none
*  Output: none
*  Return: none
***********************************************************/
__AES_INF_EXT \
VOID aes_method_unregister(VOID);

/***********************************************************
*  Function: aes128_ecb_encode
*  Input: data
*         len
*         key
*  Output: ec_data
*          ec_len
*  Return: OPERATE_RET
***********************************************************/
__AES_INF_EXT \
OPERATE_RET aes128_ecb_encode(IN CONST BYTE_T *data,IN CONST UINT_T len,\
                              OUT BYTE_T **ec_data,OUT UINT_T *ec_len,\
                              IN CONST BYTE_T *key);

/***********************************************************
*  Function: aes128_ecb_decode
*  Input: data
*         len
*         key
*  Output: dec_data->decode data buf
*          dec_len->decode data len is not the original data len
*  Return: OPERATE_RET
***********************************************************/
__AES_INF_EXT \
OPERATE_RET aes128_ecb_decode(IN CONST BYTE_T *data,IN CONST UINT_T len,\
                              OUT BYTE_T **dec_data,OUT UINT_T *dec_len,\
                              IN CONST BYTE_T *key);

/***********************************************************
*  Function: aes128_cbc_encode
*  Input: data
*         len
*         key
*         iv
*  Output: ec_data
*          ec_len
*  Return: OPERATE_RET
***********************************************************/
__AES_INF_EXT \
OPERATE_RET aes128_cbc_encode(IN CONST BYTE_T *data,IN CONST UINT_T len,\
                              IN CONST BYTE_T *key,IN CONST BYTE_T *iv,\
                              OUT BYTE_T **ec_data,OUT UINT_T *ec_len);

/***********************************************************
*  Function: aes128_cbc_decode
*  Input: data
*         len
*         key
*         iv
*  Output: dec_data->decode data buf
*          dec_len->decode data len is not the original data len
*  Return: OPERATE_RET
***********************************************************/
__AES_INF_EXT \
OPERATE_RET aes128_cbc_decode(IN CONST BYTE_T *data,IN CONST UINT_T len,\
                              IN CONST BYTE_T *key,IN CONST BYTE_T *iv,\
                              OUT BYTE_T **dec_data,OUT UINT_T *dec_len);

/***********************************************************
*  Function: aes128_free_data
*  Input: data
*  Return: OPERATE_RET
***********************************************************/
__AES_INF_EXT \
OPERATE_RET aes128_free_data(IN BYTE_T *data);


OPERATE_RET aes128_ecb_encode_raw(IN CONST BYTE_T *data, IN CONST UINT_T len,\
                                  OUT BYTE_T *ec_data,IN CONST BYTE_T *key);


OPERATE_RET aes128_ecb_decode_raw(IN CONST BYTE_T *data, IN CONST UINT_T len,\
                                  OUT BYTE_T *dec_data,IN CONST BYTE_T *key);



#ifdef __cplusplus
}
#endif
#endif

