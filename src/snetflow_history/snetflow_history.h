#ifndef _SNETFLOW_HISTORY_H_
#define _SNETFLOW_HISTORY_H_

#include <time.h>
#include <vector>
#include <iostream>

typedef struct _history{
	char flow[128];
	int bytes;
}history_s;

int get_history(MYSQL *mysql, time_t start_time, time_t end_time, mysql_conf_s *cfg, void* history_vec);

#endif
