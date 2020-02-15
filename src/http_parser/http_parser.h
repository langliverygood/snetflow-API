#ifndef _CONN_BKING_H_
#define _CONN_BKING_H_

#include "http_parser_basic.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NON_NUM '0'
#define HTTP_URL_LEN 256
#define HTTP_BODY_LEN 2048

void parser_init();
int http_parse(const char *http_pkt, int http_pkt_len);
char *get_http_url_encode();
int get_http_url_encode_len();
char *get_http_url_decode();
int get_http_url_decode_len();
char *get_http_body();
int get_http_body_len();

#ifdef __cplusplus
}
#endif

#endif
