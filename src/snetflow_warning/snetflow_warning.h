#ifndef _SNETFLOW_WARNING_H_
#define _SNETFLOW_WARNING_H_

#include "config.h"
#include <map>
#include <stdint.h>
#include <time.h>

using namespace std;

typedef struct _warning_msg_s {
    char ip[16];
    uint64_t bytes;
    char biz[128];
    char set[128];
    char module[128];
    char region[128];
    char switches[128];
} warning_msg_s;

int get_warning(MYSQL* mysql, time_t start_time, time_t end_time, mysql_conf_s* cfg, map<uint32_t, warning_msg_s>* warning_map);

#endif