#include <stdio.h>
#include <stdint.h>
#include <string.h>  
#include <arpa/inet.h>
#include <mysql/mysql.h>

#include "common.h"
#include "config.h"
#include "snetflow_history.h"

using namespace std;

/* 从数据查询结果，并写入相应的map 中 */
static int history_query(MYSQL *mysql, const char *query, vector<history_s> *history_vec)
{
	int flag;
	char s_ip[64], d_ip[64], prot_str[16], time_str[32];
	long int ip1, ip2, bytes, prot, timestamp;
	uint32_t byte_to_bit;
	time_t times, timee;
	struct in_addr ip_addr1, ip_addr2;
	history_s his;
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
		if(str_to_long(row[0], &bytes) == 0 && (str_to_long(row[1], &ip1) == 0) && (str_to_long(row[3], &ip2) == 0) && (str_to_long(row[6], &prot) == 0) && (str_to_long(row[7], &timestamp) == 0))
		{
			ip_addr1.s_addr = htonl((uint32_t)ip1);
			ip_addr2.s_addr = htonl((uint32_t)ip2);
			ipprotocal_int_to_str((int)prot, prot_str, sizeof(prot_str));
			sprintf(s_ip, "%s", inet_ntoa(ip_addr1));
			sprintf(d_ip, "%s", inet_ntoa(ip_addr2));
			timestamp_to_str(timestamp, time_str, sizeof(time_str));
			sprintf(his.flow, "[%s]%s(%s)-->%s:%s(%s) %s", time_str, s_ip, row[2], d_ip, row[4], row[5], prot_str);
			his.bytes = (bytes * byte_to_bit);
			history_vec->push_back(his);
		}
	}
	mysql_free_result(res);
	time(&timee);
	myprintf("[Cost %ld seconds]:%s\n", timee - times, query);
	
	return 0;
}

/* */
int get_history(MYSQL *mysql, time_t start_time, time_t end_time, mysql_conf_s *cfg, vector<history_s>* history_vec)
{
	int s_week;
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
	/* history只查一张表，且有limit限制 */
	wday_int_to_str(s_week, week_str, sizeof(week_str));
	sprintf(query, "select %s from record_%s%s where %s >= %lu and %s <= %lu %s limit %u", cfg->column, week_str, cfg->table, timestamp, start_time, timestamp, end_time, cfg->condition, cfg_get_history_num());
	history_query(mysql, query, history_vec);	
	
	return 0;
}