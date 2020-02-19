#ifndef _SNETFLOW_TOP_H_
#define _SNETFLOW_TOP_H_

#include <time.h>
#include <map>
#include <iostream>

#define RESPONSE_BODY_TOP_LEN 102400
#define TOP_FLOW  1
#define TOP_SRC_SET 2
#define TOP_DST_SET 3
#define TOP_SRC_BIZ 4
#define TOP_DST_BIZ 5

void *get_top(MYSQL *mysql, time_t start_time, time_t end_time, int kind);

#endif
