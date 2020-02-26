#ifndef _CONFIG_H_
#define _CONFIG_H_

#define TOP     0
#define TREND   1
#define HISTORY 2

typedef struct _mysql_conf{
	char name[32];
	char table[32];
	char column[128];
	char condition[256];
}mysql_conf_s;

void read_cfg_file(const char *file);
mysql_conf_s *get_config(const char *key, int kind);

#endif
