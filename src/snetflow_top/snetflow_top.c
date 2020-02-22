#include <stdio.h>
#include <stdint.h>
#include <string.h>  
#include <arpa/inet.h>
#include <mysql/mysql.h>
#include <cjson/cJSON.h>

#include "common.h"
#include "snetflow_top.h"

using namespace std;

static void top_insert(map<string, uint64_t> *my_map, const char *key, uint64_t bytes)
{
	string k;

	k = key;
	(*my_map)[k] += bytes;

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
static int top_query(MYSQL *mysql, const char *query, int kind, map<string, uint64_t> *mymap)
{
	int flag;
	char s_ip[64], d_ip[64], flow[512], prot_str[16];
	long int ip1, ip2, bytes, prot;
	time_t times, timee;
	struct in_addr ip_addr1, ip_addr2;
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
		/* 记录源集群/目的集群/源业务/目的业务的流量总和 */
		if(kind == TOP_SRC_SET || kind == TOP_DST_SET || kind == TOP_SRC_BIZ || kind == TOP_DST_BIZ)
		{
			top_insert(mymap, row[1], bytes);
		}
		/* 记录某个网络区域的流 */
		else
		{
			if((str_to_long(row[1], &ip1) == 0) && (str_to_long(row[3], &ip2) == 0) && (str_to_long(row[6], &prot) == 0))
			{
				memset(flow, 0, sizeof(flow));
				ip_addr1.s_addr = htonl((uint32_t)ip1);
				ip_addr2.s_addr = htonl((uint32_t)ip2);
				ipprotocal_int_to_str((int)prot, prot_str, sizeof(prot_str));
				sprintf(s_ip, "%s", inet_ntoa(ip_addr1));
				sprintf(d_ip, "%s", inet_ntoa(ip_addr2));
				sprintf(flow, "%s(%s)-->%s:%s(%s) %s", s_ip, row[2], d_ip, row[4], row[5], prot_str);	
				top_insert(mymap, flow, bytes);
			}
		}
	}
	mysql_free_result(res);
	time(&timee);
	myprintf("[Cost %ld seconds]:%s\n", timee - times, query);
	
	return 0;
}

