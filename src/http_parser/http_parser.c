#include <stdio.h>
#include <string.h>

#include "common.h"
#include "http_parser.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* http_parser 用到的变量 */
static http_parser parser;
static http_parser_settings parser_set;
static int http_url_encode_len;
static char http_url_encode[HTTP_URL_LEN];
static int http_url_decode_len;
static char http_url_decode[HTTP_URL_LEN];
static int http_body_len;
static char http_body[HTTP_BODY_LEN];

static int hex2num(char c)
{
    if (c >= '0' && c <= '9')
	{
		return c - '0';
    }
    if (c >= 'a' && c <= 'z')
	{
		return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'Z')
	{
		return c - 'A' + 10;
    }
    
    return NON_NUM;
}

/* url反编码 */
static int url_decode(const char* str, const int str_size, char* result, const int result_size)
{
    char ch, ch1, ch2;
    int i, j;
 
    if((str == NULL) || (result == NULL) || (str_size <= 0) || (result_size <= 0))
	{
        return 0;
    }
 
    for(i = 0, j = 0; (i < str_size) && (j < result_size); i++)
	{
        ch = str[i];
        switch (ch)
		{
            case '+':
                result[j++] = ' ';
                break;
            case '%':
                if (i + 2 < str_size)
				{
                    ch1 = hex2num(str[i + 1]);/* 高4位 */
                    ch2 = hex2num(str[i + 2]);/* 低4位 */
                    if ((ch1 != NON_NUM) && (ch2 != NON_NUM))
                        result[j++] = (char)((ch1 << 4) | ch2);
                    i += 2;
                }
                break;
            default:
                result[j++] = ch;
                break;
        }
    }
    result[j] = 0;
	
    return j;
}

static int on_message_begin(http_parser *_)
{
    (void)_;
	if(DEBUG)
	{
		printf("\n***MESSAGE BEGIN***\n\n");
	}
	
    return 0;
}

static int on_headers_complete(http_parser *_)
{
    (void)_;
	if(DEBUG)
	{
		printf("\n***HEADERS COMPLETE***\n\n");
	}
	
    return 0;
}

static int on_message_complete(http_parser *_)
{
    (void)_;
	if(DEBUG)
	{
		printf("\n***MESSAGE COMPLETE***\n\n");
	}
	
    return 0;
}

static int on_url(http_parser *_, const char *at, size_t length)
{
    (void)_;
	if(DEBUG)
	{
		printf("Url: %.*s\n", (int)length, at);
	}
	memcpy(http_url_encode, at, length);
	http_url_encode_len = length;
	http_url_decode_len = url_decode(http_url_encode, length, http_url_decode, sizeof(http_url_decode));
	
    return 0;
}

static int on_header_field(http_parser *_, const char *at, size_t length)
{
    (void)_;
	if(DEBUG)
	{
		printf("Header field: %.*s\n", (int)length, at);
	}
	
    return 0;
}

static int on_header_value(http_parser *_, const char *at, size_t length)
{
    (void)_;
	if(DEBUG)
	{
		printf("Header value: %.*s\n", (int)length, at);
	}
    
    return 0;
}

static int on_body(http_parser *_, const char *at, size_t length)
{
    (void)_;
	if(DEBUG)
	{
		printf("Body: %.*s\n", (int)length, at);
	}
	memcpy(http_body, at, length);
	http_body_len = length;
	
    return 0;
}

char *get_http_url_encode()
{
	return http_url_encode;
}

int get_http_url_encode_len()
{
	return http_url_encode_len;
}

char *get_http_url_decode()
{
	return http_url_decode;
}

int get_http_url_decode_len()
{
	return http_url_decode_len;
}

char *get_http_body()
{
	return http_body;
}

int get_http_body_len()
{
	return http_body_len;
}

void parser_init()
{
    /* http_parser的回调函数，需要获取HEADER后者BODY信息，可以在这里面处理. */
    memset(&parser_set, 0, sizeof(parser_set));
    parser_set.on_message_begin = on_message_begin;
    parser_set.on_header_field = on_header_field;
    parser_set.on_header_value = on_header_value;
    parser_set.on_url = on_url;
    parser_set.on_body = on_body;
    parser_set.on_headers_complete = on_headers_complete;
    parser_set.on_message_complete = on_message_complete;
	/* 初始化parser为Request类型 */
    http_parser_init(&parser, HTTP_REQUEST);

	return;
}

int http_parse(const char *http_pkt, int http_pkt_len)
{
	int parsed;

	/* 初始化http parser */
	parser_init();
	/* 初始化数据 */
	http_url_encode_len = 0;
	memset(http_url_encode, 0, sizeof(http_url_encode));
	http_url_decode_len= 0;
	memset(http_url_decode, 0, sizeof(http_url_decode));
	http_body_len= 0;
	memset(http_body, 0, sizeof(http_body));
	/* 执行解析过程 */
    parsed = http_parser_execute(&parser, &parser_set, http_pkt, http_pkt_len);
    if (parsed != http_pkt_len)
    {
		if(DEBUG)
		{
        	fprintf(stderr, "Error: %s (%s)\n", http_errno_description(HTTP_PARSER_ERRNO(&parser)), http_errno_name(HTTP_PARSER_ERRNO(&parser)));
		}
		return -1;
    }
	
    return 0;
}

#ifdef __cplusplus
}
#endif
