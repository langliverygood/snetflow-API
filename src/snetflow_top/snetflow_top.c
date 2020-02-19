#include <stdio.h>
#include <stdint.h>
#include <string.h>  
#include <arpa/inet.h>
#include <mysql/mysql.h>
#include <cjson/cJSON.h>

#include "common.h"
#include "snetflow_top.h"

using namespace std;

static map<string, uint64_t> flow_top;
static map<string, uint64_t> src_set_top;
static map<string, uint64_t> dst_set_top;
static map<string, uint64_t> src_biz_top;
static map<string, uint64_t> dst_biz_top;
static char *top_response_body;

static void top_init()
{
	top_response_body = NULL;
	flow_top.clear();
	src_set_top.clear();
	dst_set_top.clear();
	src_biz_top.clear();
	dst_biz_top.clear();

	return;
}

static void top_insert(map<string, uint64_t> &my_map, const char *key, uint64_t bytes)
{
	string k;

	k = key;
	my_map[k] += bytes;

	return;
}

#if 0
static void top_traverse(map<string, uint64_t> my_map)
{
	map<string, uint64_t>::iterator it;
	
	for(it = my_map.begin(); it != my_map.end(); it++)  
	{  
	    cout << it->first << ": " << it->second << endl;
	}

	return;
}
#endif

/* 从数据查询结果，并写入相应的map 中 */
static int top_query(MYSQL *mysql, const char *query, int kind)
{
	int flag;
	char colloct[32], s_ip[64], d_ip[64], flow[512], prot_str[16];
	long int ip1, ip2, ip3, bytes, prot;
	time_t times, timee;
	struct in_addr ip_addr1, ip_addr2, ip_addr3;
	MYSQL_RES *res;
	MYSQL_ROW row;
	
	flag = mysql_real_query(mysql, query, (unsigned int)strlen(query));
	if(flag)
	{
		myprintf("[Query Failed!]:%s\n", query);
		return -1;
	}
	
	time(&times);
	res = mysql_use_result(mysql); 
	/*mysql_fetch_row检索结果集的下一行*/
	while((row = mysql_fetch_row(res)))
	{
		/* bytes字段转化为long int */
		if(str_to_long(row[0], &bytes))
		{
			continue;
		}
		/* 记录流 */
		if(kind == TOP_FLOW)
		{
			if((str_to_long(row[1], &ip1) == 0) && (str_to_long(row[3], &ip2) == 0) && (str_to_long(row[5], &ip3) == 0) && (str_to_long(row[8], &prot) == 0))
			{
				memset(flow, 0, sizeof(flow));
				ip_addr1.s_addr = htonl((uint32_t)ip1);
				ip_addr2.s_addr = htonl((uint32_t)ip2);
				ip_addr3.s_addr = htonl((uint32_t)ip3);
				ipprotocal_int_to_str((int)prot, prot_str, sizeof(prot_str));
				sprintf(colloct, "%s", inet_ntoa(ip_addr1));
				sprintf(s_ip, "%s", inet_ntoa(ip_addr2));
				sprintf(d_ip, "%s", inet_ntoa(ip_addr3));
				sprintf(flow, "[%s %s]%s(%s)-->%s:%s(%s) %s", colloct, row[2], s_ip, row[4], d_ip, row[6], row[7], prot_str);	
				top_insert(flow_top, flow, bytes);
			}
		}
		/* 记录源集群的流量总和 */
		else if(kind == TOP_SRC_SET)
		{
			top_insert(src_set_top, row[1], bytes);
		}
		/* 记录目的集群的流量总和 */
		else if(kind == TOP_DST_SET)
		{
			top_insert(dst_set_top, row[1], bytes);
		}
		/* 记录源业务的流量总和 */
			else if(kind == TOP_SRC_BIZ)
		{
		top_insert(src_biz_top, row[1], bytes);
		}
		/* 记录目的业务的流量总和 */
		else if(kind == TOP_DST_BIZ)
		{
			top_insert(dst_biz_top, row[1], bytes);
		}
	}
	mysql_free_result(res);
	time(&timee);
	myprintf("[Cost %ld seconds]:%s\n", timee - times, query);
	

	return 0;
}

