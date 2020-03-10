#ifndef _SNETFLOW_ASSOCIATE_H_
#define _SNETFLOW_ASSOCIATE_H_

#include <time.h>
#include <map>
#include <iostream>

int get_associate(MYSQL *mysql, time_t start_time, time_t end_time, mysql_conf_s *cfg, void* associate_map, uint32_t ip);

#endif
