#ifndef COR_MONGO_H
#define COR_MONGO_H

#include <pthread.h>
#include <bson.h>
#include <mongoc.h>
#include <ev.h>

#include "cor_core.h"
#include "cor_log.h"
#include "cor_pool.h"
#include "cor_str.h"

enum
{
    COR_MONGO_CMD_GETS,
    COR_MONGO_CMD_SET,
};

typedef void (cor_mongo_cb_t) (int status, const bson_t *docs, int docs_count, void *arg);

typedef struct cor_mongo_request_s cor_mongo_request_t;
struct cor_mongo_request_s
{
    int status;
    int cmd;
    const char *db;
    const char *collection;
    bson_t query;
    bson_t fields;
    unsigned fields_limited:1;
    int docs_count;
    int docs_count_max;
    int docs_skip;
    int docs_limit;
    bson_t *docs;
    cor_mongo_cb_t *cb;
    void *arg;
    cor_mongo_request_t *next;
};

typedef struct
{
    pthread_mutex_t mutex;
    cor_mongo_request_t *head;
    cor_mongo_request_t *tail;
} cor_mongo_queue_t;

typedef struct
{
    int docs_count_min;
    int docs_count_max;
    /**/
    volatile int is_exit;
    cor_mongo_request_t *free_requests;
    cor_mongo_queue_t request_queue;
    cor_mongo_queue_t response_queue;
    ev_async response_queue_signal;
    /**/
    mongoc_client_pool_t *client_pool;
    pthread_t *threads;
    int threads_count;
    pthread_mutex_t thread_mutex;
    pthread_cond_t thread_cond;
    unsigned delete_pool:1;
    cor_pool_t *pool;
    cor_log_t *log;
    struct ev_loop *loop;
} cor_mongo_t;

cor_mongo_t *cor_mongo_new(const char *uri, int threads_count, struct ev_loop *loop, cor_pool_t *pool, cor_log_t *log);
void cor_mongo_delete(cor_mongo_t *ctx);
int cor_mongo_gets(cor_mongo_t *ctx, const char *db, const char *collection, const bson_t *query,
    const bson_t *fields, int skip, int limit, cor_mongo_cb_t *cb, void *arg);
int cor_mongo_set(cor_mongo_t *ctx, const char *db, const char *collection, const bson_t *data,
    cor_mongo_cb_t *cb, void *arg);

#endif
