#ifndef _COMMON_H_
#define _COMMON_H_

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEBUG 0
/* 数据库字段定义 */
#define TABLE_NAME "record"
#define MYSQL_TIMESTAMP "timestamp"
#define MYSQL_FILED_TIMESTAMP 0
#define MYSQL_FILED_ID        1
#define MYSQL_FILED_SOURCEID  2
#define MYSQL_FILED_EXPORTER  3
#define MYSQL_FILED_SRCIP     4
#define MYSQL_FILED_SRCSET    5
#define MYSQL_FILED_SRCBIZ    6
#define MYSQL_FILED_DSTIP     7
#define MYSQL_FILED_DSTSET    8
#define MYSQL_FILED_DSTBIZ    9
#define MYSQL_FILED_NEXTHOP   10
#define MYSQL_FILED_PROT      11
#define MYSQL_FILED_SRCPORT   12
#define MYSQL_FILED_DSTPORT   13
#define MYSQL_FILED_BYTES     14
#define MYSQL_FILED_TCPFLAGS  15
#define MYSQL_FILED_VERSION    16

/* 定义ip头部常见的协议字段 */
#define ICMP 1
#define IGMP 2 
#define TCP  6
#define UDP  17
#define IGRP 88
#define OSPF 89

/* 枚举http请求方法 */
typedef enum _http_request_method {
	GET = 1,
	POST,
	PUT,
	HEAD,
	DELETE,
	CONNETCT,
	TRACE,
	OPTIONS
}http_request_method_e;

/* 具体任务的结构体 */
typedef struct _snetflow_job {
	http_request_method_e http_request_method;
	int job_id;
	time_t start_time;
	time_t end_time;
	int kind;
	void *data;
}snetflow_job_s;

/***************************************************************/
/* 函  数：myprintf *********************************************/
/* 说  明：根据DEBUG的值控制输出 ********************************/
/***************************************************************/
int myprintf(const char* format, ...);

/***************************************************************/
/* 函  数：str_to_uint ******************************************/
/* 说  明：字符串转uint *****************************************/
/* 参  数：str 字符串 *******************************************/
/*        转换结果保存到n **************************************/
/* 返回值：0 成功 **********************************************/
/*        1 失败 ***********************************************/
/***************************************************************/
char str_to_long(const char *str, long int *n);
/***************************************************************/
/* 函  数：timestamp_to_str *************************************/
/* 说  明：时间戳转字符串 ***************************************/
/* 参  数：timestamp 时间戳 *************************************/
/*        time_str 转换结果 ************************************/
/*        str_len  结果的最大长度 ******************************/
/* 返回值：无 **************************************************/
/***************************************************************/
void timestamp_to_str(time_t timestamp, char *time_str, int str_len);
/***************************************************************/
/* 函  数：timestr_to_stamp *************************************/
/* 说  明：字符串转时间戳 ***************************************/
/* 参  数：str_time 时间字符串 **********************************/
/* 返回值：时间戳 **********************************************/
/***************************************************************/
time_t timestr_to_stamp(char *str_time);
/***************************************************************/
/* 函  数：ipprotocal_int_to_str ********************************/
/* 说  明：ip头部协议字段转字符串             *******************************/
/***************************************************************/
void ipprotocal_int_to_str(int prot, char *out, int out_len);

#ifdef __cplusplus
}
#endif

#endif