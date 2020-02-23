#include <stdio.h>
#include <stdint.h>
#include <string.h>  
#include <arpa/inet.h>
#include <mysql/mysql.h>

#include "common.h"
#include "snetflow_trend.h"

using namespace std;

/* 精确到秒统计 */
static void trend_insert(map<time_t, uint64_t> *trend_map, time_t key, uint64_t bytes)
{
	(*trend_map)[key] += bytes;

	return;
}

/* 从数据查询结果，并写入相应的map 中 */
static int trend_query(MYSQL *mysql, const char *query, int kind, map<time_t, uint64_t> *raw_map)
{
	int flag;
	long int time_long, bytes_long;
	uint64_t bytes;
	time_t times, timee, timestamp;
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
		if(str_to_long(row[0], &bytes_long) == 0 && (str_to_long(row[1], &time_long) == 0))
		{
			timestamp = (time_t)time_long;
			bytes = (uint64_t)bytes_long;
			trend_insert(raw_map, timestamp, bytes);
		}
	}
	mysql_free_result(res);
	time(&timee);
	myprintf("[Cost %ld seconds]:%s\n", timee - times, query);
	
	return 0;
}

int get_trend(MYSQL *mysql, time_t start_time, time_t end_time, int kind, void* trend_map)
{
	int i, s_week, e_week, interval;
	time_t next, inter;
	char query[1024], week_str[4], region[16];
	char column[128], condition[128];
	time_t time_now;
	map<time_t, uint64_t> raw_map;
	map<time_t, uint64_t>::iterator it;
	map<uint64_t, uint64_t> *p;
	
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
	sprintf(column, "%s,%s", MYSQL_BYTES, MYSQL_TIMESTAMP);
	if(kind == TREND_SRC_HXQ)
	{
		sprintf(condition, "%s = '%s'",  MYSQL_SRCREGION, NET_HXQ_CN);
		sprintf(region, "%s", NET_HXQ);
	}
	else if(kind == TREND_DST_HXQ)
	{
		sprintf(condition, "%s = '%s'",  MYSQL_DSTREGION, NET_HXQ_CN);
		sprintf(region, "%s", NET_HXQ);
	}
	else if(kind == TREND_SRC_HXQ_ZB)
	{
		sprintf(condition, "%s = '%s'",  MYSQL_SRCREGION, NET_HXQ_ZB_CN);
		sprintf(region, "%s", NET_HXQ_ZB);
	}
	else if(kind == TREND_DST_HXQ_ZB)
	{
		sprintf(condition, "%s = '%s'",  MYSQL_DSTREGION, NET_HXQ_ZB_CN);
		sprintf(region, "%s", NET_HXQ_ZB);
	}
	else if(kind == TREND_SRC_HXQ_SF)
	{
		sprintf(condition, "%s = '%s'",  MYSQL_SRCREGION, NET_HXQ_SF_CN);
		sprintf(region, "%s", NET_HXQ_SF);
	}
	else if(kind == TREND_DST_HXQ_SF)
	{
		sprintf(condition, "%s = '%s'",  MYSQL_DSTREGION, NET_HXQ_SF_CN);
		sprintf(region, "%s", NET_HXQ_SF);
	}
	else if(kind == TREND_SRC_GLQ)
	{
		sprintf(condition, "%s = '%s'",  MYSQL_SRCREGION, NET_GLQ_CN);
		sprintf(region, "%s", NET_GLQ);
	}
	else if(kind == TREND_DST_GLQ)
	{
		sprintf(condition, "%s = '%s'",  MYSQL_DSTREGION, NET_GLQ_CN);
		sprintf(region, "%s", NET_GLQ);
	}
	else if(kind == TREND_SRC_RZQ)
	{
		sprintf(condition, "%s = '%s'",  MYSQL_SRCREGION, NET_RZQ_CN);
		sprintf(region, "%s", NET_RZQ);
	}
	else if(kind == TREND_DST_RZQ)
	{
		sprintf(condition, "%s = '%s'",  MYSQL_DSTREGION, NET_RZQ_CN);
		sprintf(region, "%s", NET_RZQ);
	}
	else if(kind == TREND_SRC_JRQ)
	{
		sprintf(condition, "%s = '%s'",  MYSQL_SRCREGION, NET_JRQ_CN);
		sprintf(region, "%s", NET_JRQ);
	}
	else if(kind == TREND_DST_JRQ)
	{
		sprintf(condition, "%s = '%s'",  MYSQL_DSTREGION, NET_JRQ_CN);
		sprintf(region, "%s", NET_JRQ);
	}
	else if(kind == TREND_SRC_CSQ)
	{
		sprintf(column, "%s,%s,%s,%s,%s,%s,%s", MYSQL_BYTES, MYSQL_SRCIP, MYSQL_SRCBIZ, MYSQL_DSTIP, MYSQL_DSTPORT, MYSQL_DSTBIZ, MYSQL_PROT);
		sprintf(condition, "%s = '%s'",  MYSQL_SRCREGION, NET_CSQ_CN);
		sprintf(region, "%s", NET_CSQ);
	}
	else if(kind == TREND_DST_CSQ)
	{
		sprintf(condition, "%s = '%s'",  MYSQL_DSTREGION, NET_CSQ_CN);
		sprintf(region, "%s", NET_CSQ);
	}
	else if(kind == TREND_SRC_DMZ)
	{
		sprintf(condition, "%s = '%s'",  MYSQL_SRCREGION, NET_DMZ_CN);
		sprintf(region, "%s", NET_DMZ);
	}
	else if(kind == TREND_DST_DMZ)
	{
		sprintf(condition, "%s = '%s'",  MYSQL_DSTREGION, NET_DMZ_CN);
		sprintf(region, "%s", NET_DMZ);
	}
	else if(kind == TREND_SRC_ALQ)
	{
		sprintf(condition, "%s = '%s'",  MYSQL_SRCREGION, NET_ALQ_CN);
		sprintf(region, "%s", NET_ALQ);
	}
	else if(kind == TREND_DST_ALQ)
	{
		sprintf(condition, "%s = '%s'",  MYSQL_DSTREGION, NET_ALQ_CN);
		sprintf(region, "%s", NET_ALQ);
	}
	else if(kind == TREND_SRC_WBQ)
	{
		sprintf(condition, "%s = '%s'",  MYSQL_SRCREGION, NET_WBQ_CN);
		sprintf(region, "%s", NET_WBQ);
	}
	else if(kind == TREND_DST_WBQ)
	{
		sprintf(condition, "%s = '%s'",  MYSQL_DSTREGION, NET_WBQ_CN);
		sprintf(region, "%s", NET_WBQ);
	}
	else if(kind == TREND_SRC_UNKNOWN)
	{
		sprintf(condition, "%s = '%s'",  MYSQL_SRCREGION, NET_UNKNOWN);
		sprintf(region, "%s", NET_UNKNOWN);
	}
	else if(kind == TREND_DST_UNKNOWN)
	{
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
		trend_query(mysql, query, kind, &raw_map);
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
				trend_query(mysql, query, kind, &raw_map);
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
			trend_query(mysql, query, kind, &raw_map);
		}
	}
	/* 分段统计 */
	inter = (end_time - start_time) / 10;
	it = raw_map.begin();
	p = (map<uint64_t, uint64_t> *)trend_map;
	//(*p)[it->first] = 0;
	for(next = it->first + inter; it != raw_map.end();)
	{
		if(it->first <= next)
		{
			(*p)[next] += it->second;
			it++;
		}
		else
		{
			next += inter;
			if(next > end_time)
			{
				next = end_time;
			}
		}
	}
	
	return 0;
}
