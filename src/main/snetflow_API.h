#ifndef _SNETFLOW__API_H_
#define _SNETFLOW__API_H_

#ifdef __cplusplus
extern "C" {
#endif

#define API_PORT  6666
#define API_BUFFER_SIZE 40960

#define ARGUMENTS "\
Usage:\n\
$ snetflow        -i <listen interface> -p <listen port>\n\
                  [--receive-buffer-size <-r> <receive buffer size of UDP socket>]\n\
                  [--database-host <-h> <database host>]\n\
                  [--database-name <-1> <database name>]\n\
                  [--packet-sampling-rate <-3> <sampling rate>]\n\
                  [--debug <debug message> <-d>]\n\
$ snetflow        -v\n\
©2020 langl5@chinaunicom.cn\n"

#ifdef __cplusplus
}
#endif

#endif
