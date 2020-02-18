#include <stdio.h>
#include <stdint.h>
#include <string.h>  
#include <arpa/inet.h>
#include <mysql/mysql.h>
#include <cjson/cJSON.h>
#include <iostream>
#include <map>

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

/* 构建top自有的json串 */
static void build_response_body_json(int kind)
{
	map<string, uint64_t>::iterator it;
	char bytes[32];
	time_t times, timee;
	string s;
	cJSON *root, *flow, *src_set, *dst_set, *src_biz, *dst_biz, *tmp;

	root = cJSON_CreateObject();
	flow = cJSON_AddArrayToObject(root, "flow");
	src_set = cJSON_AddArrayToObject(root, "src_set");
	dst_set = cJSON_AddArrayToObject(root, "dst_set");
	src_biz = cJSON_AddArrayToObject(root, "src_biz");
	dst_biz = cJSON_AddArrayToObject(root, "dst_biz");
	time(&times);

    /* 添加流 */
	if(kind == TOP_FLOW)
	{
		myprintf("map size:%lu\n", flow_top.size());
		for(it = flow_top.begin(); it != flow_top.end(); it++) 
		{
			tmp = cJSON_CreateObject();
	        s = it->first;
	        cJSON_AddStringToObject(tmp, "flow", s.c_str());
			sprintf(bytes, "%lu", it->second);
	        cJSON_AddStringToObject(tmp, "bytes", bytes);
	        cJSON_AddItemToArray(flow, tmp);
		}
	}
	/* 添加源集群的流量 */
	if(kind == TOP_SRC_SET)
	{
		myprintf("map size:%lu\n", src_set_top.size());
		for(it = src_set_top.begin(); it != src_set_top.end(); it++) 
		{
			tmp = cJSON_CreateObject();
	        s = it->first;
	        cJSON_AddStringToObject(tmp, "src_set", s.c_str());
			sprintf(bytes, "%lu", it->second);
			cJSON_AddStringToObject(tmp, "bytes", bytes);
	        cJSON_AddItemToArray(src_set, tmp);
		}
	}
	/* 添加目的集群的流量 */
	if(kind == TOP_DST_SET)
	{
		myprintf("map size:%lu\n", dst_set_top.size());
		for(it = dst_set_top.begin(); it != dst_set_top.end(); it++) 
		{
			tmp = cJSON_CreateObject();
	        s = it->first;
	        cJSON_AddStringToObject(tmp, "dst_set", s.c_str());
	        sprintf(bytes, "%lu", it->second);
	        cJSON_AddStringToObject(tmp, "bytes", bytes);
	        cJSON_AddItemToArray(dst_set, tmp);
		}
	}
	/* 添加源业务的流量 */
	if(kind == TOP_SRC_BIZ)
	{
		myprintf("map size:%lu\n", src_biz_top.size());
		for(it = src_biz_top.begin(); it != src_biz_top.end(); it++) 
		{
			tmp = cJSON_CreateObject();
	        s = it->first;
	        cJSON_AddStringToObject(tmp, "src_biz", s.c_str());
	        sprintf(bytes, "%lu", it->second);
	        cJSON_AddStringToObject(tmp, "bytes", bytes);
	        cJSON_AddItemToArray(src_biz, tmp);
		}
	}
	/* 添加目的业务的流量 */
	else if(kind == TOP_DST_BIZ)
	{
		myprintf("map size:%lu\n", dst_biz_top.size());
		for(it = dst_biz_top.begin(); it != dst_biz_top.end(); it++) 
		{
			tmp = cJSON_CreateObject();
	        s = it->first;
	        cJSON_AddStringToObject(tmp, "dst_biz", s.c_str());
	        sprintf(bytes, "%lu", it->second);
	        cJSON_AddStringToObject(tmp, "bytes", bytes);
	        cJSON_AddItemToArray(dst_biz, tmp);
		}
	}
	time(&timee);
	myprintf("[Cost %ld seconds]:build top json!\n", timee - times);
	top_response_body = cJSON_Print(root);
	cJSON_Delete(root);
	
	return;
}

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
	res = mysql_use_result(mysql);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         	/*mysql_fetch_row检索结果集的下一行*/
	while((row = mysql_fetch_row(res)))
	{
		/* bytes字段转化为long int */
		if(str_to_long(row[MYSQL_FILED_BYTES], &bytes))
		{
			continue;
		}
		/* 记录流 */
		if(kind == TOP_FLOW)
		{
			if((str_to_long(row[MYSQL_FILED_EXPORTER], &ip1) == 0) && (str_to_long(row[MYSQL_FILED_SRCIP], &ip2) == 0) && (str_to_long(row[MYSQL_FILED_DSTIP], &ip3) == 0) && (str_to_long(row[MYSQL_FILED_PROT], &prot) == 0))
			{
				memset(flow, 0, sizeof(flow));
				ip_addr1.s_addr = htonl((uint32_t)ip1);
				ip_addr2.s_addr = htonl((uint32_t)ip2);
				ip_addr3.s_addr = htonl((uint32_t)ip3);
				sprintf(colloct, "[%s %s]", inet_ntoa(ip_addr1), row[MYSQL_FILED_SOURCEID]);
				sprintf(s_ip, "%s(%s)", inet_ntoa(ip_addr2), row[MYSQL_FILED_SRCBIZ]);
				sprintf(d_ip, "%s:%s(%s)", inet_ntoa(ip_addr3), row[MYSQL_FILED_DSTPORT], row[MYSQL_FILED_DSTBIZ]);
				ipprotocal_int_to_str((int)prot, prot_str, sizeof(prot_str));
				sprintf(flow, "%-23s %-39s --> %-45s %s", colloct, s_ip, d_ip, prot_str);	
				top_insert(flow_top, flow, bytes);
			}
		}
		/* 记录源集群的流量总和 */
		else if(kind == TOP_SRC_SET)
		{
			top_insert(src_set_top, row[MYSQL_FILED_SRCSET], bytes);
		}
		/* 记录目的集群的流量总和 */
		else if(kind == TOP_DST_SET)
		{
			top_insert(dst_set_top, row[MYSQL_FILED_DSTSET], bytes);
		}
		/* 记录源业务的流量总和 */
			else if(kind == TOP_SRC_BIZ)
		{
		top_insert(src_biz_top, row[MYSQL_FILED_SRCBIZ], bytes);
		}
		/* 记录目的业务的流量总和 */
		else if(kind == TOP_DST_BIZ)
		{
			top_insert(dst_biz_top, row[MYSQL_FILED_DSTBIZ], bytes);
		}
	}
	mysql_free_result(res);
	time(&timee);
	myprintf("[Cost %ld seconds]:%s\n", timee - times, query);

	return 0;
}

