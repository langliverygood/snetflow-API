#ifndef _SNETFLOW_TOP_H_
#define _SNETFLOW_TOP_H_

#include <time.h>

#define RESPONSE_BODY_TOP_LEN 102400

char *get_top(MYSQL *mysql, time_t start_time, time_t end_time);

#endif
