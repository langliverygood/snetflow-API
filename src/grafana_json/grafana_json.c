#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <cjson/cJSON.h>
#include <mysql/mysql.h>

#include "common.h"
#include "snetflow_top.h"
#include "snetflow_history.h"
#include "snetflow_trend.h"
#include "grafana_json.h"

using namespace std;

/* 从CJSON结构中获取某个整数值,  返回值0代表成功(写到参数buf中), -1代表失败。 */
static int grafana_get_json_item_int(cJSON *obj, const char *tag, int *buf)
{
	cJSON *item;

	item = cJSON_GetObjectItem(obj, tag);
	if(item)
	{
		*buf = item->valueint;
		return 0;
	}
	
	return -1;
}

/* 从CJSON结构中获取某个字符串,写到参数buf中,可能为空 */
static void grafana_get_json_item_str(cJSON *obj, const char *tag, char *buf, const int buf_len)
{
	cJSON *item;

	memset(buf, 0, buf_len);
	item = cJSON_GetObjectItem(obj, tag);
	if(item)
	{
		strncpy(buf, item->valuestring, buf_len - 1);
	}
	
	return;
}

/* 从grafana的请求参数中获取时间段 */
static void grafana_get_time_from_request(const grafana_query_request_s *rst, char *start_time, char *end_time, int time_len)
{
	memset(start_time, 0, time_len);
	memset(end_time, 0, time_len);
	memcpy(start_time, rst->range.from, 10);
	memcpy(end_time, rst->range.to, 10);
	start_time[10] = ' ';
	end_time[10] = ' ';
	strncat(start_time, &(rst->range.from[11]), 8);
	strncat(end_time, &(rst->range.to[11]), 8);

	return;
}

/* 释放结构体中申请的空间 */
static void grafana_query_free(grafana_query_request_s *rst)
{
	grafana_query_request_target_s *target, *target_next;
	grafana_query_request_adhoc_filter_s *adhoc_filter, *adhoc_filter_next;

	/* 释放target数组所占空间 */
	target = rst->targets;
	if(target)
	{
		target_next = target->next;
		free(target);
		while(target_next != NULL)
		{
			target = target_next;
			target_next = target_next->next;
			free(target);
		}
	}
	/* 释放adhoc_filter数组所占空间 */
	adhoc_filter = rst->adhoc_filters;
	if(adhoc_filter)
	{
		adhoc_filter_next = adhoc_filter->next;
		free(adhoc_filter);
		while(adhoc_filter_next != NULL)
		{
			adhoc_filter = adhoc_filter_next;
			adhoc_filter_next = adhoc_filter_next->next;
			free(adhoc_filter);
		}
	}
	memset(rst, 0, sizeof(grafana_query_request_s));

	return;
}