char *get_top(MYSQL *mysql, time_t start_time, time_t end_time, int kind)
{
	int i, s_week, e_week, interval;
	char query[1024], s_time[128], e_time[128], week_str[4];
	time_t time_now;
	
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
	/* 当interval=0的时候，有两种情况：1、时间跨度只包含一天。2、时间跨度将近但未到达七天，如：上个周六晚8点到这个周六早10点。*/
	if(interval == 0)
	{
		wday_int_to_str(s_week, week_str, sizeof(week_str));
		sprintf(query, "select * from %s_%s where %s >= '%s' and %s <= '%s'", TABLE_NAME, week_str, MYSQL_TIMESTAMP, s_time, MYSQL_TIMESTAMP, e_time);
		top_query(mysql, query, kind);
		/* 如果是第二种情况(时间跨度超过一天), 要查询其他6张表的全部 */
		if(end_time - start_time > 60 * 60 * 24)
		{
			for(i = (s_week + 1) % 7; i != s_week; i = (i + 1) % 7)
			{
				wday_int_to_str(i, week_str, sizeof(week_str));
				sprintf(query, "select * from %s_%s", TABLE_NAME, week_str);
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
				sprintf(query, "select * from %s_%s where %s >= '%s'", TABLE_NAME, week_str, MYSQL_TIMESTAMP, s_time);
			}
			else if(i == interval) /* 查询最后一张表的部分 */
			{
				sprintf(query, "select * from %s_%s where %s <= '%s'", TABLE_NAME, week_str, MYSQL_TIMESTAMP, e_time);
			}
			else /* 查询其他表的全部 */
			{
				sprintf(query, "select * from %s_%s", TABLE_NAME, week_str);
			}
			top_query(mysql, query, kind);
		}
	}
	build_response_body_json(kind);

	return top_response_body;
}
