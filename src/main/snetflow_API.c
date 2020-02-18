#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <getopt.h>
#include <mysql/mysql.h>
#include <evhttp.h>
#include <event.h>
#include <event2/http.h>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/bufferevent_compat.h>
#include <event2/http_struct.h>
#include <event2/http_compat.h>
#include <event2/util.h>
#include <event2/listener.h>

#include "common.h"
#include "snetflow_top.h"
#include "grafana_json.h"
#include "snetflow_API.h"

#ifdef __cplusplus
extern "C"
{
#endif

static uint16_t api_port;
static snetflow_job_s snetflow_job;
/* 数据库对象 */ 
static MYSQL mysql_hd;
static char database_host[32];
static char database_username[32];
static char database_password[32];
static char database_name[32];
static uint16_t database_port;

/* 程序初始化 */
static void sneflow_API_init(int argc, char *argv[])
{
	int requirednum, errornum, index;
	char ret;
	static struct option long_options[] = {
        {"API-port", required_argument, 0, 'p'},
        {"database-host", required_argument, 0, 'h'},
        {"database-name", required_argument, 0, 'a'},
		{"database-port", required_argument, 0, 't'},
		{"debug", no_argument, 0, 'd'},
        {"version", no_argument, 0, 'v'},
        {0,0,0,0}
    };
		
    /* 读取命令行中的参数 */
	sprintf(database_username, "%s", "root");
    sprintf(database_password, "%s", "toor");
	requirednum = errornum = 0;
    optind = optopt = opterr = 0;
    while ((ret = getopt_long(argc, argv, "p:h:a:t:vd", long_options, &index)) != -1)   
    {
        switch (ret)
        {
            case 'p':
                api_port = atoi(optarg);
                requirednum++;
                break;
            case 'h':
				strncpy(database_host, optarg, sizeof(database_host));
				requirednum++;
                break;
            case 'a':
                strncpy(database_name, optarg, sizeof(database_name));
				requirednum++;
                break;
			case 't':
				database_port = atoi(optarg);
				requirednum++;
                break;
            case 'd':
                set_debug(1);
                break;
            case 'v':
                printf("%s version 1.0 \nAuther：langl5@chinaunicom.cn\n", argv[0]);
                exit(0);
                break;
            default:
                errornum++;
                break;
        }
    }
    if(requirednum != 4 || errornum != 0)
    {
        printf(ARGUMENTS);
        exit(-1);
    }
	/* 连接数据库 */
	mysql_init(&mysql_hd);
	if(!mysql_real_connect(&mysql_hd, database_host, database_username, database_password, database_name, database_port, NULL, 0))
	{
		myprintf("Failed to connect to Mysql!\n");
		exit(-1);
	}
    if(mysql_set_character_set(&mysql_hd, "utf8"))
	{ 
        myprintf("Failed to set UTF-8: %s\n", mysql_error(&mysql_hd));
		exit(-1);
    }

	return;
}

/* 初始化snetflow_结构体 */
static void init_snetflow_job(snetflow_job_s *job)
{
	if(!job->data)
	{
		free(job->data);
	}
	memset(job, 0, sizeof(snetflow_job_s));

	return;
}

/* 解析post请求数据 */
static char *get_post_body(struct evhttp_request *req)
{
	int post_size, copy_len;
	char *buf;
	
	post_size = evbuffer_get_length(req->input_buffer); /*获取数据长度 */
	myprintf("post len:%d\n", post_size);
	if (post_size <= 0)
	{
		myprintf("post msg is empty!\n");
		return NULL;
	}
	else
	{
		copy_len = post_size > API_BUFFER_SIZE ? API_BUFFER_SIZE : post_size;
		buf = (char *)malloc(copy_len + 1);
		memcpy(buf, evbuffer_pullup(req->input_buffer, -1), copy_len);
		buf[copy_len] = 0;
		myprintf("post len:%d, copy_len:%d\npost body:%s\n",post_size, copy_len, buf);
	}

	return buf;
}

/* 解析post请求数据 */
static void send_response(struct evhttp_request *req, const char *response_body, const int status_code)
{
	struct evbuffer *retbuff;
	
	retbuff = evbuffer_new();
	if(retbuff == NULL)
	{
		myprintf("%s\n", "Send response error!");
		return;
	}
	if(response_body)
	{
		evbuffer_add_printf(retbuff, "%s", response_body);
	}
	evhttp_add_header(req->output_headers, "Content-Type", "text/plain; charset=UTF-8");
	evhttp_add_header(req->output_headers, "Connection", "close");
	evhttp_send_reply(req, status_code, "Client", retbuff);
	evbuffer_free(retbuff);

	return;
}

static void http_handler_top(struct evhttp_request *req, void *arg)
{
	send_response(req, NULL, HTTP_OK);

	return;
}

static void http_handler_top_search(struct evhttp_request *req, void *arg)
{
	char *out;
	
	init_snetflow_job(&snetflow_job);
	snetflow_job.job_id = 1;
	out = grafana_build_reponse_search_top();
	send_response(req, out, HTTP_OK);
	if(out)
	{
		free(out);
	}

	return;
}

static void http_handler_top_query(struct evhttp_request *req, void *arg)
{
	char *out, *body;
	
	init_snetflow_job(&snetflow_job);
	snetflow_job.job_id = 1;
	body = get_post_body(req); /* 获取请求数据，一般是json格式的数据 */
	if(body == NULL)
	{
		myprintf("%s\n", "Request body is null.");
		send_response(req, NULL, HTTP_BADREQUEST);
		return;
	}

	out = grafana_build_reponse_query_top(&mysql_hd, body, &snetflow_job);
	send_response(req, out, HTTP_OK);
	if(out)
	{
		free(out);
	}

	return;
}

static void http_handler_others(struct evhttp_request *req, void *arg)
{
	send_response(req, NULL, HTTP_NOTFOUND);

	return;
}

int main(int argc, char *argv[])
{
	struct evhttp *http_server;
	
	//初始化
	http_server = NULL;
	event_init();
	sneflow_API_init(argc, argv);
	//启动http服务端
	http_server = evhttp_start((const char *)"0.0.0.0", api_port);
	if(http_server == NULL)
	{
		myprintf("%s\n", "http server start failed.");
		return -1;
	}
	
	/* 设置请求超时时间(s) */
	evhttp_set_timeout(http_server, 1800);
	/* 设置事件处理函数，evhttp_set_cb针对每一个事件(请求)注册一个处理函数 */
	evhttp_set_cb(http_server, "/snetflow-API/top/", http_handler_top, NULL);
	evhttp_set_cb(http_server, "/snetflow-API/top/search", http_handler_top_search, NULL);
	evhttp_set_cb(http_server, "/snetflow-API/top/query", http_handler_top_query, NULL);
	/* evhttp_set_gencb函数，是对所有请求设置一个统一的处理函数 */
	evhttp_set_gencb(http_server, http_handler_others, NULL);
	/* 循环监听 */
	event_dispatch();
	/* 实际上不会释放，代码不会运行到这一步 */
	evhttp_free(http_server);
		
	return 0;
}

#ifdef __cplusplus
}
#endif
