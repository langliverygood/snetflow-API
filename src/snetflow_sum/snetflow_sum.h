#ifndef _SNETFLOW_SUM_H_
#define _SNETFLOW_SUM_H_

#ifdef __cplusplus
extern "C"
{
#endif

int get_sum(MYSQL *mysql, time_t start_time, time_t end_time, mysql_conf_s *cfg, uint64_t* sum);

#ifdef __cplusplus
}
#endif

#endif
