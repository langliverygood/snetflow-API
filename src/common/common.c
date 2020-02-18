#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>

#include "common.h"

#ifdef __cplusplus
extern "C"
{
#endif

static int debug;
static char *wday[] = {(char *)"Sun", (char *)"Mon", (char *)"Tue", (char *)"Wed", (char *)"Thu", (char *)"Fri", (char *)"Sat"};

/***************************************************************/
/* 函  数：set_debug ********************************************/
/* 说  明：设置DEBUG的值 ****************************************/
/***************************************************************/
void set_debug(int d)
{
	debug = d;

	return;
}

/***************************************************************/
/* 函  数：myprintf *********************************************/
/* 说  明：根据DEBUG的值控制输出 ********************************/
/***************************************************************/
int myprintf(const char* format, ...)
{
	int result;

	result = 0;
	if(debug)
	{
	    va_list vp;
	    va_start(vp, format);
	    result = vprintf(format, vp);
	    va_end(vp);
	}

	return result;
}

/***************************************************************/
/* 函  数：str_to_uint ******************************************/
/* 说  明：字符串转uint *****************************************/
/* 参  数：str 字符串 *******************************************/
/*        转换结果保存到n **************************************/
/* 返回值：0 成功***********************************************/
/*        1 失败************************************************/
/***************************************************************/
char str_to_long(const char *str, long int *n)
{
	long int val;
	char *endptr;
	
	errno = 0;
	val = strtol(str, &endptr, 0);
	if((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN))|| (errno != 0 && val == 0)) 
	{
		return 1;
	}
	if(endptr == str) 
	{
	   return 1;
	}
	*n = val;
	
	return 0;
}

/***************************************************************/
/* 函  数：timestamp_to_str *************************************/
/* 说  明：时间戳转字符串 ***************************************/
/* 参  数：timestamp 时间戳 *************************************/
/*        time_str 转换结果 ************************************/
/*        str_len  结果的最大长度 ******************************/
/* 返回值：无 **************************************************/
/***************************************************************/
void timestamp_to_str(time_t timestamp, char *time_str, int str_len)
{
	struct tm *tm_t;
	
	tm_t = localtime(&timestamp);
	strftime(time_str , str_len , "%Y-%m-%d %H:%M:%S", tm_t);

	return;
}

/***************************************************************/
/* 函  数：timestr_to_stamp *************************************/
/* 说  明：字符串转时间戳 ***************************************/
/* 参  数：str_time 时间字符串 **********************************/
/* 返回值：时间戳 **********************************************/
/***************************************************************/
time_t timestr_to_stamp(char *str_time)
{
	struct tm stm;
	time_t t;

	if(!strptime(str_time, "%Y-%m-%d %H:%M:%S", &stm))
	{
		return 0;
	}
	t = mktime(&stm);

	return t;
}

/***************************************************************/
/* 函  数：ipprotocal_int_to_str ********************************/
/* 说  明：ip头部协议字段转字符串             *******************************/
/***************************************************************/
void ipprotocal_int_to_str(int prot, char *out, int out_len)
{
	memset(out, 0, out_len);
	switch(prot)
	{
		case ICMP:
			strncpy(out, "ICMP", out_len);
			break;
		case IGMP:
			strncpy(out, "IGMP", out_len);
			break;
		case TCP:
			strncpy(out, "TCP", out_len);
			break;
		case UDP:
			strncpy(out, "UDP", out_len);
			break;
		case IGRP:
			strncpy(out, "IGRP", out_len);
			break;
		case OSPF:
			strncpy(out, "OSPF", out_len);
			break;
		default:
			strncpy(out, "Unknown", out_len);
			break;
	}

	return;
}

/***************************************************************/
/* 函  数：get_wday_by_timestamp ********************************/
/* 说  明：根据时间戳获取星期  (周日是0，...，周六是6) ************/
/***************************************************************/
int get_wday_by_timestamp(time_t timep)
{
    struct tm *p;

    p = localtime(&timep);

	return p->tm_wday;
}

/***************************************************************/
/* 函  数：wday_int_to_str **************************************/
/* 说  明：星期整型转字符串 *************************************/
/***************************************************************/
void wday_int_to_str(const int w, char *wd, const int len)
{
	memset(wd, 0, len);
	strcpy(wd, wday[w]);

	return;
}

#ifdef __cplusplus
}
#endif
