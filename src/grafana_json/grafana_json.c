#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <cjson/cJSON.h>
#include <mysql/mysql.h>

#include "common.h"
#include "snetflow_top.h"
#include "grafana_json.h"

using namespace std;

static grafana_query_request_s query_rst;

static char *grafana_get_json_item_str(cJSON *obj, const char *tag)
{
	cJSON *item;
	
	item = cJSON_GetObjectItem(obj, tag);
	if(!item)
	{
		return NULL;
	}

	return item->valuestring;
}

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

static int grafana_query_structured(const char *request_body, grafana_query_request_s *rst)
{
	int i, count;
	cJSON *root, *tmp1, *tmp2, *tmp3;
	char *str_value;
	grafana_query_request_target_s *target, *target_tmp;
	grafana_query_request_adhoc_filter_s *adhoc_filter, *adhoc_filter_tmp;
	
	root = cJSON_Parse(request_body);
	if(!root)
	{
		return -1;
	}
	/* 解析之前先释放之前占用的空间 */
	grafana_query_free(rst);
    /* 解析grafana的query    request，解析结果写入结构体 */
	rst->panelid = cJSON_GetObjectItem(root, "panelId")->valueint;
	tmp1 = cJSON_GetObjectItem(root, "range");
	tmp2 = cJSON_GetObjectItem(tmp1, "raw");
	str_value = grafana_get_json_item_str(tmp1, (const char *)"from");
	if(!str_value)
	{
		return -1;
	}
	strcpy(rst->range.from, str_value);
	
	str_value = grafana_get_json_item_str(tmp1, (const char *)"to");
	if(!str_value)
	{
		return -1;
	}
	strcpy(rst->range.to, str_value);
	
	str_value = grafana_get_json_item_str(tmp2, (const char *)"from");
	if(!str_value)
	{
		return -1;
	}
	strcpy(rst->range.raw.from, str_value);
	
	str_value = grafana_get_json_item_str(tmp2, (const char *)"to");
	if(!str_value)
	{
		return -1;
	}
	strcpy(rst->range.raw.to, str_value);
	
	tmp1 = cJSON_GetObjectItem(root, "rangeRaw");
	str_value = grafana_get_json_item_str(tmp1, (const char *)"from");
	if(!str_value)
	{
		return -1;
	}
	strcpy(rst->range_raw.from, str_value);
	
	str_value = grafana_get_json_item_str(tmp1, (const char *)"to");
	if(!str_value)
	{
		return -1;
	}
	strcpy(rst->range_raw.to, str_value);
	
	str_value = grafana_get_json_item_str(root, (const char *)"interval");
	if(!str_value)
	{
		return -1;
	}
	strcpy(rst->interval, str_value);
	
	rst->interval_ms = cJSON_GetObjectItem(root, "intervalMs")->valueint;
	rst->max_data_points = cJSON_GetObjectItem(root, "maxDataPoints")->valueint;
	/* 解析targets数组 */
	tmp1 = cJSON_GetObjectItem(root, "targets");
	count = cJSON_GetArraySize(tmp1);
	for(i = 0; i < count; i++)
	{
		tmp2 = cJSON_GetArrayItem(tmp1, i);
		target = (grafana_query_request_target_s *)malloc(sizeof(grafana_query_request_target_s));
		memset(target, 0, sizeof(grafana_query_request_target_s));
		str_value = grafana_get_json_item_str(tmp2, (const char *)"target");
		if(!str_value)
		{
			return -1;
		}
		strcpy(target->target, str_value);
		
		str_value = grafana_get_json_item_str(tmp2, (const char *)"refId");
		if(!str_value)
		{
			return -1;
		}
		strcpy(target->refid, str_value);
		
		str_value = grafana_get_json_item_str(tmp2, (const char *)"type");
		if(!str_value)
		{
			return -1;
		}
		strcpy(target->type, str_value);
		
		tmp3 = cJSON_GetObjectItem(tmp2, "data");
		if(tmp3)
		{
			str_value = grafana_get_json_item_str(tmp3, (const char *)"additional");
			if(!str_value)
			{
				return -1;
			}
			strcpy(target->data.additional, str_value);
		}
		if(i == 0)
		{
			rst->targets = target;
			
		}
		else
		{
			target_tmp->next = target;
		}
		target_tmp = target;
	}
	/* 解析adhoc_filters数组 */
	tmp1 = cJSON_GetObjectItem(root, "adhocFilters");
	count = cJSON_GetArraySize(tmp1);
	for(i = 0; i < count; i++)
	{
		tmp2 = cJSON_GetArrayItem(tmp1, i);
		adhoc_filter = (grafana_query_request_adhoc_filter_s *)malloc(sizeof(grafana_query_request_adhoc_filter_s));
		memset(adhoc_filter, 0, sizeof(grafana_query_request_adhoc_filter_s));
		str_value = grafana_get_json_item_str(tmp2, (const char *)"key");
		if(!str_value)
		{
			return -1;
		}
		strcpy(adhoc_filter->key, str_value);
		
		str_value = grafana_get_json_item_str(tmp2, (const char *)"operator");
		if(!str_value)
		{
			return -1;
		}
		strcpy(adhoc_filter->operator_c, str_value);
		
		str_value = grafana_get_json_item_str(tmp2, (const char *)"value");
		if(!str_value)
		{
			return -1;
		}
		strcpy(adhoc_filter->value, str_value);
		if(i == 0)
		{
			rst->adhoc_filters = adhoc_filter;
		}
		else
		{
			adhoc_filter_tmp->next = adhoc_filter;
		}
		adhoc_filter_tmp = adhoc_filter;
	}
	cJSON_Delete(root);

	return 0;
}

