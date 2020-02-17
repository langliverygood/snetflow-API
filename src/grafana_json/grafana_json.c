#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <cjson/cJSON.h>
#include <mysql/mysql.h>

#include "common.h"
#include "snetflow_top.h"
#include "grafana_json.h"

#ifdef __cplusplus
extern "C" {
#endif

static grafana_query_request_s *query_rst;

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

static char *grafana_query_structured(const char *request_body)
{
	int i, count;
	cJSON *root, *tmp1, *tmp2, *tmp3;
	char *str_value;
	grafana_query_request_target_s *target, *target_tmp;
	grafana_query_request_adhoc_filter_s *adhoc_filter, *adhoc_filter_tmp;
	
	root = cJSON_Parse(request_body);
	if(!root)
	{
		return NULL;
	}
	query_rst = (grafana_query_request_s *)malloc(sizeof(grafana_query_request_s));
	if(!query_rst)
	{
		return NULL;
	}
	memset(query_rst, 0, sizeof(grafana_query_request_s));
    /* 解析grafana的query    request，解析结果写入结构体 */
	query_rst->panelid = cJSON_GetObjectItem(root, "panelId")->valueint;
	tmp1 = cJSON_GetObjectItem(root, "range");
	tmp2 = cJSON_GetObjectItem(tmp1, "raw");
	
	str_value = grafana_get_json_item_str(tmp1, (const char *)"from");
	if(!str_value)
	{
		return NULL;
	}
	strcpy(query_rst->range.from, str_value);
	
	str_value = grafana_get_json_item_str(tmp1, (const char *)"to");
	if(!str_value)
	{
		return NULL;
	}
	strcpy(query_rst->range.to, str_value);
	
	str_value = grafana_get_json_item_str(tmp2, (const char *)"from");
	if(!str_value)
	{
		return NULL;
	}
	strcpy(query_rst->range.raw.from, str_value);
	
	str_value = grafana_get_json_item_str(tmp2, (const char *)"to");
	if(!str_value)
	{
		return NULL;
	}
	strcpy(query_rst->range.raw.to, str_value);
	
	tmp1 = cJSON_GetObjectItem(root, "rangeRaw");
	str_value = grafana_get_json_item_str(tmp1, (const char *)"from");
	if(!str_value)
	{
		return NULL;
	}
	strcpy(query_rst->range_raw.from, str_value);
	
	str_value = grafana_get_json_item_str(tmp1, (const char *)"to");
	if(!str_value)
	{
		return NULL;
	}
	strcpy(query_rst->range_raw.to, str_value);
	
	str_value = grafana_get_json_item_str(root, (const char *)"interval");
	if(!str_value)
	{
		return NULL;
	}
	strcpy(query_rst->interval, str_value);
	
	query_rst->interval_ms = cJSON_GetObjectItem(root, "intervalMs")->valueint;
	query_rst->max_data_points = cJSON_GetObjectItem(root, "maxDataPoints")->valueint;
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
			return NULL;
		}
		strcpy(target->target, str_value);
		
		str_value = grafana_get_json_item_str(tmp2, (const char *)"refId");
		if(!str_value)
		{
			return NULL;
		}
		strcpy(target->refid, str_value);
		
		str_value = grafana_get_json_item_str(tmp2, (const char *)"type");
		if(!str_value)
		{
			return NULL;
		}
		strcpy(target->type, str_value);
		
		tmp3 = cJSON_GetObjectItem(tmp2, "data");
		if(tmp3)
		{
			str_value = grafana_get_json_item_str(tmp3, (const char *)"additional");
			if(!str_value)
			{
				return NULL;
			}
			strcpy(target->data.additional, str_value);
		}
		if(i == 0)
		{
			query_rst->targets = target;
			
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
			return NULL;
		}
		strcpy(adhoc_filter->key, str_value);
		
		str_value = grafana_get_json_item_str(tmp2, (const char *)"operator");
		if(!str_value)
		{
			return NULL;
		}
		strcpy(adhoc_filter->operator_c, str_value);
		
		str_value = grafana_get_json_item_str(tmp2, (const char *)"value");
		if(!str_value)
		{
			return NULL;
		}
		strcpy(adhoc_filter->value, str_value);
		if(i == 0)
		{
			query_rst->adhoc_filters = adhoc_filter;
		}
		else
		{
			adhoc_filter_tmp->next = adhoc_filter;
		}
		adhoc_filter_tmp = adhoc_filter;
	}
	cJSON_Delete(root);

	return (char *)query_rst;
}

static void grafana_query_free()
{
	grafana_query_request_target_s *target, *target_next;
	grafana_query_request_adhoc_filter_s *adhoc_filter, *adhoc_filter_next;
	if(query_rst)
	{
		target = query_rst->targets;
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
		adhoc_filter = query_rst->adhoc_filters;
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
		free(query_rst);
		query_rst = NULL;
	}

	return;
}

