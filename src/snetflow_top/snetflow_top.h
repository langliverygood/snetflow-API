#ifndef _SNETFLOW_TOP_H_
#define _SNETFLOW_TOP_H_

#include <time.h>
#include <map>
#include <iostream>

#define TOP_SRC_SET     1
#define TOP_DST_SET     2
#define TOP_SRC_BIZ     3
#define TOP_DST_BIZ     4
#define TOP_SRC_HXQ     5
#define TOP_DST_HXQ     6
#define TOP_SRC_HXQ_ZB  7
#define TOP_DST_HXQ_ZB  8
#define TOP_SRC_HXQ_SF  9
#define TOP_DST_HXQ_SF  10
#define TOP_SRC_GLQ     11
#define TOP_DST_GLQ     12
#define TOP_SRC_RZQ     13
#define TOP_DST_RZQ     14
#define TOP_SRC_JRQ     15
#define TOP_DST_JRQ     16
#define TOP_SRC_CSQ     17
#define TOP_DST_CSQ     18
#define TOP_SRC_DMZ     19
#define TOP_DST_DMZ     20
#define TOP_SRC_ALQ     21
#define TOP_DST_ALQ     22
#define TOP_SRC_WBQ     23
#define TOP_DST_WBQ     24
#define TOP_SRC_UNKNOWN 25
#define TOP_DST_UNKNOWN 26

int get_top(MYSQL *mysql, time_t start_time, time_t end_time, int kind, void* top_map);

#endif
