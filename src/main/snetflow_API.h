#ifndef _SNETFLOW__API_H_
#define _SNETFLOW__API_H_

#ifdef __cplusplus
extern "C" {
#endif

#define ARGUMENTS "\
Usage:\n\
$ snetflow-API    necessary:\n\
                  [--API-port <-p> <listened port>\n\
                  [--database-name <-a> <database name>]\n\
                  optional:\n\
                  [--database-host <-h> <database host(default:127.0.0.1)>]\n\
                  [--database-port <-t> <database port(default:3306)>]\n\
                  [--file <-f> <configuration file(default:/etc/snetflow/snetflow.cfg)>]\n\
                  [--time-change   <-c> <time delay/forward(default:0)>]\n\
                  [--debug         <-d> <debug message>]\n\
                  [--version       <-v> <version message>]\n\
$ snetflow-API\
©2020 langl5@chinaunicom.cn\n"

typedef struct _httpd_info_s{
    struct event_base *base;
    struct evhttp *httpd;
}httpd_info_s;

#ifdef __cplusplus
}
#endif

#endif
