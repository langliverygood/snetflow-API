#ifndef _SNETFLOW_TREND_H_
#define _SNETFLOW_TREND_H_

#include <time.h>
#include <map>
#include <iostream>

#define POINT_NUM 10

#define TREND_SRC_HXQ     1
#define TREND_DST_HXQ     2
#define TREND_SRC_HXQ_ZB  3
#define TREND_DST_HXQ_ZB  4
#define TREND_SRC_HXQ_SF  5
#define TREND_DST_HXQ_SF  6
#define TREND_SRC_GLQ     7
#define TREND_DST_GLQ     8
#define TREND_SRC_RZQ     9
#define TREND_DST_RZQ     10
#define TREND_SRC_JRQ     11
#define TREND_DST_JRQ     12
#define TREND_SRC_CSQ     13
#define TREND_DST_CSQ     14
#define TREND_SRC_DMZ     15
#define TREND_DST_DMZ     16
#define TREND_SRC_ALQ     17
#define TREND_DST_ALQ     18
#define TREND_SRC_WBQ     19
#define TREND_DST_WBQ     20
#define TREND_SRC_UNKNOWN 21
#define TREND_DST_UNKNOWN 22

int get_trend(MYSQL *mysql, time_t start_time, time_t end_time, int kind, void* trend_map);

#endif
