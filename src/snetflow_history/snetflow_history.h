#ifndef _SNETFLOW_HISTORY_H_
#define _SNETFLOW_HISTORY_H_

#include <time.h>
#include <vector>
#include <iostream>

#define POINT_NUM 10

#define HISTORY_SRC_HXQ     1
#define HISTORY_DST_HXQ     2
#define HISTORY_SRC_HXQ_ZB  3
#define HISTORY_DST_HXQ_ZB  4
#define HISTORY_SRC_HXQ_SF  5
#define HISTORY_DST_HXQ_SF  6
#define HISTORY_SRC_GLQ     7
#define HISTORY_DST_GLQ     8
#define HISTORY_SRC_RZQ     9
#define HISTORY_DST_RZQ     10
#define HISTORY_SRC_JRQ     11
#define HISTORY_DST_JRQ     12
#define HISTORY_SRC_CSQ     13
#define HISTORY_DST_CSQ     14
#define HISTORY_SRC_DMZ     15
#define HISTORY_DST_DMZ     16
#define HISTORY_SRC_ALQ     17
#define HISTORY_DST_ALQ     18
#define HISTORY_SRC_WBQ     19
#define HISTORY_DST_WBQ     20
#define HISTORY_SRC_UNKNOWN 21
#define HISTORY_DST_UNKNOWN 22

typedef struct _history{
	char flow[128];
	int bytes;
}history_s;

int get_history(MYSQL *mysql, time_t start_time, time_t end_time, int kind, void* history_vec);

#endif
