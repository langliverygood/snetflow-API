#ifndef _SNETFLOW__API_H_
#define _SNETFLOW__API_H_

#ifdef __cplusplus
extern "C" {
#endif

#define API_BUFFER_SIZE 40960

#define ARGUMENTS "\
Usage:\n\
$ snetflow-API    [--API-port <-p> <listened port>\n\
                  [--database-host <-h> <database host>]\n\
                  [--database-name <-a> <database name>]\n\
                  [--database-port <-t> <database port>]\n\
                  [--debug <-d> <debug message>]\n\
                  [--version <-v> <version message>]\n\
$ snetflow-API\
©2020 langl5@chinaunicom.cn\n"

#ifdef __cplusplus
}
#endif

#endif
