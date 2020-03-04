#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <getopt.h>
#include <pthread.h>
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
#include "config.h"
#include "grafana_json.h"
#include "snetflow_API.h"

#ifdef __cplusplus
extern "C"
{
#endif

static uint16_t api_port;
/* 数据库对象 */
static char database_host[32];
static char database_username[32];
static char database_password[32];
static char database_name[32];
static uint16_t database_port;
/* 配置文件 */
static char cfg_file[256];

/* 数据库初始化 */
static int init_mysql(MYSQL *mysql)
{
	/* 连接数据库 */
	mysql_init(mysql);
	if(!mysql_real_connect(mysql, database_host, database_username, database_password, database_name, database_port, NULL, 0))
	{
		myprintf("Failed to connect to Mysql: %s\n", mysql_error(mysql));
		return -1;
	}
    if(mysql_set_character_set(mysql, "utf8"))
	{ 
        myprintf("Failed to set UTF-8: %s\n", mysql_error(mysql));
		return -1;
    }

	return 0;
}

/* 程序初始化 */
static void sneflow_API_init(int argc, char *argv[])
{
	int requirednum, errornum, index;
	char ret;
	static struct option long_options[] = {
        {"API-port", required_argument, 0, 'p'},
        {"database-host", required_argument, 0, 'h'},
        {"database-name", required_argument, 0, 'a'},
	    {"file", required_argument, 0, 'f'},
		{"database-port", required_argument, 0, 't'},
		{"time-change", required_argument, 0, 'c'},
		{"debug", no_argument, 0, 'd'},
        {"version", no_argument, 0, 'v'},
        {0,0,0,0}
    };
		
    /* 读取命令行中的参数,先设置默认值 */
	sprintf(database_host, "%s", "127.0.0.1");
	sprintf(database_username, "%s", "root");
    sprintf(database_password, "%s", "toor");
	sprintf(cfg_file, "%s", "/etc/snetflow-API/snetflow-API.cfg");
	database_port = 3306;
	requirednum = errornum = 0;
    optind = optopt = opterr = 0;
    while ((ret = getopt_long(argc, argv, "p:h:a:t:c:f:vd", long_options, &index)) != -1)   
    {
        switch (ret)
        {
            case 'p':
                api_port = atoi(optarg);
                requirednum++;
                break;
            case 'h':
				strncpy(database_host, optarg, sizeof(database_host));
                break;
            case 'a':
                strncpy(database_name, optarg, sizeof(database_name));
				requirednum++;
                break;
			case 'f':
                strncpy(cfg_file, optarg, sizeof(cfg_file) - 1);
                break;
			case 't':
				database_port = atoi(optarg);
                break;
			case 'c':
				set_timechange(atoi(optarg));
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
    if(requirednum != 2 || errornum != 0)
    {
        printf(ARGUMENTS);
        exit(-1);
    }
	/* 读取配置文件 */
	read_cfg_file(cfg_file);

	return;
}

/* 绑定套接字 */
static int bind_socket()
{
    int ret, server_socket, opt;
	struct sockaddr_in addr;

	/* NOTE 多线程evhttp必须非阻塞 */
    server_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0); 
    if(server_socket < 0)
    {
		myprintf("ERROR get socket: %d\n", server_socket);
		exit(-1);
    }
	/* 端口复用 */
	opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    /* 绑定端口     */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(api_port);
    ret = bind(server_socket, (struct sockaddr *)&addr, sizeof(struct sockaddr));
    if(ret < 0)
    {
        myprintf("bind error\n");
		exit(-1);
    }
    listen(server_socket, cfg_get_listened_num());
	
    return server_socket;
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

/* 打印request信息 */
static void print_request(struct evhttp_request *req)
{
	struct evkeyvalq *headers;
    struct evkeyval *kv;
	
    switch (req->type)
    {
	    case EVHTTP_REQ_GET:
	        myprintf("GET");
	        break;
	    case EVHTTP_REQ_POST:
	        myprintf("POST");
	        break;
	    case EVHTTP_REQ_HEAD:
	        myprintf("HEAD");
	        break;
	    case EVHTTP_REQ_PUT:
	        myprintf("PUT");
	        break;
	    case EVHTTP_REQ_DELETE:
	        myprintf("DELETE");
	        break;
	    case EVHTTP_REQ_OPTIONS:
	        myprintf("OPTIONS");
	        break;
	    case EVHTTP_REQ_TRACE:
	        myprintf("TRACE");
	        break;
	    case EVHTTP_REQ_CONNECT:
	        myprintf("CONNECT");
	        break;
	    case EVHTTP_REQ_PATCH:
	        myprintf("PATCH");
	        break;
	    default:
	        myprintf("UNKNOWN");
    }
    myprintf(" %s \n", req->uri);
    headers = evhttp_request_get_input_headers(req);
    kv = headers->tqh_first;
    while(kv)
    {
        myprintf("%s: %s\n", kv->key, kv->value);
        kv = kv->next.tqe_next;
    }

	return;
}


/* 解析post请求数据 */
static char *get_post_body(struct evhttp_request *req)
{
	int post_size, copy_len, body_len;
	char *buf;
	
	post_size = evbuffer_get_length(req->input_buffer); /*获取数据长度 */
	if (post_size <= 0)
	{
		myprintf("post msg is empty!\n");
		return NULL;
	}
	else
	{
		body_len = cfg_get_request_body_size();
		copy_len = post_size > body_len ? body_len : post_size;
		buf = (char *)malloc(copy_len + 1);
		memcpy(buf, evbuffer_pullup(req->input_buffer, -1), copy_len);
		buf[copy_len] = 0;
		myprintf("===THREAD %ld===\n", pthread_self());
	    myprintf("IP: %s:%d\n", req->remote_host, req->remote_port);
	    print_request(req);
		myprintf("Body len:%d {%s}\n", post_size, buf);
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
	myprintf("Send response(status code:%d) success!\n", status_code);

	return;
}

static void http_handler_root(struct evhttp_request *req, void *arg)
{
	send_response(req, NULL, HTTP_OK);

	return;
}

static void http_handler_search(struct evhttp_request *req, void *arg)
{
	char *out;
	snetflow_job_s snetflow_job;
	
	init_snetflow_job(&snetflow_job);
	snetflow_job.job_id = 1;
	out = grafana_build_reponse_search();
	send_response(req, out, HTTP_OK);
	if(out)
	{
		free(out);
	}

	return;
}

static void http_handler_query(struct evhttp_request *req, void *arg)
{
	char *out, *body;
	snetflow_job_s snetflow_job;
	MYSQL mysql;

	mysql_thread_init();
	if(init_mysql(&mysql) != 0)
	{
		/* 未知错误 */
		send_response(req, NULL, HTTP_INTERNAL);
		return;
	}
	init_snetflow_job(&snetflow_job);
	snetflow_job.job_id = 1;
	body = get_post_body(req); /* 获取请求数据，一般是json格式的数据 */
	if(body == NULL)
	{
		myprintf("%s\n", "Request body is null.");
		send_response(req, NULL, HTTP_BADREQUEST);
		return;
	}
	out = grafana_build_reponse_query(&mysql, body, &snetflow_job);
	mysql_close(&mysql);
	mysql_thread_end();
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

void *http_dispatch(void *args)
{
    httpd_info_s *info;

	info = (httpd_info_s *)args;
    myprintf("thread %ld start\n", pthread_self());
    event_base_dispatch(info->base);
    myprintf("thread %ld done\n", pthread_self());
    event_base_free(info->base);
    evhttp_free(info->httpd);

	return NULL;
}

int main(int argc, char *argv[])
{
	int i, ret, server_socket, thread_num;
	pthread_t *http_ser_ths, *pthrd;
    httpd_info_s *http_ser_info, *pinfo;

	/* 初始化 */
	sneflow_API_init(argc, argv);
	server_socket = bind_socket();
	/* 创建线程 */
	thread_num = cfg_get_thread_num();
	http_ser_ths = (pthread_t *)malloc(sizeof(pthread_t) * thread_num);
	memset(http_ser_ths, 0, sizeof(pthread_t) * thread_num);
	http_ser_info = (httpd_info_s *)malloc(sizeof(httpd_info_s) * thread_num);
	memset(http_ser_info, 0, sizeof(httpd_info_s) * thread_num);
	for(i = 0; i < thread_num; i++)
    {
		pthrd = http_ser_ths + i;
        pinfo = http_ser_info + i;
        pinfo->base = event_base_new();
        if(pinfo->base == NULL)
        {
            myprintf("ERROR new http base\n");
			exit(-1);
        }
        pinfo->httpd = evhttp_new(pinfo->base);
        if(pinfo->httpd == NULL)
        {
            myprintf("ERROR new evhttp\n");
			exit(-1);
        }
        ret = evhttp_accept_socket(pinfo->httpd, server_socket);
        if(ret != 0)
        {
            myprintf("Error evhttp_accept_socket\n");
			exit(-1);
        }
		/* 设置请求超时时间(s) */
		evhttp_set_timeout(pinfo->httpd, cfg_get_response_timeout());
		/* 设置事件处理函数，evhttp_set_cb针对每一个事件(请求)注册一个处理函数 */
		evhttp_set_cb(pinfo->httpd, "/snetflow-API/top/", http_handler_root, NULL);
		evhttp_set_cb(pinfo->httpd, "/snetflow-API/top/search", http_handler_search, NULL);
		evhttp_set_cb(pinfo->httpd, "/snetflow-API/top/query", http_handler_query, NULL);
		/* evhttp_set_gencb函数，是对所有请求设置一个统一的处理函数 */
		evhttp_set_gencb(pinfo->httpd, http_handler_others, NULL);
        ret = pthread_create(pthrd, NULL, http_dispatch, (void *)pinfo);
		if(ret != 0)
        {
            myprintf("Error pthread_create\n");
			exit(-1);
        }
    }
    for (i = 0; i < thread_num; i++)
    {
		pthrd = http_ser_ths + i;
        pthread_join(*pthrd, NULL);
    }
		
	return 0;
}

#ifdef __cplusplus
}
#endif
