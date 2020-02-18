#ifndef _GRAFANA_JSON_H_
#define _GRAFANA_JSON_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _grafana_query_request_range_raw{
	char from[128];
	char to[128];
}grafana_query_request_range_raw_s;

typedef struct _grafana_query_request_range{
	char from[128];
	char to[128];
	grafana_query_request_range_raw_s raw;
}grafana_query_request_range_s;

typedef struct _grafana_query_request_targets_data{
	char additional[128];
}grafana_query_request_targets_data_s;

typedef struct _grafana_query_request_target{
	char target[128];
	char refid[128];
	char type[128];
	grafana_query_request_targets_data_s data;
	struct _grafana_query_request_target *next;
}grafana_query_request_target_s;

typedef struct _grafana_query_request_adhoc_filter{
	char key[128];
	char operator_c[128];
	char value[128];
	struct _grafana_query_request_adhoc_filter *next;
}grafana_query_request_adhoc_filter_s;

typedef struct _grafana_query_request{
    uint32_t panelid;
	grafana_query_request_range_s range;
	grafana_query_request_range_raw_s range_raw;
	char interval[128];
	uint32_t interval_ms;
	uint32_t max_data_points;
	grafana_query_request_target_s *targets;
	grafana_query_request_adhoc_filter_s *adhoc_filters;
}grafana_query_request_s;

char *grafana_build_reponse_search_top();
char *grafana_build_reponse_query_top(MYSQL *mysql, const char *request_body, snetflow_job_s *job);

#ifdef __cplusplus
}
#endif

#endif
