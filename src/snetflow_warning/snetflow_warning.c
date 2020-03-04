#include <stdio.h>
#include <stdint.h>
#include <string.h>  
#include <arpa/inet.h>
#include <mysql/mysql.h>

#include "common.h"
#include "config.h"
#include "snetflow_warning.h"

using namespace std;

/* 从数据查询结果，并写入相应的map 中 */
static int warning_query(MYSQL *mysql, const char *query, mysql_conf_s *cfg, map<uint32_t, warning_msg_s> *warning_map)
{
	int flag;
	long int ip, bytes;
	time_t times, timee;
	struct in_addr ip_addr;
	warning_msg_s *war;
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
		if(str_to_long(row[1], &ip) == 0)
		{
			(*warning_map)[ip].bytes += bytes;
			war = &((*warning_map)[ip]);
			ip_addr.s_addr = htonl((uint32_t)ip);
			strncpy(war->ip, inet_ntoa(ip_addr), sizeof(war->ip));
			strncpy(war->biz, row[2], sizeof(war->biz));
			strncpy(war->set, row[3], sizeof(war->set));
			strncpy(war->module, row[4], sizeof(war->module));
			strncpy(war->region, row[5], sizeof(war->region));
			strncpy(war->switches, row[6], sizeof(war->switches));
		}
	}
	mysql_free_result(res);
	time(&timee);
	myprintf("[Cost %ld seconds]:%s\n", timee - times, query);
	
	return 0;
}

int get_warning(MYSQL *mysql, time_t start_time, time_t end_time, mysql_conf_s *cfg, void* warning_map)
{
	int i, s_week, e_week, interval;
	char query[1024], week_str[4], *timestamp;
	time_t time_now;

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
		warning_query(mysql, query, cfg, (map<uint32_t, warning_msg_s> *)warning_map);
		/* 如果是第二种情况(时间跨度超过一天), 要查询其他6张表的全部 */
		if(end_time - start_time > 60 * 60 * 24)
		{
			for(i = (s_week + 1) % 7; i != s_week; i = (i + 1) % 7)
			{
				wday_int_to_str(i, week_str, sizeof(week_str));
				sprintf(query, "select %s from record_%s%s where 1=1 %s", cfg->column, week_str, cfg->table, cfg->condition);
				warning_query(mysql, query, cfg, (map<uint32_t, warning_msg_s> *)warning_map);
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
			warning_query(mysql, query, cfg, (map<uint32_t, warning_msg_s> *)warning_map);
		}
	}
	
	return 0;
}
