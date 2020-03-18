#ifndef _SNETFLOW_HISTORY_H_
#define _SNETFLOW_HISTORY_H_

#include <time.h>
#include <vector>

using namespace std;

typedef struct _history {
    char flow[128];
    int bytes;
} history_s;

int get_history(MYSQL* mysql, time_t start_time, time_t end_time, mysql_conf_s* cfg, vector<history_s>* history_vec);

#endif