/* 将grafana请求的JSON结构化,可能会额外申请空间,用完后要释放 */
static int grafana_query_structured(const char *request_body, grafana_query_request_s *rst)
{
	int i, count;
	cJSON *root, *tmp1, *tmp2, *tmp3;
	grafana_query_request_target_s *target, *target_tmp;
	grafana_query_request_adhoc_filter_s *adhoc_filter, *adhoc_filter_tmp;

	memset(rst, 0, sizeof(grafana_query_request_s));
	root = cJSON_Parse(request_body);
	if(!root)
	{
		return -1;
	}
    /* 解析grafana的query    request，解析结果写入结构体 */
	grafana_get_json_item_int(root, "panelId", &(rst->panelid));
	tmp1 = cJSON_GetObjectItem(root, "range");
	if(!tmp1)
	{
		return -1;
	}
	tmp2 = cJSON_GetObjectItem(tmp1, "raw");
	if(!tmp2)
	{
		return -1;
	}
	grafana_get_json_item_str(tmp1, (const char *)"from", rst->range.from, sizeof(rst->range.from));
	grafana_get_json_item_str(tmp1, (const char *)"to", rst->range.to, sizeof(rst->range.to));
	grafana_get_json_item_str(tmp2, (const char *)"from", rst->range.raw.from, sizeof(rst->range.raw.from));
	grafana_get_json_item_str(tmp2, (const char *)"to", rst->range.raw.to, sizeof(rst->range.raw.to));
	tmp1 = cJSON_GetObjectItem(root, "rangeRaw");
	if(!tmp1)
	{
		return -1;
	}
	grafana_get_json_item_str(tmp1, (const char *)"from", rst->range_raw.from, sizeof(rst->range_raw.from));
	grafana_get_json_item_str(tmp1, (const char *)"to", rst->range_raw.to, sizeof(rst->range_raw.to));
	grafana_get_json_item_str(root, (const char *)"interval", rst->interval, sizeof(rst->interval));
	grafana_get_json_item_int(root, (const char *)"intervalMs", &(rst->interval_ms));
	grafana_get_json_item_int(root, (const char *)"maxDataPoints", &(rst->max_data_points));
	/* 解析targets数组 */
	tmp1 = cJSON_GetObjectItem(root, "targets");
	if(!tmp1)
	{
		return -1;
	}
	count = cJSON_GetArraySize(tmp1);
	for(i = 0; i < count; i++)
	{
		tmp2 = cJSON_GetArrayItem(tmp1, i);
		target = (grafana_query_request_target_s *)malloc(sizeof(grafana_query_request_target_s));
		memset(target, 0, sizeof(grafana_query_request_target_s));
		if(i == 0)
		{
			rst->targets = target;
		}
		else
		{
			target_tmp->next = target;
		}
		target_tmp = target;
		grafana_get_json_item_str(tmp2, (const char *)"target", target->target, sizeof(target->target));
		grafana_get_json_item_str(tmp2, (const char *)"refId", target->refid, sizeof(target->target));
		grafana_get_json_item_str(tmp2, (const char *)"type", target->type, sizeof(target->target));
		grafana_get_json_item_str(tmp2, (const char *)"target", target->target, sizeof(target->target));
		tmp3 = cJSON_GetObjectItem(tmp2, "data");
		if(!tmp3)
		{
			grafana_query_free(rst);
			return -1;
		}
		grafana_get_json_item_str(tmp3, (const char *)"additional", target->data.additional, sizeof(target->data.additional));
	}
	/* 解析adhoc_filters数组 */
	tmp1 = cJSON_GetObjectItem(root, "adhocFilters");
	if(!tmp1)
	{
		grafana_query_free(rst);
		return -1;
	}
	count = cJSON_GetArraySize(tmp1);
	for(i = 0; i < count; i++)
	{
		tmp2 = cJSON_GetArrayItem(tmp1, i);
		adhoc_filter = (grafana_query_request_adhoc_filter_s *)malloc(sizeof(grafana_query_request_adhoc_filter_s));
		memset(adhoc_filter, 0, sizeof(grafana_query_request_adhoc_filter_s));
		if(i == 0)
		{
			rst->adhoc_filters = adhoc_filter;
		}
		else
		{
			adhoc_filter_tmp->next = adhoc_filter;
		}
		adhoc_filter_tmp = adhoc_filter;
		grafana_get_json_item_str(tmp2, (const char *)"key", adhoc_filter->key, sizeof(adhoc_filter->key));
		grafana_get_json_item_str(tmp2, (const char *)"operator", adhoc_filter->operator_c, sizeof(adhoc_filter->operator_c));
		grafana_get_json_item_str(tmp2, (const char *)"value", adhoc_filter->value, sizeof(adhoc_filter->value));
	}
	cJSON_Delete(root);

	return 0;
}

/* 响应grafana的 search 请求, 返回的指针用完要free */
char *grafana_build_reponse_search()
{
	char *json_str;
	cJSON *json;

	json = cJSON_CreateArray();
	cJSON_AddItemToArray(json, cJSON_CreateString("top"));
	cJSON_AddItemToArray(json, cJSON_CreateString("history"));
	cJSON_AddItemToArray(json, cJSON_CreateString("trend"));
	json_str = cJSON_Print(json);
	cJSON_Delete(json);

	return json_str;
}

