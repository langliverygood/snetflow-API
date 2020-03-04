#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <cjson/cJSON.h>
#include <mysql/mysql.h>

#include "common.h"
#include "config.h"
#include "snetflow_top.h"
#include "snetflow_history.h"
#include "snetflow_trend.h"
#include "snetflow_warning.h"
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
	cJSON_AddItemToArray(json, cJSON_CreateString("warning"));
	json_str = cJSON_Print(json);
	cJSON_Delete(json);

	return json_str;
}

/* 响应grafana的 query(top) 请求, 返回的指针用完要free */
static char *grafana_build_reponse_query_top(MYSQL *mysql, const char *request_body, grafana_query_request_s *rst, snetflow_job_s *job)
{
	int i, ret;
	char *out, *tag;
	map<string, uint64_t> top_map;
	map<string, uint64_t>::iterator it;
	string s;
	mysql_conf_s *cfg;
	cJSON *root, *response_json, *columns, *column, *rows, *row, *json_tag, *json_bytes;
	cJSON *prev;
	
	tag = rst->targets->data.additional;
	if(tag == NULL)
	{
		return NULL;
	}
	cfg = get_config(tag, TOP);
	if(cfg == NULL)
	{
		return NULL;
	}
	ret = get_top(mysql, job->start_time, job->end_time, cfg, (void *)&top_map);
	if(ret != 0)
	{
		return NULL;
	}
	/* 根据target拼接json */
	response_json = cJSON_CreateArray();
	root = cJSON_CreateObject();
	cJSON_AddItemToArray(response_json, root);
	rows = cJSON_AddArrayToObject(root, "rows");
	columns = cJSON_AddArrayToObject(root, "columns");
	cJSON_AddStringToObject(root, "type", "table");
	/* rows */
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
	cJSON_AddStringToObject(column, "text", cfg->name);
    cJSON_AddStringToObject(column, "type", "string");
	cJSON_AddItemToArray(columns, column);
    column = cJSON_CreateObject();
	cJSON_AddStringToObject(column, "text", "bytes");
    cJSON_AddStringToObject(column, "type", "number");
	cJSON_AddItemToArray(columns, column);
	/* 释放空间 */
	out = cJSON_Print(response_json);
	cJSON_Delete(response_json);
	
	return out;
}

/* 响应grafana的 query(trend) 请求, 返回的指针用完要free */
static char *grafana_build_reponse_query_trend(MYSQL *mysql, const char *request_body, grafana_query_request_s *rst, snetflow_job_s *job)
{
	int i, ret;
	char *out, *tag;
	mysql_conf_s *cfg;
	map<uint64_t, uint64_t> trend_map;
	map<uint64_t, uint64_t>::iterator it;
	cJSON *root, *response_json, *datapoints, *datapoint, *json_time, *json_bytes;
	cJSON *prev;

	tag = rst->targets->data.additional;
	if(tag == NULL)
	{
		return NULL;
	}
	cfg = get_config(tag, TREND);
	if(cfg == NULL)
	{
		return NULL;
	}
	ret = get_trend(mysql, job->start_time, job->end_time, cfg, (void *)&trend_map);
	if(ret != 0)
	{
		return NULL;
	}
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
	/* 释放空间 */
	out = cJSON_Print(response_json);
	cJSON_Delete(response_json);
	
	return out;
}

/* 响应grafana的 query(history) 请求, 返回的指针用完要free */
static char *grafana_build_reponse_query_history(MYSQL *mysql, const char *request_body, grafana_query_request_s *rst, snetflow_job_s *job)
{
	int i, ret;
	char *out, *tag;
	mysql_conf_s *cfg;
	vector<history_s> history_vec;
	vector<history_s>::iterator it;
	cJSON *root, *response_json, *columns, *column, *rows, *row, *json_flow, *json_bytes;
	cJSON *prev;
	
	tag = rst->targets->data.additional;
	if(tag == NULL)
	{
		return NULL;
	}
	cfg = get_config(tag, HISTORY);
	if(cfg == NULL)
	{
		return NULL;
	}
	ret = get_history(mysql, job->start_time, job->end_time, cfg, (void *)&history_vec);
	if(ret != 0)
	{
		return NULL;
	}
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
	cJSON_AddStringToObject(column, "text", cfg->name);
    cJSON_AddStringToObject(column, "type", "string");
	cJSON_AddItemToArray(columns, column);
    column = cJSON_CreateObject();
	cJSON_AddStringToObject(column, "text", "bytes");
    cJSON_AddStringToObject(column, "type", "number");
	cJSON_AddItemToArray(columns, column);
	/* 释放空间 */
	out = cJSON_Print(response_json);
	cJSON_Delete(response_json);
	
	return out;
}