void *get_top(MYSQL *mysql, time_t start_time, time_t end_time, int kind)
{
	int i, s_week, e_week, interval;
	char query[1024], s_time[128], e_time[128], week_str[4], column[128];
	time_t time_now;
	map<string, uint64_t> *ret;
	
    /* 每次查询都要将上次的结果清空 */
	top_init();
	/* 保证截止时间不超过当前, 时间跨度不超过7天 */
	time(&time_now);
	if(end_time > time_now)
	{
		end_time = time_now;
	}
	if(end_time - start_time >= 60 * 60 * 24 * 7)
	{
		start_time = end_time - 60 * 60 * 24 * 7;
	}
	/* 时间戳转字符串 */
	timestamp_to_str(start_time, s_time, sizeof(s_time));
	timestamp_to_str(end_time, e_time, sizeof(e_time));
	/* 获得时间戳对应的星期 */
	s_week = get_wday_by_timestamp(start_time);
	e_week = get_wday_by_timestamp(end_time);
	/* 时间跨度 */
	interval = e_week - s_week;
	if(interval < 0)
	{
		interval += 7;
	}
	/* 确定要查询的列 */
	if(kind == TOP_FLOW)
	{
		sprintf(column, "%s,%s,%s,%s,%s,%s,%s,%s,%s", MYSQL_BYTES, MYSQL_EXPORTER, MYSQL_SOURCEID, MYSQL_SRCIP, MYSQL_SRCBIZ, MYSQL_DSTIP, MYSQL_DSTPORT, MYSQL_DSTBIZ, MYSQL_PROT);
        ret = &flow_top;
	}
	else if(kind == TOP_SRC_SET)
	{
		sprintf(column, "%s,%s", MYSQL_BYTES, MYSQL_SRCSET);
		ret = &src_set_top;
	}
	else if(kind == TOP_DST_SET)
	{
		sprintf(column, "%s,%s", MYSQL_BYTES, MYSQL_DSTSET);
		ret = &dst_set_top;
	}
	else if(kind == TOP_SRC_BIZ)
	{
		sprintf(column, "%s,%s", MYSQL_BYTES, MYSQL_SRCBIZ);
		ret = &src_biz_top;
	}
	else if(kind == TOP_DST_BIZ)
	{
		sprintf(column, "%s,%s", MYSQL_BYTES, MYSQL_DSTBIZ);
		ret = &dst_biz_top;
	}
	else
	{
		return NULL;
	}
	/* 当interval=0的时候，有两种情况：1、时间跨度只包含一天。2、时间跨度将近但未到达七天，如：上个周六晚8点到这个周六早10点。*/
	if(interval == 0)
	{
		wday_int_to_str(s_week, week_str, sizeof(week_str));
		sprintf(query, "select %s from %s_%s where %s >= '%s' and %s <= '%s'", column, TABLE_NAME, week_str, MYSQL_TIMESTAMP, s_time, MYSQL_TIMESTAMP, e_time);
		top_query(mysql, query, kind);
		/* 如果是第二种情况(时间跨度超过一天), 要查询其他6张表的全部 */
		if(end_time - start_time > 60 * 60 * 24)
		{
			for(i = (s_week + 1) % 7; i != s_week; i = (i + 1) % 7)
			{
				wday_int_to_str(i, week_str, sizeof(week_str));
				sprintf(query, "select %s from %s_%s", column, TABLE_NAME, week_str);
				top_query(mysql, query, kind);
			}
		}
	}
	else
	{
		for(i = 0; i <= interval; i++)
		{
			wday_int_to_str((s_week + i) % 7, week_str, sizeof(week_str));
			if(i == 0) /* 查询第一张表的部分 */
			{
				sprintf(query, "select %s from %s_%s where %s >= '%s'", column, TABLE_NAME, week_str, MYSQL_TIMESTAMP, s_time);
			}
			else if(i == interval) /* 查询最后一张表的部分 */
			{
				sprintf(query, "select %s from %s_%s where %s <= '%s'", column, TABLE_NAME, week_str, MYSQL_TIMESTAMP, e_time);
			}
			else /* 查询其他表的全部 */
			{
				sprintf(query, "select %s from %s_%s", column, TABLE_NAME, week_str);
			}
			top_query(mysql, query, kind);
		}
	}
	return (void *)ret;

}
