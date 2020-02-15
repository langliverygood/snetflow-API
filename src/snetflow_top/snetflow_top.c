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

static map<string, uint64_t> src_ip_top;
static map<string, uint64_t> dst_ip_top;
static map<string, uint64_t> src_set_top;
static map<string, uint64_t> dst_set_top;
static map<string, uint64_t> src_biz_top;
static map<string, uint64_t> dst_biz_top;
static char *top_response_body;

static void top_init()
{
	top_response_body = NULL;
	src_ip_top.clear();
	dst_ip_top.clear();
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

static void build_response_body_json()
{
	map<string, uint64_t>::iterator it;
	char bytes[32];
	string s;
	cJSON *root, *src_ip, *dst_ip, *src_set, *dst_set, *src_biz, *dst_biz, *tmp;

	root = cJSON_CreateObject();
	src_ip = cJSON_AddArrayToObject(root, "src_ip");
	dst_ip = cJSON_AddArrayToObject(root, "dst_ip");
	src_set = cJSON_AddArrayToObject(root, "src_set");
	dst_set = cJSON_AddArrayToObject(root, "dst_set");
	src_biz = cJSON_AddArrayToObject(root, "src_biz");
	dst_biz = cJSON_AddArrayToObject(root, "dst_biz");

    /* 添加源ip的流量 */
	for(it = src_ip_top.begin(); it != src_ip_top.end(); it++) 
	{
		tmp = cJSON_CreateObject();
        s = it->first;
        cJSON_AddStringToObject(tmp, "src_ip", s.c_str());
		sprintf(bytes, "%lu", it->second);
        cJSON_AddStringToObject(tmp, "bytes", bytes);
        cJSON_AddItemToArray(src_ip, tmp);
	}
	/* 添加目的ip的流量 */
	for(it = dst_ip_top.begin(); it != dst_ip_top.end(); it++) 
	{
		tmp = cJSON_CreateObject();
        s = it->first;
        cJSON_AddStringToObject(tmp, "dst_ip", s.c_str());
        sprintf(bytes, "%lu", it->second);
        cJSON_AddStringToObject(tmp, "bytes", bytes);
        cJSON_AddItemToArray(dst_ip, tmp);
	}
	/* 添加源集群的流量 */
	for(it = src_set_top.begin(); it != src_set_top.end(); it++) 
	{
		tmp = cJSON_CreateObject();
        s = it->first;
        cJSON_AddStringToObject(tmp, "src_set", s.c_str());
		sprintf(bytes, "%lu", it->second);
		cJSON_AddStringToObject(tmp, "bytes", bytes);
        cJSON_AddItemToArray(src_set, tmp);
	}
	/* 添加目的集群的流量 */
	for(it = dst_set_top.begin(); it != dst_set_top.end(); it++) 
	{
		tmp = cJSON_CreateObject();
        s = it->first;
        cJSON_AddStringToObject(tmp, "dst_set", s.c_str());
        sprintf(bytes, "%lu", it->second);
        cJSON_AddStringToObject(tmp, "bytes", bytes);
        cJSON_AddItemToArray(dst_set, tmp);
	}
	/* 添加源业务的流量 */
	for(it = src_biz_top.begin(); it != src_biz_top.end(); it++) 
	{
		tmp = cJSON_CreateObject();
        s = it->first;
        cJSON_AddStringToObject(tmp, "src_biz", s.c_str());
        sprintf(bytes, "%lu", it->second);
        cJSON_AddStringToObject(tmp, "bytes", bytes);
        cJSON_AddItemToArray(src_biz, tmp);
	}
	/* 添加目的业务的流量 */
	for(it = dst_biz_top.begin(); it != dst_biz_top.end(); it++) 
	{
		tmp = cJSON_CreateObject();
        s = it->first;
        cJSON_AddStringToObject(tmp, "dst_biz", s.c_str());
        sprintf(bytes, "%lu", it->second);
        cJSON_AddStringToObject(tmp, "bytes", bytes);
        cJSON_AddItemToArray(dst_biz, tmp);
	}
	top_response_body = cJSON_Print(root);
	cJSON_Delete(root);
	
	return;
}

char *get_top(MYSQL *mysql, time_t start_time, time_t end_time)
{
	int flag;
	long int ip, bytes;
	char query[1024], s_time[128], e_time[128];
	MYSQL_RES *res;
	MYSQL_ROW row;
	struct in_addr ip_addr;

    /* 每次查询都要将上次的结果清空 */
	top_init();
	/* 时间戳转字符串 */
	timestamp_to_str(start_time, s_time, sizeof(s_time));
	timestamp_to_str(end_time, e_time, sizeof(e_time));
	/* 构建sql语句并查询 */
	sprintf(query, "select * from %s where %s >= '%s' and %s <= '%s'", TABLE_NAME, MYSQL_TIMESTAMP, s_time, MYSQL_TIMESTAMP, e_time);
	flag = mysql_real_query(mysql, query, (unsigned int)strlen(query));
	if(flag)
	{
		if(DEBUG)
		{
			printf("Query failed!\n");
		}
		return 0;
	}
	
	/* mysql_store_result将全部的查询结果读取到客户端 */
	res = mysql_store_result(mysql);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         	/*mysql_fetch_row检索结果集的下一行*/
	while((row = mysql_fetch_row(res)))
	{
		/* bytes字段转化为long int */
		if(str_to_long(row[MYSQL_FILED_BYTES], &bytes))
		{
				continue;
		}
		/* 记录源ip的流量总和 */
		if(!str_to_long(row[MYSQL_FILED_SRCIP], &ip))
		{
			ip_addr.s_addr = htonl((uint32_t)ip);
			top_insert(src_ip_top, inet_ntoa(ip_addr), bytes);
		}
		/* 记录目的ip的流量总和 */
		if(!str_to_long(row[MYSQL_FILED_DSTIP], &ip))
		{
			ip_addr.s_addr = htonl((uint32_t)ip);
			top_insert(dst_ip_top, inet_ntoa(ip_addr), bytes);
		}
		/* 记录源集群的流量总和 */
		top_insert(src_set_top, row[MYSQL_FILED_SRCSET], bytes);
		/* 记录目的集群的流量总和 */
		top_insert(dst_set_top, row[MYSQL_FILED_DSTSET], bytes);
		/* 记录源业务的流量总和 */
		top_insert(src_biz_top, row[MYSQL_FILED_SRCBIZ], bytes);
		/* 记录目的业务的流量总和 */
		top_insert(dst_biz_top, row[MYSQL_FILED_DSTBIZ], bytes);
	}
	build_response_body_json();

	return top_response_body;
}
