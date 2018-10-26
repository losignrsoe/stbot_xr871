#ifndef TUYA_SSL_H
#define TUYA_SSL_H

#include <string.h>
#include "uni_network.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/certs.h"

struct mbedtls_ssl{
	mbedtls_net_context server_fd;
	mbedtls_ssl_context ssl;
	mbedtls_ssl_config conf;
	mbedtls_x509_crt cacert;
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_entropy_context entropy;
};
typedef struct mbedtls_ssl mbedtls_ssl;

int tuya_ssl_connect(mbedtls_ssl **p_mbedtls_ssl_ctx, char *hostname);
int tuya_ssl_write(mbedtls_ssl *mbedtls_ssl_ctx, unsigned char *buf, size_t len);
int tuya_ssl_read(mbedtls_ssl *mbedtls_ssl_ctx, unsigned char *buf, size_t len);
int tuya_ssl_disconnect(mbedtls_ssl *mbedtls_ssl_ctx);

#endif//TUYA_SSL_H