char *grafana_build_reponse_search_top()
{
	char *json_str;
	cJSON *json;

	json = cJSON_CreateArray();
	cJSON_AddItemToArray(json, cJSON_CreateString("src_ip"));
	cJSON_AddItemToArray(json, cJSON_CreateString("dst_ip"));
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
	int i, count;
	long int bytes;
	char *out, s_time[128], e_time[128], *tag, col_name[32];
	time_t start_time, end_time;
	cJSON *out_json, *arrays, *array, *response_json, *columns, *column, *rows, *row, *json_tag, *json_bytes;

	if(!grafana_query_structured(request_body))
	{
		return NULL;
	}
	grafana_get_time_from_request(query_rst, s_time, e_time, sizeof(s_time));
	tag = query_rst->targets->data.additional;
	if(tag == NULL)
	{
		return NULL;
	}
	start_time = timestr_to_stamp(s_time);
	end_time = timestr_to_stamp(e_time);
	job->start_time = start_time;
	job->end_time = end_time;
	/* 根据target拼接json */
	response_json = cJSON_CreateObject();
	/* rows */
	rows = cJSON_AddArrayToObject(response_json, "rows");
	if(!strcasecmp(tag, "flow"))
	{
		out = get_top(mysql, start_time, end_time, TOP_FLOW);
		if(!out)
		{
			return NULL;
		}
		out_json = cJSON_Parse(out);
		job->kind = TOP_FLOW;
		arrays = cJSON_GetObjectItem(out_json, "flow");
		memset(col_name, 0, sizeof(col_name));
		strcpy(col_name, "flow");
	}
	else if(!strcasecmp(tag, "src_set"))
	{
		out = get_top(mysql, start_time, end_time, TOP_SRC_SET);
		if(!out)
		{
			return NULL;
		}
		out_json = cJSON_Parse(out);
		job->kind = TOP_SRC_SET;
		arrays = cJSON_GetObjectItem(out_json, "src_set");
		memset(col_name, 0, sizeof(col_name));
		strcpy(col_name, "src_set");
	}
	else if(!strcasecmp(tag, "dst_set"))
	{
		out = get_top(mysql, start_time, end_time, TOP_DST_SET);
		if(!out)
		{
			return NULL;
		}
		out_json = cJSON_Parse(out);
		job->kind = TOP_DST_SET;
		arrays = cJSON_GetObjectItem(out_json, "dst_set");
		memset(col_name, 0, sizeof(col_name));
		strcpy(col_name, "dst_set");
	}
	else if(!strcasecmp(tag, "src_biz"))
	{
		out = get_top(mysql, start_time, end_time, TOP_SRC_BIZ);
		if(!out)
		{
			return NULL;
		}
		out_json = cJSON_Parse(out);
		job->kind = TOP_SRC_BIZ;
		arrays = cJSON_GetObjectItem(out_json, "src_biz");
		memset(col_name, 0, sizeof(col_name));
		strcpy(col_name, "src_biz");
	}
	else if(!strcasecmp(tag, "dst_biz"))
	{
		out = get_top(mysql, start_time, end_time, TOP_DST_BIZ);
		if(!out)
		{
			return NULL;
		}
		out_json = cJSON_Parse(out);
		job->kind = TOP_DST_BIZ;
		arrays = cJSON_GetObjectItem(out_json, "dst_biz");
		memset(col_name, 0, sizeof(col_name));
		strcpy(col_name, "dst_biz");
	}
	else
	{
		return NULL;
	}
	count = cJSON_GetArraySize(arrays);
	for(i = 0; i < count; i++)
	{
		array = cJSON_GetArrayItem(arrays, i);
		json_tag = cJSON_CreateString(cJSON_GetObjectItemCaseSensitive(array, col_name)->valuestring);
		str_to_long((const char *)(cJSON_GetObjectItemCaseSensitive(array, "bytes")->valuestring), &bytes);
	    json_bytes = cJSON_CreateNumber(bytes);
		row = cJSON_CreateArray();
	    cJSON_AddItemToArray(row, json_tag);
	    cJSON_AddItemToArray(row, json_bytes);
   		cJSON_AddItemToArray(rows, row);
	}
	/* columns */
	columns = cJSON_AddArrayToObject(response_json, "columns");
	column = cJSON_CreateObject();
	cJSON_AddStringToObject(column, "text", col_name);
    cJSON_AddStringToObject(column, "type", "string");
	cJSON_AddItemToArray(columns, column);
    column = cJSON_CreateObject();
	cJSON_AddStringToObject(column, "text", "bytes");
    cJSON_AddStringToObject(column, "type", "number");
	cJSON_AddItemToArray(columns, column);
	/* type */
	cJSON_AddStringToObject(response_json, "type", "table");
	/* 释放空间 */
	cJSON_Delete(out_json);
	free(out);
	out = cJSON_Print(response_json);
	cJSON_Delete(response_json);
	grafana_query_free();
	
	return out;
}

#ifdef __cplusplus
}
#endif
