#ifndef _SNETFLOW_TREND_H_
#define _SNETFLOW_TREND_H_

#include <time.h>
#include <map>
#include <iostream>

#define POINT_NUM 10

int get_trend(MYSQL *mysql, time_t start_time, time_t end_time, mysql_conf_s *cfg, void* trend_map);

#endif
