#ifndef _SNETFLOW_TOP_H_
#define _SNETFLOW_TOP_H_

#include <time.h>
#include <map>
#include <iostream>

using namespace std;

int get_top(MYSQL *mysql, time_t start_time, time_t end_time, mysql_conf_s *cfg, map<string, uint64_t>* top_map);

#endif