int get_top(MYSQL *mysql, time_t start_time, time_t end_time, int kind, void* mymap)
{
	int i, s_week, e_week, interval;
	char query[1024], week_str[4], region[16];
	char column[128], condition[128];
	time_t time_now;

	memset(condition, 0, sizeof(condition));
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
	/* 获得时间戳对应的星期 */
	s_week = get_wday_by_timestamp(start_time);
	e_week = get_wday_by_timestamp(end_time);
	/* 时间跨度 */
	interval = e_week - s_week;
	if(interval < 0)
	{
		interval += 7;
	}
	/* 确定查询条件 */
	memset(region, 0, sizeof(region));
	if(kind == TOP_SRC_SET)
	{
		sprintf(column, "%s,%s", MYSQL_BYTES, MYSQL_SRCSET);
	}
	else if(kind == TOP_DST_SET)
	{
		sprintf(column, "%s,%s", MYSQL_BYTES, MYSQL_DSTSET);
	}
	else if(kind == TOP_SRC_BIZ)
	{
		sprintf(column, "%s,%s", MYSQL_BYTES, MYSQL_SRCBIZ);
	}
	else if(kind == TOP_DST_BIZ)
	{
		sprintf(column, "%s,%s", MYSQL_BYTES, MYSQL_DSTBIZ);;
	}
	else if(kind == TOP_SRC_HXQ)
	{
		sprintf(column, "%s,%s,%s,%s,%s,%s,%s", MYSQL_BYTES, MYSQL_SRCIP, MYSQL_SRCBIZ, MYSQL_DSTIP, MYSQL_DSTPORT, MYSQL_DSTBIZ, MYSQL_PROT);
		sprintf(condition, "%s = '%s'",  MYSQL_SRCREGION, NET_HXQ_CN);
		sprintf(region, "%s", NET_HXQ);
	}
	else if(kind == TOP_DST_HXQ)
	{
		sprintf(column, "%s,%s,%s,%s,%s,%s,%s", MYSQL_BYTES, MYSQL_SRCIP, MYSQL_SRCBIZ, MYSQL_DSTIP, MYSQL_DSTPORT, MYSQL_DSTBIZ, MYSQL_PROT);
		sprintf(condition, "%s = '%s'",  MYSQL_DSTREGION, NET_HXQ_CN);
		sprintf(region, "%s", NET_HXQ);
	}
	else if(kind == TOP_SRC_HXQ_ZB)
	{
		sprintf(column, "%s,%s,%s,%s,%s,%s,%s", MYSQL_BYTES, MYSQL_SRCIP, MYSQL_SRCBIZ, MYSQL_DSTIP, MYSQL_DSTPORT, MYSQL_DSTBIZ, MYSQL_PROT);
		sprintf(condition, "%s = '%s'",  MYSQL_SRCREGION, NET_HXQ_ZB_CN);
		sprintf(region, "%s", NET_HXQ_ZB);
	}
	else if(kind == TOP_DST_HXQ_ZB)
	{
		sprintf(column, "%s,%s,%s,%s,%s,%s,%s", MYSQL_BYTES, MYSQL_SRCIP, MYSQL_SRCBIZ, MYSQL_DSTIP, MYSQL_DSTPORT, MYSQL_DSTBIZ, MYSQL_PROT);
		sprintf(condition, "%s = '%s'",  MYSQL_DSTREGION, NET_HXQ_ZB_CN);
		sprintf(region, "%s", NET_HXQ_ZB);
	}
	else if(kind == TOP_SRC_HXQ_SF)
	{
		sprintf(column, "%s,%s,%s,%s,%s,%s,%s", MYSQL_BYTES, MYSQL_SRCIP, MYSQL_SRCBIZ, MYSQL_DSTIP, MYSQL_DSTPORT, MYSQL_DSTBIZ, MYSQL_PROT);
		sprintf(condition, "%s = '%s'",  MYSQL_SRCREGION, NET_HXQ_SF_CN);
		sprintf(region, "%s", NET_HXQ_SF);
	}
	else if(kind == TOP_DST_HXQ_SF)
	{
		sprintf(column, "%s,%s,%s,%s,%s,%s,%s", MYSQL_BYTES, MYSQL_SRCIP, MYSQL_SRCBIZ, MYSQL_DSTIP, MYSQL_DSTPORT, MYSQL_DSTBIZ, MYSQL_PROT);
		sprintf(condition, "%s = '%s'",  MYSQL_DSTREGION, NET_HXQ_SF_CN);
		sprintf(region, "%s", NET_HXQ_SF);
	}
	else if(kind == TOP_SRC_GLQ)
	{
		sprintf(column, "%s,%s,%s,%s,%s,%s,%s", MYSQL_BYTES, MYSQL_SRCIP, MYSQL_SRCBIZ, MYSQL_DSTIP, MYSQL_DSTPORT, MYSQL_DSTBIZ, MYSQL_PROT);
		sprintf(condition, "%s = '%s'",  MYSQL_SRCREGION, NET_GLQ_CN);
		sprintf(region, "%s", NET_GLQ);
	}
	else if(kind == TOP_DST_GLQ)
	{
		sprintf(column, "%s,%s,%s,%s,%s,%s,%s", MYSQL_BYTES, MYSQL_SRCIP, MYSQL_SRCBIZ, MYSQL_DSTIP, MYSQL_DSTPORT, MYSQL_DSTBIZ, MYSQL_PROT);
		sprintf(condition, "%s = '%s'",  MYSQL_DSTREGION, NET_GLQ_CN);
		sprintf(region, "%s", NET_GLQ);
	}
	else if(kind == TOP_SRC_RZQ)
	{
		sprintf(column, "%s,%s,%s,%s,%s,%s,%s", MYSQL_BYTES, MYSQL_SRCIP, MYSQL_SRCBIZ, MYSQL_DSTIP, MYSQL_DSTPORT, MYSQL_DSTBIZ, MYSQL_PROT);
		sprintf(condition, "%s = '%s'",  MYSQL_SRCREGION, NET_RZQ_CN);
		sprintf(region, "%s", NET_RZQ);
	}
	else if(kind == TOP_DST_RZQ)
	{
		sprintf(column, "%s,%s,%s,%s,%s,%s,%s", MYSQL_BYTES, MYSQL_SRCIP, MYSQL_SRCBIZ, MYSQL_DSTIP, MYSQL_DSTPORT, MYSQL_DSTBIZ, MYSQL_PROT);
		sprintf(condition, "%s = '%s'",  MYSQL_DSTREGION, NET_RZQ_CN);
		sprintf(region, "%s", NET_RZQ);
	}
	else if(kind == TOP_SRC_JRQ)
	{
		sprintf(column, "%s,%s,%s,%s,%s,%s,%s", MYSQL_BYTES, MYSQL_SRCIP, MYSQL_SRCBIZ, MYSQL_DSTIP, MYSQL_DSTPORT, MYSQL_DSTBIZ, MYSQL_PROT);
		sprintf(condition, "%s = '%s'",  MYSQL_SRCREGION, NET_JRQ_CN);
		sprintf(region, "%s", NET_JRQ);
	}
	else if(kind == TOP_DST_JRQ)
	{
		sprintf(column, "%s,%s,%s,%s,%s,%s,%s", MYSQL_BYTES, MYSQL_SRCIP, MYSQL_SRCBIZ, MYSQL_DSTIP, MYSQL_DSTPORT, MYSQL_DSTBIZ, MYSQL_PROT);
		sprintf(condition, "%s = '%s'",  MYSQL_DSTREGION, NET_JRQ_CN);
		sprintf(region, "%s", NET_JRQ);
	}
	else if(kind == TOP_SRC_CSQ)
	{
		sprintf(column, "%s,%s,%s,%s,%s,%s,%s", MYSQL_BYTES, MYSQL_SRCIP, MYSQL_SRCBIZ, MYSQL_DSTIP, MYSQL_DSTPORT, MYSQL_DSTBIZ, MYSQL_PROT);
		sprintf(condition, "%s = '%s'",  MYSQL_SRCREGION, NET_CSQ_CN);
		sprintf(region, "%s", NET_CSQ);
	}
	else if(kind == TOP_DST_CSQ)
	{
		sprintf(column, "%s,%s,%s,%s,%s,%s,%s", MYSQL_BYTES, MYSQL_SRCIP, MYSQL_SRCBIZ, MYSQL_DSTIP, MYSQL_DSTPORT, MYSQL_DSTBIZ, MYSQL_PROT);
		sprintf(condition, "%s = '%s'",  MYSQL_DSTREGION, NET_CSQ_CN);
		sprintf(region, "%s", NET_CSQ);
	}
	else if(kind == TOP_SRC_DMZ)
	{
		sprintf(column, "%s,%s,%s,%s,%s,%s,%s", MYSQL_BYTES, MYSQL_SRCIP, MYSQL_SRCBIZ, MYSQL_DSTIP, MYSQL_DSTPORT, MYSQL_DSTBIZ, MYSQL_PROT);
		sprintf(condition, "%s = '%s'",  MYSQL_SRCREGION, NET_DMZ_CN);
		sprintf(region, "%s", NET_DMZ);
	}
	else if(kind == TOP_DST_DMZ)
	{
		sprintf(column, "%s,%s,%s,%s,%s,%s,%s", MYSQL_BYTES, MYSQL_SRCIP, MYSQL_SRCBIZ, MYSQL_DSTIP, MYSQL_DSTPORT, MYSQL_DSTBIZ, MYSQL_PROT);
		sprintf(condition, "%s = '%s'",  MYSQL_DSTREGION, NET_DMZ_CN);
		sprintf(region, "%s", NET_DMZ);
	}
	else if(kind == TOP_SRC_ALQ)
	{
		sprintf(column, "%s,%s,%s,%s,%s,%s,%s", MYSQL_BYTES, MYSQL_SRCIP, MYSQL_SRCBIZ, MYSQL_DSTIP, MYSQL_DSTPORT, MYSQL_DSTBIZ, MYSQL_PROT);
		sprintf(condition, "%s = '%s'",  MYSQL_SRCREGION, NET_ALQ_CN);
		sprintf(region, "%s", NET_ALQ);
	}
	else if(kind == TOP_DST_ALQ)
	{
		sprintf(column, "%s,%s,%s,%s,%s,%s,%s", MYSQL_BYTES, MYSQL_SRCIP, MYSQL_SRCBIZ, MYSQL_DSTIP, MYSQL_DSTPORT, MYSQL_DSTBIZ, MYSQL_PROT);
		sprintf(condition, "%s = '%s'",  MYSQL_DSTREGION, NET_ALQ_CN);
		sprintf(region, "%s", NET_ALQ);
	}
	else if(kind == TOP_SRC_WBQ)
	{
		sprintf(column, "%s,%s,%s,%s,%s,%s,%s", MYSQL_BYTES, MYSQL_SRCIP, MYSQL_SRCBIZ, MYSQL_DSTIP, MYSQL_DSTPORT, MYSQL_DSTBIZ, MYSQL_PROT);
		sprintf(condition, "%s = '%s'",  MYSQL_SRCREGION, NET_WBQ_CN);
		sprintf(region, "%s", NET_WBQ);
	}
	else if(kind == TOP_DST_WBQ)
	{
		sprintf(column, "%s,%s,%s,%s,%s,%s,%s", MYSQL_BYTES, MYSQL_SRCIP, MYSQL_SRCBIZ, MYSQL_DSTIP, MYSQL_DSTPORT, MYSQL_DSTBIZ, MYSQL_PROT);
		sprintf(condition, "%s = '%s'",  MYSQL_DSTREGION, NET_WBQ_CN);
		sprintf(region, "%s", NET_WBQ);
	}
	else if(kind == TOP_SRC_UNKNOWN)
	{
		sprintf(column, "%s,%s,%s,%s,%s,%s,%s", MYSQL_BYTES, MYSQL_SRCIP, MYSQL_SRCBIZ, MYSQL_DSTIP, MYSQL_DSTPORT, MYSQL_DSTBIZ, MYSQL_PROT);
		sprintf(condition, "%s = '%s'",  MYSQL_SRCREGION, NET_UNKNOWN);
		sprintf(region, "%s", NET_UNKNOWN);
	}
	else if(kind == TOP_DST_UNKNOWN)
	{
		sprintf(column, "%s,%s,%s,%s,%s,%s,%s", MYSQL_BYTES, MYSQL_SRCIP, MYSQL_SRCBIZ, MYSQL_DSTIP, MYSQL_DSTPORT, MYSQL_DSTBIZ, MYSQL_PROT);
		sprintf(condition, "%s = '%s'",  MYSQL_DSTREGION, NET_UNKNOWN);
		sprintf(region, "%s", NET_UNKNOWN);
	}
	else
	{
		return -1;
	}
	/* 当interval=0的时候，有两种情况：1、时间跨度只包含一天。2、时间跨度将近但未到达七天，如：上个周六晚8点到这个周六早10点。*/
	if(interval == 0)
	{
		wday_int_to_str(s_week, week_str, sizeof(week_str));
		sprintf(query, "select %s from %s_%s%s where %s >= %lu and %s <= %lu", column, TABLE_NAME, week_str, region, MYSQL_TIMESTAMP, start_time, MYSQL_TIMESTAMP, end_time);
		if(strlen(condition) > 0)
		{
			strcat(query, " and ");
			strcat(query, condition);
		}
		top_query(mysql, query, kind, (map<string, uint64_t> *)mymap);
		/* 如果是第二种情况(时间跨度超过一天), 要查询其他6张表的全部 */
		if(end_time - start_time > 60 * 60 * 24)
		{
			for(i = (s_week + 1) % 7; i != s_week; i = (i + 1) % 7)
			{
				wday_int_to_str(i, week_str, sizeof(week_str));
				sprintf(query, "select %s from %s_%s%s", column, TABLE_NAME, week_str, region);
				if(strlen(condition) > 0)
				{
					strcat(query, " where ");
					strcat(query, condition);
				}
				top_query(mysql, query, kind, (map<string, uint64_t> *)mymap);
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
				sprintf(query, "select %s from %s_%s%s where %s >= %lu", column, TABLE_NAME, week_str, region, MYSQL_TIMESTAMP, start_time);
				if(strlen(condition) > 0)
				{
					strcat(query, " and ");
					strcat(query, condition);
				}
			}
			else if(i == interval) /* 查询最后一张表的部分 */
			{
				sprintf(query, "select %s from %s_%s%s where %s <= %lu", column, TABLE_NAME, week_str, region, MYSQL_TIMESTAMP, end_time);
				if(strlen(condition) > 0)
				{
					strcat(query, " and ");
					strcat(query, condition);
				}
			}
			else /* 查询其他表的全部 */
			{
				sprintf(query, "select %s from %s_%s%s", column, TABLE_NAME, week_str, region);
				if(strlen(condition) > 0)
				{
					strcat(query, " where ");
					strcat(query, condition);
				}
			}
			top_query(mysql, query, kind, (map<string, uint64_t> *)mymap);
		}
	}
	
	return 0;

}
