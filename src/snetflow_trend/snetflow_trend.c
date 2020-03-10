#include <stdio.h>
#include <stdint.h>
#include <string.h>  
#include <arpa/inet.h>
#include <mysql/mysql.h>

#include "common.h"
#include "config.h"
#include "snetflow_trend.h"

using namespace std;

/* 从数据查询结果，并写入相应的map 中 */
static int trend_query(MYSQL *mysql, const char *query, map<time_t, uint64_t> *raw_map)
{
	int flag;
	long int time_long, bytes_long;
	uint64_t bytes;
	uint32_t byte_to_bit;
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
	byte_to_bit = cfg_get_byte_to_bit();
	res = mysql_use_result(mysql); 
	/*mysql_fetch_row检索结果集的下一行*/
	while((row = mysql_fetch_row(res)))
	{
		if(str_to_long(row[0], &bytes_long) == 0 && (str_to_long(row[1], &time_long) == 0))
		{
			timestamp = (time_t)time_long;
			bytes = (uint64_t)bytes_long;
			/* 精确到秒统计 */
			(*raw_map)[timestamp] += (bytes * byte_to_bit);
		}
	}
	mysql_free_result(res);
	time(&timee);
	myprintf("[Cost %ld seconds]:%s\n", timee - times, query);
	
	return 0;
}

int get_trend(MYSQL *mysql, time_t start_time, time_t end_time, mysql_conf_s *cfg, void* trend_map)
{
	int i, s_week, e_week, interval;
	char query[1024], week_str[4], *timestamp;
	time_t next, inter, time_now;
	map<time_t, uint64_t> raw_map;
	map<time_t, uint64_t>::iterator it;
	map<uint64_t, uint64_t> *p;

	timestamp = cfg_get_timestamp();
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
	/* 当interval=0的时候，有两种情况：1、时间跨度只包含一天。2、时间跨度将近但未到达七天，如：上个周六晚8点到这个周六早10点。*/
	if(interval == 0)
	{
		wday_int_to_str(s_week, week_str, sizeof(week_str));
		sprintf(query, "select %s from record_%s%s where %s >= %lu and %s <= %lu %s", cfg->column, week_str, cfg->table, timestamp, start_time, timestamp, end_time, cfg->condition);
		trend_query(mysql, query, &raw_map);
		/* 如果是第二种情况(时间跨度超过一天), 要查询其他6张表的全部 */
		if(end_time - start_time > 60 * 60 * 24)
		{
			for(i = (s_week + 1) % 7; i != s_week; i = (i + 1) % 7)
			{
				wday_int_to_str(i, week_str, sizeof(week_str));
				sprintf(query, "select %s from record_%s%s where 1=1 %s", cfg->column, week_str, cfg->table, cfg->condition);
				trend_query(mysql, query, &raw_map);
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
				sprintf(query, "select %s from record_%s%s where %s >= %lu %s", cfg->column, week_str, cfg->table, timestamp, start_time, cfg->condition);
			}
			else if(i == interval) /* 查询最后一张表的部分 */
			{
				sprintf(query, "select %s from record_%s%s where %s <= %lu %s", cfg->column, week_str, cfg->table, timestamp, end_time, cfg->condition);
			}
			else /* 查询其他表的全部 */
			{
				sprintf(query, "select %s from record_%s%s where 1=1 %s", cfg->column, week_str, cfg->table, cfg->condition);
			}
			trend_query(mysql, query, &raw_map);
		}
	}
	/* 分段统计 */
	inter = (end_time - start_time) / cfg_get_trend_point();
	it = raw_map.begin();
	p = (map<uint64_t, uint64_t> *)trend_map;
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