char *grafana_build_reponse_search_top()
{
	char *json_str;
	cJSON *json;

	json = cJSON_CreateArray();
	cJSON_AddItemToArray(json, cJSON_CreateString("flow"));
	cJSON_AddItemToArray(json, cJSON_CreateString("src_set"));
	cJSON_AddItemToArray(json, cJSON_CreateString("dst_set"));
	cJSON_AddItemToArray(json, cJSON_CreateString("src_biz"));
	cJSON_AddItemToArray(json, cJSON_CreateString("dst_biz"));
	json_str = cJSON_Print(json);
	cJSON_Delete(json);

	return json_str;
}

char *grafana_build_reponse_query_top(MYSQL *mysql, const char *request_body, snetflow_job_s *job)
{
	int i, ret;
	char *out, s_time[128], e_time[128], *tag, col_name[32];
	time_t start_time, end_time;
	map<string, uint64_t> top_map;
	map<string, uint64_t>::iterator it;
	string s;
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
		ret = get_top(mysql, start_time, end_time, TOP_SRC_HXQ, (void *)&top_map);
		strcpy(col_name, "HXQ_in");
	}
	else if(!strcasecmp(tag, "HXQ_out"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_DST_HXQ, (void *)&top_map);
		strcpy(col_name, "HXQ_out");
	}
	else if(!strcasecmp(tag, "HXQ_ZB_in"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_SRC_HXQ_ZB, (void *)&top_map);
		strcpy(col_name, "HXQ_ZB_in");
	}
	else if(!strcasecmp(tag, "HXQ_ZB_out"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_DST_HXQ_ZB, (void *)&top_map);
		strcpy(col_name, "HXQ_ZB_out");
	}
	else if(!strcasecmp(tag, "HXQ_SF_in"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_SRC_HXQ_SF, (void *)&top_map);
		strcpy(col_name, "HXQ_SF_in");
	}
	else if(!strcasecmp(tag, "HXQ_SF_out"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_DST_HXQ_SF, (void *)&top_map);
		strcpy(col_name, "HXQ_SF_out");
	}
	else if(!strcasecmp(tag, "GLQ_in"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_SRC_GLQ, (void *)&top_map);
		strcpy(col_name, "GLQ_in");
	}
	else if(!strcasecmp(tag, "GLQ_out"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_DST_GLQ, (void *)&top_map);
		strcpy(col_name, "GLQ_out");
	}
	else if(!strcasecmp(tag, "RZQ_in"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_SRC_RZQ, (void *)&top_map);
		strcpy(col_name, "RZQ_in");
	}
	else if(!strcasecmp(tag, "RZQ_out"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_DST_RZQ, (void *)&top_map);
		strcpy(col_name, "RZQ_out");
	}
	else if(!strcasecmp(tag, "JRQ_in"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_SRC_JRQ, (void *)&top_map);
		strcpy(col_name, "JRQ_in");
	}
	else if(!strcasecmp(tag, "JRQ_out"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_DST_JRQ, (void *)&top_map);
		strcpy(col_name, "JRQ_out");
	}
	else if(!strcasecmp(tag, "CSQ_in"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_SRC_CSQ, (void *)&top_map);
		strcpy(col_name, "CSQ_in");
	}
	else if(!strcasecmp(tag, "CSQ_out"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_DST_CSQ, (void *)&top_map);
		strcpy(col_name, "CSQ_out");
	}
	else if(!strcasecmp(tag, "DMZ_in"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_SRC_DMZ, (void *)&top_map);
		strcpy(col_name, "DMZ_in");
	}
	else if(!strcasecmp(tag, "DMZ_out"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_DST_DMZ, (void *)&top_map);
		strcpy(col_name, "DMZ_out");
	}
	else if(!strcasecmp(tag, "ALQ_in"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_SRC_ALQ, (void *)&top_map);
		strcpy(col_name, "ALQ_in");
	}
	else if(!strcasecmp(tag, "ALQ_out"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_DST_ALQ, (void *)&top_map);
		strcpy(col_name, "ALQ_out");
	}
	else if(!strcasecmp(tag, "WBQ_in"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_SRC_WBQ, (void *)&top_map);
		strcpy(col_name, "WBQ_in");
	}
	else if(!strcasecmp(tag, "WBQ_out"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_DST_WBQ, (void *)&top_map);
		strcpy(col_name, "WBQ_out");
	}
	else if(!strcasecmp(tag, "UNKNOWN_in"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_SRC_UNKNOWN, (void *)&top_map);
		strcpy(col_name, "UNKNOWN_in");
	}
	else if(!strcasecmp(tag, "UNKNOWN_out"))
	{
		ret = get_top(mysql, start_time, end_time, TOP_DST_UNKNOWN, (void *)&top_map);
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
