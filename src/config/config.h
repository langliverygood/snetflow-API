#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>

#define TOP       0
#define TREND     1
#define HISTORY   2
#define WARNING   3
#define SUM       4
#define ASSOCIATE 5

typedef struct _mysql_conf{
	char name[32];
	char table[32];
	char column[128];
	char condition[256];
}mysql_conf_s;

void read_cfg_file(const char *file);
mysql_conf_s *get_config(const char *key, int kind);
char *cfg_get_timestamp();
uint32_t cfg_get_thread_num();
uint32_t cfg_get_response_timeout();
uint32_t cfg_get_listened_num();
uint32_t cfg_get_request_body_size();
uint32_t cfg_get_history_num();
uint32_t cfg_get_trend_point();
uint32_t cfg_get_byte_to_bit();

#endif