/* 响应grafana的 query(warning) 请求, 返回的指针用完要free */
static char *grafana_build_reponse_query_warning(MYSQL *mysql, const char *request_body, grafana_query_request_s *rst, snetflow_job_s *job)
{
	int i, ret;
	char *out, *tag;
	mysql_conf_s *cfg;
	map<uint32_t, warning_msg_s> warning_map;
	map<uint32_t, warning_msg_s>::iterator it;
	cJSON *root, *response_json, *columns, *column, *rows, *row;
	cJSON *ip, *bytes, *biz, *set, *module, *region, *switches, *prev;
	
	tag = rst->targets->data.additional;
	if(tag == NULL)
	{
		return NULL;
	}
	cfg = get_config(tag, WARNING);
	if(cfg == NULL)
	{
		return NULL;
	}
	ret = get_warning(mysql, job->start_time, job->end_time, cfg, (void *)&warning_map);
	if(ret != 0)
	{
		return NULL;
	}
	/* 根据target拼接json */
	response_json = cJSON_CreateArray();
	root = cJSON_CreateObject();
	cJSON_AddItemToArray(response_json, root);
	rows = cJSON_AddArrayToObject(root, "rows");
	columns = cJSON_AddArrayToObject(root, "columns");
	cJSON_AddStringToObject(root, "type", "table");
	/* rows */
	for(i = 0, it = warning_map.begin(); it != warning_map.end(); it++)  
	{
		ip = cJSON_CreateString(it->second.ip);
		bytes = cJSON_CreateNumber(it->second.bytes);
		biz = cJSON_CreateString(it->second.biz);
		set = cJSON_CreateString(it->second.set);
		module = cJSON_CreateString(it->second.module);
		region = cJSON_CreateString(it->second.region);
		switches = cJSON_CreateString(it->second.switches);
		row = cJSON_CreateArray();
	    cJSON_AddItemToArray(row, ip);
	    cJSON_AddItemToArray(row, bytes);
		cJSON_AddItemToArray(row, biz);
		cJSON_AddItemToArray(row, set);
		cJSON_AddItemToArray(row, module);
		cJSON_AddItemToArray(row, region);
		cJSON_AddItemToArray(row, switches);
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
	cJSON_AddStringToObject(column, "text", "ip");
    cJSON_AddStringToObject(column, "type", "string");
	cJSON_AddItemToArray(columns, column);
    column = cJSON_CreateObject();
	cJSON_AddStringToObject(column, "text", "bytes");
    cJSON_AddStringToObject(column, "type", "number");
	cJSON_AddItemToArray(columns, column);
	column = cJSON_CreateObject();
	cJSON_AddStringToObject(column, "text", "biz");
    cJSON_AddStringToObject(column, "type", "string");
	cJSON_AddItemToArray(columns, column);
	column = cJSON_CreateObject();
	cJSON_AddStringToObject(column, "text", "set");
    cJSON_AddStringToObject(column, "type", "string");
	cJSON_AddItemToArray(columns, column);
	column = cJSON_CreateObject();
	cJSON_AddStringToObject(column, "text", "module");
    cJSON_AddStringToObject(column, "type", "string");
	cJSON_AddItemToArray(columns, column);
	column = cJSON_CreateObject();
	cJSON_AddStringToObject(column, "text", "region");
    cJSON_AddStringToObject(column, "type", "string");
	cJSON_AddItemToArray(columns, column);
	column = cJSON_CreateObject();
	cJSON_AddStringToObject(column, "text", "switches");
    cJSON_AddStringToObject(column, "type", "string");
	cJSON_AddItemToArray(columns, column);
	/* 释放空间 */
	out = cJSON_Print(response_json);
	cJSON_Delete(response_json);
	
	return out;
}

/* 响应grafana的 query 请求, 返回的指针用完要free */
char *grafana_build_reponse_query(MYSQL *mysql, const char *request_body, snetflow_job_s *job)
{
	char *target, s_time[128], e_time[128];
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
	grafana_get_time_from_request(&query_rst, s_time, e_time, sizeof(s_time));
	job->start_time = timestr_to_stamp(s_time);
	job->end_time = timestr_to_stamp(e_time);
	timechanged(&(job->start_time));
	timechanged(&(job->end_time));
	timestamp_to_str(job->start_time, s_time, sizeof(s_time));
	timestamp_to_str(job->end_time, e_time, sizeof(e_time));
	myprintf("Start:%s  End:%s\n", s_time, e_time);
	if(strcasecmp(target, "top") == 0)
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
	else if(strcasecmp(target, "warning") == 0)
	{
		return grafana_build_reponse_query_warning(mysql, request_body, &query_rst, job);
	}

	return NULL;
}

