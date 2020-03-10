#include <stdio.h>#include <stdlib.h>#include <string.h>#include <sys/socket.h>#include <netinet/in.h>#include <arpa/inet.h>#include <libconfig.h>#include <iostream>#include <string>#include <map>#include "config.h"using namespace std;static map<string, mysql_conf_s> top;static map<string, mysql_conf_s> trend;static map<string, mysql_conf_s> history;static map<string, mysql_conf_s> warning;static map<string, mysql_conf_s> sum;static map<string, mysql_conf_s> associate;static char timestamp[128];static uint32_t thread_num;static uint32_t response_timeout;static uint32_t listened_num;static uint32_t request_body_size;static uint32_t history_num;static uint32_t trend_point;void read_cfg_file(const char *file){    int cnt, i;    char f[256];    const char *name, *table, *column, *condition;    mysql_conf_s conf;    config_t cfg;    config_setting_t *setting, *tmp;    if (!file || strlen(file) == 0)    {        sprintf(f, "%s", "/etc/snetflow-API/snetflow-API.cfg");    }    else    {        snprintf(f, sizeof(f) - 1, "%s", file);    }	/* 初始化变量 */	sprintf(timestamp, "%s", "timestamp");	thread_num = 10;	response_timeout = 600;	listened_num = 1024;	request_body_size = 40960;	history_num = 10000;	trend_point = 30;    /* 打开配置文件 */    config_init(&cfg);    if (!config_read_file(&cfg, f))    {        fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));        config_destroy(&cfg);        exit(-1);    }    /* 读取配置文件top */    setting = config_lookup(&cfg, "top");    if (setting != NULL)    {        cnt = config_setting_length(setting);        for (i = 0; i < cnt; i++)        {            tmp = config_setting_get_elem(setting, i);            if (!(config_setting_lookup_string(tmp, "name", &name) && config_setting_lookup_string(tmp, "table", &table) && config_setting_lookup_string(tmp, "column", &column) && config_setting_lookup_string(tmp, "condition", &condition)))            {                fprintf(stderr, "Invalid key or value\n");                exit(-1);            }			strncpy(conf.name, name, sizeof(conf.name));			strncpy(conf.table, table, sizeof(conf.table));			strncpy(conf.column, column, sizeof(conf.column));			strncpy(conf.condition, condition, sizeof(conf.condition));			top[conf.name] = conf;        }    }    else    {        fprintf(stderr, "Can't find top\n");        exit(-1);    }	/* 读取配置文件trend */    setting = config_lookup(&cfg, "trend");    if (setting != NULL)    {        cnt = config_setting_length(setting);        for (i = 0; i < cnt; i++)        {            tmp = config_setting_get_elem(setting, i);            if (!(config_setting_lookup_string(tmp, "name", &name) && config_setting_lookup_string(tmp, "table", &table) && config_setting_lookup_string(tmp, "column", &column) && config_setting_lookup_string(tmp, "condition", &condition)))            {                fprintf(stderr, "Invalid key or value\n");                exit(-1);            }			strncpy(conf.name, name, sizeof(conf.name));			strncpy(conf.table, table, sizeof(conf.table));			strncpy(conf.column, column, sizeof(conf.column));			strncpy(conf.condition, condition, sizeof(conf.condition));			trend[conf.name] = conf;        }    }    else    {        fprintf(stderr, "Can't find trend\n");        exit(-1);    }	/* 读取配置文件history */    setting = config_lookup(&cfg, "history");    if (setting != NULL)    {        cnt = config_setting_length(setting);        for (i = 0; i < cnt; i++)        {            tmp = config_setting_get_elem(setting, i);            if (!(config_setting_lookup_string(tmp, "name", &name) && config_setting_lookup_string(tmp, "table", &table) && config_setting_lookup_string(tmp, "column", &column) && config_setting_lookup_string(tmp, "condition", &condition)))            {                fprintf(stderr, "Invalid key or value\n");                exit(-1);            }			strncpy(conf.name, name, sizeof(conf.name));			strncpy(conf.table, table, sizeof(conf.table));			strncpy(conf.column, column, sizeof(conf.column));			strncpy(conf.condition, condition, sizeof(conf.condition));			history[conf.name] = conf;        }    }    else    {        fprintf(stderr, "Can't find history\n");        exit(-1);    }	/* 读取配置文件warning */    setting = config_lookup(&cfg, "warning");    if (setting != NULL)    {        cnt = config_setting_length(setting);        for (i = 0; i < cnt; i++)        {            tmp = config_setting_get_elem(setting, i);            if (!(config_setting_lookup_string(tmp, "name", &name) && config_setting_lookup_string(tmp, "table", &table) && config_setting_lookup_string(tmp, "column", &column) && config_setting_lookup_string(tmp, "condition", &condition)))            {                fprintf(stderr, "Invalid key or value\n");                exit(-1);            }			strncpy(conf.name, name, sizeof(conf.name));			strncpy(conf.table, table, sizeof(conf.table));			strncpy(conf.column, column, sizeof(conf.column));			strncpy(conf.condition, condition, sizeof(conf.condition));			warning[conf.name] = conf;        }    }    else    {        fprintf(stderr, "Can't find warning\n");        exit(-1);    }	/* 读取配置文件sum */    setting = config_lookup(&cfg, "sum");    if (setting != NULL)    {        cnt = config_setting_length(setting);        for (i = 0; i < cnt; i++)        {            tmp = config_setting_get_elem(setting, i);            if (!(config_setting_lookup_string(tmp, "name", &name) && config_setting_lookup_string(tmp, "table", &table) && config_setting_lookup_string(tmp, "column", &column) && config_setting_lookup_string(tmp, "condition", &condition)))            {                fprintf(stderr, "Invalid key or value\n");                exit(-1);            }			strncpy(conf.name, name, sizeof(conf.name));			strncpy(conf.table, table, sizeof(conf.table));			strncpy(conf.column, column, sizeof(conf.column));			strncpy(conf.condition, condition, sizeof(conf.condition));			sum[conf.name] = conf;        }    }    else    {        fprintf(stderr, "Can't find sum\n");        exit(-1);    }	/* 读取配置文件sum */    setting = config_lookup(&cfg, "associate");    if (setting != NULL)    {        cnt = config_setting_length(setting);        for (i = 0; i < cnt; i++)        {            tmp = config_setting_get_elem(setting, i);            if (!(config_setting_lookup_string(tmp, "name", &name) && config_setting_lookup_string(tmp, "table", &table) && config_setting_lookup_string(tmp, "column", &column) && config_setting_lookup_string(tmp, "condition", &condition)))            {                fprintf(stderr, "Invalid key or value\n");                exit(-1);            }			strncpy(conf.name, name, sizeof(conf.name));			strncpy(conf.table, table, sizeof(conf.table));			strncpy(conf.column, column, sizeof(conf.column));			strncpy(conf.condition, condition, sizeof(conf.condition));			associate[conf.name] = conf;        }    }    else    {        fprintf(stderr, "Can't find associate\n");        exit(-1);    }	/* 读取配置文件base */    setting = config_lookup(&cfg, "base");    if (setting != NULL)    {        if(config_setting_lookup_string(setting, "timestamp", &name))        {			sprintf(timestamp, "%s", name);        }		if(config_setting_lookup_int(setting, "threadNum", &i))        {			thread_num = (uint32_t)i;        }		if(config_setting_lookup_int(setting, "responseTimeout", &i))        {			response_timeout = (uint32_t)i;        }		if(config_setting_lookup_int(setting, "requestBodySize", &i))        {			request_body_size= (uint32_t)i;        }		if(config_setting_lookup_int(setting, "historyNum", &i))        {			history_num = (uint32_t)i;        }		if(config_setting_lookup_int(setting, "trendPoint", &i))        {			trend_point = (uint32_t)i;        }    }	config_destroy(&cfg);	return;}mysql_conf_s *get_config(const char *key, int kind){	if(kind == TOP && top.find(key) != top.end())	{		return &(top[key]);	}	else if(kind == TREND && trend.find(key) != trend.end())	{		return &(trend[key]);	}	else if(kind == HISTORY && history.find(key) != history.end())	{		return &(history[key]);	}	else if(kind == WARNING && warning.find(key) != warning.end())	{		return &(warning[key]);	}	else if(kind == SUM && sum.find(key) != sum.end())	{		return &(sum[key]);	}	else if(kind == ASSOCIATE && associate.find(key) != associate.end())	{		return &(associate[key]);	}	return NULL;}char *cfg_get_timestamp(){	return timestamp;}uint32_t cfg_get_thread_num(){	return thread_num;}uint32_t cfg_get_response_timeout(){	return response_timeout;}uint32_t cfg_get_listened_num(){	return listened_num;}uint32_t cfg_get_request_body_size(){	return request_body_size;}uint32_t cfg_get_history_num(){	return history_num;}uint32_t cfg_get_trend_point(){	return trend_point;}