/* 响应grafana的 query(top) 请求, 返回的指针用完要free */
static char *grafana_build_reponse_query_top(MYSQL *mysql, const char *request_body, grafana_query_request_s *rst, snetflow_job_s *job)
{
	int i, ret;
	char *out, s_time[128], e_time[128], *tag, col_name[32];
	time_t start_time, end_time;
	map<string, uint64_t> top_map;
	map<string, uint64_t>::iterator it;
	string s;
	grafana_query_request_s query_rst;
	cJSON *root, *response_json, *columns, *column, *rows, *row, *json_tag, *json_bytes;
	cJSON *prev;
	time_t times, timee;
	

	if(grafana_query_structured(request_body, &query_rst) != 0)
	{
		return NULL;
	}
	grafana_get_time_from_request(&query_rst, s_time, e_time, sizeof(s_time));
	tag = query_rst.targets->data.additional;
	if(tag == NULL)
	{
		return NULL;
	}
	start_time = timestr_to_stamp(s_time);
	end_time = timestr_to_stamp(e_time);
	job->start_time = start_time;
	job->end_time = end_time;
	/* 根据target拼接json */
	response_json = cJSON_CreateArray();
	root = cJSON_CreateObject();
	cJSON_AddItemToArray(response_json, root);
	rows = cJSON_AddArrayToObject(root, "rows");
	columns = cJSON_AddArrayToObject(root, "columns");
	cJSON_AddStringToObject(root, "type", "table");
	/* rows */
	memset(col_name, 0, sizeof(col_name));
	if(!strcasecmp(tag, "src_set"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_SRC_SET, (void *)&top_map);
		strcpy(col_name, "src_set");
	}
	else if(!strcasecmp(tag, "dst_set"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_DST_SET, (void *)&top_map);
		strcpy(col_name, "dst_set");
	}
	else if(!strcasecmp(tag, "src_biz"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_SRC_BIZ, (void *)&top_map);
		strcpy(col_name, "src_biz");
	}
	else if(!strcasecmp(tag, "dst_biz"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_DST_BIZ, (void *)&top_map);
		strcpy(col_name, "dst_biz");
	}
	else if(!strcasecmp(tag, "HXQ_in"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_DST_HXQ, (void *)&top_map);
		strcpy(col_name, "HXQ_in");
	}
	else if(!strcasecmp(tag, "HXQ_out"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_SRC_HXQ, (void *)&top_map);
		strcpy(col_name, "HXQ_out");
	}
	else if(!strcasecmp(tag, "HXQ_ZB_in"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_DST_HXQ_ZB, (void *)&top_map);
		strcpy(col_name, "HXQ_ZB_in");
	}
	else if(!strcasecmp(tag, "HXQ_ZB_out"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_SRC_HXQ_ZB, (void *)&top_map);
		strcpy(col_name, "HXQ_ZB_out");
	}
	else if(!strcasecmp(tag, "HXQ_SF_in"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_DST_HXQ_SF, (void *)&top_map);
		strcpy(col_name, "HXQ_SF_in");
	}
	else if(!strcasecmp(tag, "HXQ_SF_out"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_SRC_HXQ_SF, (void *)&top_map);
		strcpy(col_name, "HXQ_SF_out");
	}
	else if(!strcasecmp(tag, "GLQ_in"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_DST_GLQ, (void *)&top_map);
		strcpy(col_name, "GLQ_in");
	}
	else if(!strcasecmp(tag, "GLQ_out"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_SRC_GLQ, (void *)&top_map);
		strcpy(col_name, "GLQ_out");
	}
	else if(!strcasecmp(tag, "RZQ_in"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_DST_RZQ, (void *)&top_map);
		strcpy(col_name, "RZQ_in");
	}
	else if(!strcasecmp(tag, "RZQ_out"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_SRC_RZQ, (void *)&top_map);
		strcpy(col_name, "RZQ_out");
	}
	else if(!strcasecmp(tag, "JRQ_in"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_DST_JRQ, (void *)&top_map);
		strcpy(col_name, "JRQ_in");
	}
	else if(!strcasecmp(tag, "JRQ_out"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_SRC_JRQ, (void *)&top_map);
		strcpy(col_name, "JRQ_out");
	}
	else if(!strcasecmp(tag, "CSQ_in"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_DST_CSQ, (void *)&top_map);
		strcpy(col_name, "CSQ_in");
	}
	else if(!strcasecmp(tag, "CSQ_out"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_SRC_CSQ, (void *)&top_map);
		strcpy(col_name, "CSQ_out");
	}
	else if(!strcasecmp(tag, "DMZ_in"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_DST_DMZ, (void *)&top_map);
		strcpy(col_name, "DMZ_in");
	}
	else if(!strcasecmp(tag, "DMZ_out"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_SRC_DMZ, (void *)&top_map);
		strcpy(col_name, "DMZ_out");
	}
	else if(!strcasecmp(tag, "ALQ_in"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_DST_ALQ, (void *)&top_map);
		strcpy(col_name, "ALQ_in");
	}
	else if(!strcasecmp(tag, "ALQ_out"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_SRC_ALQ, (void *)&top_map);
		strcpy(col_name, "ALQ_out");
	}
	else if(!strcasecmp(tag, "WBQ_in"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_DST_WBQ, (void *)&top_map);
		strcpy(col_name, "WBQ_in");
	}
	else if(!strcasecmp(tag, "WBQ_out"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_SRC_WBQ, (void *)&top_map);
		strcpy(col_name, "WBQ_out");
	}
	else if(!strcasecmp(tag, "UNKNOWN_in"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_DST_UNKNOWN, (void *)&top_map);
		strcpy(col_name, "UNKNOWN_in");
	}
	else if(!strcasecmp(tag, "UNKNOWN_out"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_SRC_UNKNOWN, (void *)&top_map);
		strcpy(col_name, "UNKNOWN_out");
	}
	else
	{
		return NULL;
	}
	if(ret != 0)
	{
		return NULL;
	}
	myprintf("Map Size:%lu\n", top_map.size());
	time(&times);
	for(i = 0, it = top_map.begin(); it != top_map.end(); it++)  
	{
		s = it->first;
		json_tag = cJSON_CreateString(s.c_str());
		json_bytes = cJSON_CreateNumber(it->second);
		row = cJSON_CreateArray();
	    cJSON_AddItemToArray(row, json_tag);
	    cJSON_AddItemToArray(row, json_bytes);
		/* 优化cJSON_AddItemToArray(rows, row)提高速度*/ 
		if(i == 0)
		{
			i = 1;
			cJSON_AddItemToArray(rows, row);
			prev = row;
		}
		else
		{
			row->prev = prev;
			prev->next = row;
			prev = row;
		}
	}
	/* columns */
	column = cJSON_CreateObject();
	cJSON_AddStringToObject(column, "text", col_name);
    cJSON_AddStringToObject(column, "type", "string");
	cJSON_AddItemToArray(columns, column);
    column = cJSON_CreateObject();
	cJSON_AddStringToObject(column, "text", "bytes");
    cJSON_AddStringToObject(column, "type", "number");
	cJSON_AddItemToArray(columns, column);
	time(&timee);
	myprintf("[Cost %ld seconds]:Create Json.\n", timee - times);
	/* 释放空间 */
	out = cJSON_Print(response_json);
	cJSON_Delete(response_json);
	
	return out;
}

