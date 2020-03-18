#ifndef _SNETFLOW_TREND_H_
#define _SNETFLOW_TREND_H_

#include <iostream>
#include <map>
#include <time.h>

using namespace std;

int get_trend(MYSQL* mysql, time_t start_time, time_t end_time, mysql_conf_s* cfg, map<uint64_t, uint64_t>* trend_map);

#endif