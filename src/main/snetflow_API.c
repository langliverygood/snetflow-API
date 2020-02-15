#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <mysql/mysql.h>

#include "common.h"
#include "http_parser.h"
#include "snetflow_top.h"
#include "grafana_json.h"
#include "snetflow_API.h"

#ifdef __cplusplus
extern "C"
{
#endif

static int server_socket;
static struct sockaddr_in server_addr;
static struct sockaddr_in client_addr;
static char recv_buffer[API_BUFFER_SIZE];
static snetflow_job_s snetflow_job;
static url_parted_s url_parted;
static MYSQL mysql_hd;

static void part_url(const char *url, const int url_len, url_parted_s *u)
{
	int i, j;

	/* 初始化并释放空间 */
	for(i = 0; i < u->count; i++)
	{
		free(u->parted[i]);
	}
	u->count = 0;

	j = 0;
	while(j < url_len)
	{
		i = j;
		if(url[i] == '/')
		{
			i++;
			j = i;
			continue;
		}
		while(url[j] != '/' && j < url_len)
		{
			j++;
		}
		u->parted[u->count] = (char *)malloc(j - i + 1);
		memcpy(u->parted[u->count], url + i, j - i);
		u->parted[u->count][j - i] = 0;
		u->count++;	
	}

	return;
}

static void parse_request_line(const char *request, int request_len, snetflow_job_s *job)
{
	int i;
	char request_method[16];

	/* 解析方法，如不是get或者post就报错退出 */
	for(i = 0; i < request_len; i++)
	{
		if(request[i] == ' ')
		{
			break;
		}
	}
	if(i >= 16)
	{
		job->job_id = -1;
		return;
	}
	memset(request_method, 0, sizeof(request_method));
	strncpy(request_method, request, i);
	if(!strcasecmp(request_method, "get"))
	{
		job->http_request_method = GET;
	}
	else if(!strcasecmp(request_method, "post"))
	{
		job->http_request_method = POST;
	}
	else
	{
		job->job_id = -1;
		return;
	}

	/* 解析url */
	part_url(get_http_url_decode(), get_http_url_decode_len(), &url_parted);
	if(!strcasecmp("snetflow-API", url_parted.parted[0]))
	{
		if(!strcasecmp("top", url_parted.parted[1]))
		{
			job->job_id = 1;
		}
		else if(!strcasecmp("history", url_parted.parted[1]))
		{
			job->job_id = 2;
		}
		else if(!strcasecmp("increase", url_parted.parted[1]))
		{
			job->job_id = 3;
		}
		else
		{
			job->job_id = -1;
		}
	}
	else
	{
		job->job_id = -1;
	}

    return;
}

void sneflow_API_init()
{
	if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket!");
        exit(-1);
    }
	/* 定义sockaddr_in */
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(API_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	/* 将套接字绑定到地址,bind，成功返回0，出错返回-1 */
	if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connect");
        exit(-1);
    }
	/* 监听,，成功返回0，出错返回-1 */
	if (listen(server_socket, 5) < 0)
    {
        perror("listen");
        exit(-1);
    }
	/* 连接数据库 */
	mysql_init(&mysql_hd);
	if(!mysql_real_connect(&mysql_hd, "localhost", "root", "toor", "netflow_xx", 0, NULL, 0))
	{
		printf("Failed to connect to Mysql!\n");
		exit(-1);
	}
    if(mysql_set_character_set(&mysql_hd, "utf8"))
	{ 
        printf("Failed to set UTF-8: %s\n", mysql_error(&mysql_hd));
		exit(-1);
    }

	return;
}

int main(int argc, char *argv[])
{
    int client, client_len, n;
    int recv_cnt;
	char *out, send_data[1024];

	memset(send_data, 0, sizeof(send_data));
	
	client_len = sizeof(client_addr);
	sneflow_API_init();
	out = NULL;
    while (1)
    {
        client = accept(server_socket, (struct sockaddr *)&client_addr, (socklen_t *)&client_len);
        if (client < 0)
        {
            perror("accept");
            continue;
        }
		memset(recv_buffer, 0, sizeof(recv_buffer));
		recv_cnt = recv(client, recv_buffer, sizeof(recv_buffer), 0);
        if (recv_cnt < 0)
        {
            perror("recv null");
			close(client);
            continue;
        }
			
		if(!http_parse(recv_buffer, recv_cnt))
		{
			parse_request_line(recv_buffer, recv_cnt, &snetflow_job);
			if(snetflow_job.job_id == -1)
			{
				close(client);
				continue;
			}
			if(snetflow_job.job_id == 1)
			{
				if(url_parted.count > 2)
				{
					out = grafana_parse_request(&mysql_hd, get_http_body(), &snetflow_job, url_parted.parted[2]);
				}
				else
				{
					out = grafana_parse_request(&mysql_hd, get_http_body(), &snetflow_job, "");
				}
			}
			sprintf(send_data, "HTTP/1.0 ok \r\n%s", out);
			n = send(client, send_data, strlen(send_data), 0);
			{
				printf("%d:%s\n", n, send_data);
			}
			free(out);
			
		}
		else
		{
			sprintf(send_data, "HTTP/1.0 ok \r\nRequest ERROR");
			n = send(client, send_data, strlen(send_data), 0);
			printf("%d:%s\n", n, send_data);
		}
		//close(client);
		shutdown(client, SHUT_RDWR);
    }

    return 0;
}

#ifdef __cplusplus
}
#endif
