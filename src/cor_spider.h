#ifndef COR_SPIDER_H
#define COR_SPIDER_H

#include <ev.h>
#include <curl/curl.h>

#include "cor_core.h"
#include "cor_buf.h"
#include "cor_list.h"
#include "cor_log.h"
#include "cor_str.h"

enum {
    COR_SPIDER_OK = 0,
    COR_SPIDER_ERROR,
    COR_SPIDER_TIMEOUT,
};

typedef void (cor_spider_cb_t) (cor_str_t *url, cor_buf_chain_t *bufs, int status, long http_code, void *arg);
typedef struct cor_spider_s cor_spider_t;

typedef struct cor_spider_conn_s cor_spider_conn_t;
struct cor_spider_conn_s
{
    int fd;
    int flags;
    ev_io ev;
    ev_timer timer;
    unsigned easy_active:1;
    CURL *easy;
    char error[CURL_ERROR_SIZE];
    cor_str_t url;
    size_t url_buf_size;
    cor_buf_chain_t bufs;
    /**/
    void *arg;
    cor_spider_cb_t *cb;
    /**/
    cor_spider_t *spider;
    cor_spider_conn_t *next;
};

struct cor_spider_s
{
    cor_list_t *conns;
    cor_spider_conn_t *free_conns;
    cor_buf_pool_t *buf_pool;
    cor_log_t *log;
    struct ev_loop *loop;
    /**/
    CURLM *curl;
    ev_timer curl_timer;
};

cor_spider_t *cor_spider_new(struct ev_loop *loop, cor_log_t *log, int buf_count, int buf_size);
void cor_spider_delete(cor_spider_t *spider);
int cor_spider_get_url_content(cor_spider_t *spider, const char *url, int url_size, cor_spider_cb_t *cb, void *arg);

#endif