/* 响应grafana的 query(history) 请求, 返回的指针用完要free */
static char *grafana_build_reponse_query_history(MYSQL *mysql, const char *request_body, grafana_query_request_s *rst, snetflow_job_s *job)
{
	int i, ret;
	char *out, s_time[128], e_time[128], *tag, col_name[32];
	time_t start_time, end_time;
	vector<history_s> history_vec;
	vector<history_s>::iterator it;
	cJSON *root, *response_json, *columns, *column, *rows, *row, *json_flow, *json_bytes;
	cJSON *prev;
	time_t times, timee;
	
	grafana_get_time_from_request(rst, s_time, e_time, sizeof(s_time));
	tag = rst->targets->data.additional;
	if(tag == NULL)
	{
		return NULL;
	}
	start_time = timestr_to_stamp(s_time);
	end_time = timestr_to_stamp(e_time);
	job->start_time = start_time;
	job->end_time = end_time;
	memset(col_name, 0, sizeof(col_name));
	if(!strcasecmp(tag, "HXQ_in"))
	{
		ret = get_history(mysql, start_time, end_time, HISTORY_DST_HXQ, (void *)&history_vec);
	}
	else if(!strcasecmp(tag, "HXQ_out"))
	{
		ret = get_history(mysql, start_time, end_time, HISTORY_SRC_HXQ, (void *)&history_vec);
	}
	else if(!strcasecmp(tag, "HXQ_ZB_in"))
	{
		ret = get_history(mysql, start_time, end_time, HISTORY_DST_HXQ_ZB, (void *)&history_vec);
	}
	else if(!strcasecmp(tag, "HXQ_ZB_out"))
	{
		ret = get_history(mysql, start_time, end_time, HISTORY_SRC_HXQ_ZB, (void *)&history_vec);
	}
	else if(!strcasecmp(tag, "HXQ_SF_in"))
	{
		ret = get_history(mysql, start_time, end_time, HISTORY_DST_HXQ_SF, (void *)&history_vec);
	}
	else if(!strcasecmp(tag, "HXQ_SF_out"))
	{
		ret = get_history(mysql, start_time, end_time, HISTORY_SRC_HXQ_SF, (void *)&history_vec);
	}
	else if(!strcasecmp(tag, "GLQ_in"))
	{
		ret = get_history(mysql, start_time, end_time, HISTORY_DST_GLQ, (void *)&history_vec);
	}
	else if(!strcasecmp(tag, "GLQ_out"))
	{
		ret = get_history(mysql, start_time, end_time, HISTORY_SRC_GLQ, (void *)&history_vec);
	}
	else if(!strcasecmp(tag, "RZQ_in"))
	{
		ret = get_history(mysql, start_time, end_time, HISTORY_DST_RZQ, (void *)&history_vec);
	}
	else if(!strcasecmp(tag, "RZQ_out"))
	{
		ret = get_history(mysql, start_time, end_time, HISTORY_SRC_RZQ, (void *)&history_vec);
	}
	else if(!strcasecmp(tag, "JRQ_in"))
	{
		ret = get_history(mysql, start_time, end_time, HISTORY_DST_JRQ, (void *)&history_vec);
	}
	else if(!strcasecmp(tag, "JRQ_out"))
	{
		ret = get_history(mysql, start_time, end_time, HISTORY_SRC_JRQ, (void *)&history_vec);
	}
	else if(!strcasecmp(tag, "CSQ_in"))
	{
		ret = get_history(mysql, start_time, end_time, HISTORY_DST_CSQ, (void *)&history_vec);
	}
	else if(!strcasecmp(tag, "CSQ_out"))
	{
		ret = get_history(mysql, start_time, end_time, HISTORY_SRC_CSQ, (void *)&history_vec);
	}
	else if(!strcasecmp(tag, "DMZ_in"))
	{
		ret = get_history(mysql, start_time, end_time, HISTORY_DST_DMZ, (void *)&history_vec);
	}
	else if(!strcasecmp(tag, "DMZ_out"))
	{
		ret = get_history(mysql, start_time, end_time, HISTORY_SRC_DMZ, (void *)&history_vec);
	}
	else if(!strcasecmp(tag, "ALQ_in"))
	{
		ret = get_history(mysql, start_time, end_time, HISTORY_DST_ALQ, (void *)&history_vec);
	}
	else if(!strcasecmp(tag, "ALQ_out"))
	{
		ret = get_history(mysql, start_time, end_time, HISTORY_SRC_ALQ, (void *)&history_vec);
	}
	else if(!strcasecmp(tag, "WBQ_in"))
	{
		ret = get_history(mysql, start_time, end_time, HISTORY_DST_WBQ, (void *)&history_vec);
	}
	else if(!strcasecmp(tag, "WBQ_out"))
	{
		ret = get_history(mysql, start_time, end_time, HISTORY_SRC_WBQ, (void *)&history_vec);
	}
	else if(!strcasecmp(tag, "UNKNOWN_in"))
	{
		ret = get_history(mysql, start_time, end_time, HISTORY_DST_UNKNOWN, (void *)&history_vec);
	}
	else if(!strcasecmp(tag, "UNKNOWN_out"))
	{
		ret = get_history(mysql, start_time, end_time, HISTORY_SRC_UNKNOWN, (void *)&history_vec);
	}
	else
	{
		return NULL;
	}
	if(ret != 0)
	{
		return NULL;
	}
	myprintf("Vector Size:%lu\n", history_vec.size());
	time(&times);
	/* 根据target拼接json */
	response_json = cJSON_CreateArray();
	root = cJSON_CreateObject();
	cJSON_AddItemToArray(response_json, root);
	rows = cJSON_AddArrayToObject(root, "rows");
	columns = cJSON_AddArrayToObject(root, "columns");
	cJSON_AddStringToObject(root, "type", "table");
	/* rows */
	for(i = 0, it = history_vec.begin(); it != history_vec.end(); it++)  
	{
		json_flow = cJSON_CreateString((*it).flow);
		json_bytes = cJSON_CreateNumber((*it).bytes);
		row = cJSON_CreateArray();
	    cJSON_AddItemToArray(row, json_flow);
	    cJSON_AddItemToArray(row, json_bytes);
		/* 优化cJSON_AddItemToArray(rows, row)提高速度*/ 
		if(i == 0)
		{
			i = 1;
			cJSON_AddItemToArray(rows, row);
			prev = row;
		}
		else
		{
			row->prev = prev;
			prev->next = row;
			prev = row;
		}
	}
	/* columns */
	column = cJSON_CreateObject();
	cJSON_AddStringToObject(column, "text", "flow");
    cJSON_AddStringToObject(column, "type", "string");
	cJSON_AddItemToArray(columns, column);
    column = cJSON_CreateObject();
	cJSON_AddStringToObject(column, "text", "bytes");
    cJSON_AddStringToObject(column, "type", "number");
	cJSON_AddItemToArray(columns, column);
	time(&timee);
	myprintf("[Cost %ld seconds]:Create Json.\n", timee - times);
	/* 释放空间 */
	out = cJSON_Print(response_json);
	cJSON_Delete(response_json);
	
	return out;
}

