#ifndef _SNETFLOW_ASSOCIATE_H_
#define _SNETFLOW_ASSOCIATE_H_

#include <map>
#include <stdint.h>
#include <string>
#include <time.h>

using namespace std;

int get_associate(MYSQL* mysql, time_t start_time, time_t end_time, mysql_conf_s* cfg, map<string, uint64_t>* associate_map, uint32_t ip);

#endif