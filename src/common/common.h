#ifndef _COMMON_H_
#define _COMMON_H_

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 网络区域 */
#define NET_HXQ        "HXQ"
#define NET_HXQ_CN     "核心区"
#define NET_HXQ_ZB     "HXQ_ZB"
#define NET_HXQ_ZB_CN  "总部核心区"
#define NET_HXQ_SF     "HXQ_SF"
#define NET_HXQ_SF_CN  "省分核心区"
#define NET_GLQ        "GLQ"
#define NET_GLQ_CN     "管理区"
#define NET_RZQ        "RZQ"
#define NET_RZQ_CN     "容灾区"
#define NET_JRQ        "JRQ"
#define NET_JRQ_CN     "接入区"
#define NET_CSQ        "CSQ"
#define NET_CSQ_CN     "测试区"
#define NET_DMZ        "DMZ"
#define NET_DMZ_CN     "DMZ区"
#define NET_ALQ        "ALQ"
#define NET_ALQ_CN     "阿里区"
#define NET_WBQ        "WBQ"
#define NET_WBQ_CN     "外部区"
#define NET_UNKNOWN    "Unknown"

/* 数据库字段定义 */
#define TABLE_NAME      "record"
#define MYSQL_TIMESTAMP "timestamp"
#define MYSQL_ID        "id"
#define MYSQL_SOURCEID  "source_id"
#define MYSQL_EXPORTER  "exporter_address"
#define MYSQL_SRCIP     "ipv4_src_addr"
#define MYSQL_SRCSET    "src_set"
#define MYSQL_SRCBIZ    "src_biz"
#define MYSQL_SRCREGION "src_region"
#define MYSQL_SRCSW     "src_sw"
#define MYSQL_DSTIP     "ipv4_dst_addr"
#define MYSQL_DSTSET    "dst_set"
#define MYSQL_DSTBIZ    "dst_biz"
#define MYSQL_DSTREGION "dst_region"
#define MYSQL_DSTSW     "dst_sw"
#define MYSQL_NEXTHOP   "ipv4_next_hop"
#define MYSQL_PROT      "prot"
#define MYSQL_SRCPORT   "l4_src_port"
#define MYSQL_DSTPORT   "l4_dst_port"
#define MYSQL_BYTES     "bytes"
#define MYSQL_TCPFLAGS  "tcp_flags"
#define MYSQL_VERSION   "version"
#define MYSQL_FILED_TIMESTAMP 0
#define MYSQL_FILED_ID        1
#define MYSQL_FILED_SOURCEID  2
#define MYSQL_FILED_EXPORTER  3
#define MYSQL_FILED_SRCIP     4
#define MYSQL_FILED_SRCSET    5
#define MYSQL_FILED_SRCBIZ    6
#define MYSQL_FILED_SRCREGION 7
#define MYSQL_FILED_SRCSW     8
#define MYSQL_FILED_DSTIP     9
#define MYSQL_FILED_DSTSET    10
#define MYSQL_FILED_DSTBIZ    11
#define MYSQL_FILED_DSTREGION 12
#define MYSQL_FILED_DSTSW     13
#define MYSQL_FILED_NEXTHOP   14
#define MYSQL_FILED_PROT      15
#define MYSQL_FILED_SRCPORT   16
#define MYSQL_FILED_DSTPORT   17
#define MYSQL_FILED_BYTES     18
#define MYSQL_FILED_TCPFLAGS  19
#define MYSQL_FILED_VERSION   20

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
/* 函  数：set_debug ********************************************/
/* 说  明：设置DEBUG的值 ****************************************/
/***************************************************************/
void set_debug(int d);

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
/***************************************************************/
/* 函  数：get_wday_by_timestamp ********************************/
/* 说  明：根据时间戳获取星期  (周日是0，...，周六是6) ************/
/***************************************************************/
int get_wday_by_timestamp(time_t timep);

/***************************************************************/
/* 函  数：wday_int_to_str **************************************/
/* 说  明：星期整型转字符串 *************************************/
/***************************************************************/
void wday_int_to_str(const int w, char *wd, const int len);

#ifdef __cplusplus
}
#endif

#endif