/* 响应grafana的 query(trend) 请求, 返回的指针用完要free */
static char *grafana_build_reponse_query_trend(MYSQL *mysql, const char *request_body, grafana_query_request_s *rst, snetflow_job_s *job)
{
	int i, ret;
	char *out, s_time[128], e_time[128], *tag;
	time_t start_time, end_time;
	map<uint64_t, uint64_t> trend_map;
	map<uint64_t, uint64_t>::iterator it;
	cJSON *root, *response_json, *datapoints, *datapoint, *json_time, *json_bytes;
	cJSON *prev;
	time_t times, timee;
	
	grafana_get_time_from_request(rst, s_time, e_time, sizeof(s_time));
	tag = rst->targets->data.additional;
	if(tag == NULL)
	{
		return NULL;
	}
	start_time = timestr_to_stamp(s_time);
	end_time = timestr_to_stamp(e_time);
	job->start_time = start_time;
	job->end_time = end_time;
	if(!strcasecmp(tag, "HXQ_in"))
	{
		ret = get_trend(mysql, start_time, end_time, TREND_DST_HXQ, (void *)&trend_map);
	}
	else if(!strcasecmp(tag, "HXQ_out"))
	{
		ret = get_trend(mysql, start_time, end_time, TREND_SRC_HXQ, (void *)&trend_map);
	}
	else if(!strcasecmp(tag, "HXQ_ZB_in"))
	{
		ret = get_trend(mysql, start_time, end_time, TREND_DST_HXQ_ZB, (void *)&trend_map);
	}
	else if(!strcasecmp(tag, "HXQ_ZB_out"))
	{
		ret = get_trend(mysql, start_time, end_time, TREND_SRC_HXQ_ZB, (void *)&trend_map);
	}
	else if(!strcasecmp(tag, "HXQ_SF_in"))
	{
		ret = get_trend(mysql, start_time, end_time, TREND_DST_HXQ_SF, (void *)&trend_map);
	}
	else if(!strcasecmp(tag, "HXQ_SF_out"))
	{
		ret = get_trend(mysql, start_time, end_time, TREND_SRC_HXQ_SF, (void *)&trend_map);
	}
	else if(!strcasecmp(tag, "GLQ_in"))
	{
		ret = get_trend(mysql, start_time, end_time, TREND_DST_GLQ, (void *)&trend_map);
	}
	else if(!strcasecmp(tag, "GLQ_out"))
	{
		ret = get_trend(mysql, start_time, end_time, TREND_SRC_GLQ, (void *)&trend_map);
	}
	else if(!strcasecmp(tag, "RZQ_in"))
	{
		ret = get_trend(mysql, start_time, end_time, TREND_DST_RZQ, (void *)&trend_map);
	}
	else if(!strcasecmp(tag, "RZQ_out"))
	{
		ret = get_trend(mysql, start_time, end_time, TREND_SRC_RZQ, (void *)&trend_map);
	}
	else if(!strcasecmp(tag, "JRQ_in"))
	{
		ret = get_trend(mysql, start_time, end_time, TREND_DST_JRQ, (void *)&trend_map);
	}
	else if(!strcasecmp(tag, "JRQ_out"))
	{
		ret = get_trend(mysql, start_time, end_time, TREND_SRC_JRQ, (void *)&trend_map);
	}
	else if(!strcasecmp(tag, "CSQ_in"))
	{
		ret = get_trend(mysql, start_time, end_time, TREND_DST_CSQ, (void *)&trend_map);
	}
	else if(!strcasecmp(tag, "CSQ_out"))
	{
		ret = get_trend(mysql, start_time, end_time, TREND_SRC_CSQ, (void *)&trend_map);
	}
	else if(!strcasecmp(tag, "DMZ_in"))
	{
		ret = get_trend(mysql, start_time, end_time, TREND_DST_DMZ, (void *)&trend_map);
	}
	else if(!strcasecmp(tag, "DMZ_out"))
	{
		ret = get_trend(mysql, start_time, end_time, TREND_SRC_DMZ, (void *)&trend_map);
	}
	else if(!strcasecmp(tag, "ALQ_in"))
	{
		ret = get_trend(mysql, start_time, end_time, TREND_DST_ALQ, (void *)&trend_map);
	}
	else if(!strcasecmp(tag, "ALQ_out"))
	{
		ret = get_trend(mysql, start_time, end_time, TREND_SRC_ALQ, (void *)&trend_map);
	}
	else if(!strcasecmp(tag, "WBQ_in"))
	{
		ret = get_trend(mysql, start_time, end_time, TREND_DST_WBQ, (void *)&trend_map);
	}
	else if(!strcasecmp(tag, "WBQ_out"))
	{
		ret = get_trend(mysql, start_time, end_time, TREND_SRC_WBQ, (void *)&trend_map);
	}
	else if(!strcasecmp(tag, "UNKNOWN_in"))
	{
		ret = get_trend(mysql, start_time, end_time, TREND_DST_UNKNOWN, (void *)&trend_map);
	}
	else if(!strcasecmp(tag, "UNKNOWN_out"))
	{
		ret = get_trend(mysql, start_time, end_time, TREND_SRC_UNKNOWN, (void *)&trend_map);
	}
	else
	{
		return NULL;
	}
	if(ret != 0)
	{
		return NULL;
	}
	myprintf("Map Size:%lu\n", trend_map.size());
	time(&times);
	/* 根据target拼接json */
	response_json = cJSON_CreateArray();
	root = cJSON_CreateObject();
	cJSON_AddItemToArray(response_json, root);
	cJSON_AddStringToObject(root, "target", "trend");
	datapoints = cJSON_AddArrayToObject(root, "datapoints");
	/* datapoints */
	for(i = 0, it = trend_map.begin(); it != trend_map.end(); it++)  
	{
		json_time = cJSON_CreateNumber(it->first * 1000);
		json_bytes = cJSON_CreateNumber(it->second);
		datapoint = cJSON_CreateArray();
	    cJSON_AddItemToArray(datapoint, json_bytes);
	    cJSON_AddItemToArray(datapoint, json_time);
		/* 优化cJSON_AddItemToArray(rows, row)提高速度*/ 
		if(i == 0)
		{
			i = 1;
			cJSON_AddItemToArray(datapoints, datapoint);
			prev = datapoint;
		}
		else
		{
			datapoint->prev = prev;
			prev->next = datapoint;
			prev = datapoint;
		}
	}
	time(&timee);
	myprintf("[Cost %ld seconds]:Create Json.\n", timee - times);
	/* 释放空间 */
	out = cJSON_Print(response_json);
	cJSON_Delete(response_json);
	
	return out;
}

/* 响应grafana的 query 请求, 返回的指针用完要free */
char *grafana_build_reponse_query(MYSQL *mysql, const char *request_body, snetflow_job_s *job)
{
	char *target;
	grafana_query_request_s query_rst;

	if(grafana_query_structured(request_body, &query_rst) != 0)
	{
		return NULL;
	}
	
	target = query_rst.targets->target;
	if(target == NULL)
	{
		return NULL;
	}
	else if(strcasecmp(target, "top") == 0)
	{
		return grafana_build_reponse_query_top(mysql, request_body, &query_rst, job);
	}
	else if(strcasecmp(target, "history") == 0)
	{
		return grafana_build_reponse_query_history(mysql, request_body, &query_rst, job);
	}
	else if(strcasecmp(target, "trend") == 0)
	{
		return grafana_build_reponse_query_trend(mysql, request_body, &query_rst, job);
	}

	return NULL;
}

