#include "cor_mongo.h"

#define COR_MONGO_POOL_SIZE 4096
#define COR_MONGO_RESULT_COUNT_MIN 4
#define COR_MONGO_RESULT_COUNT_MAX 256

static void *cor_mongo_loop(void *arg);
static void cor_mongo_response_queue_cb(struct ev_loop *loop, ev_async *w, int revents);
static inline void cor_mongo_queue_push(cor_mongo_queue_t *queue, cor_mongo_request_t *r);
static inline cor_mongo_request_t *cor_mongo_request_new(cor_mongo_t *ctx);
static inline void cor_mongo_request_delete(cor_mongo_t *ctx, cor_mongo_request_t *r);
static inline int cor_mongo_doc_append(cor_mongo_t *ctx, cor_mongo_request_t *r, const bson_t *doc);
static inline cor_mongo_request_t *cor_mongo_queue_pop(cor_mongo_queue_t *queue);
static int cor_mongo_gets_query(cor_mongo_t *ctx, cor_mongo_request_t *r);
static int cor_mongo_set_query(cor_mongo_t *ctx, cor_mongo_request_t *r);

cor_mongo_t *
cor_mongo_new(const char *uri, int threads_count, struct ev_loop *loop, cor_pool_t *pool, cor_log_t *log)
{
    int delete_pool = 0;
    if (!pool) {
        pool = cor_pool_new(COR_MONGO_POOL_SIZE);
        if (!pool) {
            cor_log_error(log, "can't cor_pool_new");
            return NULL;
        }
        delete_pool = 1;
    }
    cor_mongo_t *ctx = (cor_mongo_t *) cor_pool_calloc(pool, sizeof(cor_mongo_t));
    if (!ctx) {
        cor_log_error(log, "can't cor_pool_calloc");
        if (delete_pool) {
            cor_pool_delete(pool);
        }
        return NULL;
    }
    ctx->pool = pool;
    ctx->delete_pool = delete_pool;
    ctx->log = log;
    ctx->loop = loop;
    ctx->docs_count_min = COR_MONGO_RESULT_COUNT_MIN;
    ctx->docs_count_max = COR_MONGO_RESULT_COUNT_MAX;
    /**/
    if (pthread_mutex_init(&ctx->thread_mutex, NULL) != 0) {
        cor_log_error(log, "can't pthread_mutex_init");
        cor_mongo_delete(ctx);
        return NULL;
    }
    if (pthread_cond_init(&ctx->thread_cond, NULL) != 0) {
        cor_log_error(log, "can't pthread_cond_init");
        cor_mongo_delete(ctx);
        return NULL;
    }
    /**/
    mongoc_init();
    mongoc_uri_t *mongo_uri = mongoc_uri_new(uri);
    if (!mongo_uri) {
        cor_log_error(log, "can't mongoc_uri_new");
        mongoc_cleanup();
        cor_mongo_delete(ctx);
        return NULL;
    }
    ctx->client_pool = mongoc_client_pool_new(mongo_uri);
    if (!ctx->client_pool) {
        cor_log_error(log, "can't mongoc_client_pool_new");
        mongoc_uri_destroy(mongo_uri);
        mongoc_cleanup();
        cor_mongo_delete(ctx);
        return NULL;
    }
    /**/
    ev_async_init(&ctx->response_queue_signal, cor_mongo_response_queue_cb);
    ctx->response_queue_signal.data = (void *) ctx;
    ev_async_start(ctx->loop, &ctx->response_queue_signal);
    /**/
    ctx->threads = (pthread_t *) cor_pool_alloc(ctx->pool, sizeof(pthread_t) * threads_count);
    if (!ctx->threads) {
        cor_log_error(log, "can't cor_pool_alloc");
        mongoc_uri_destroy(mongo_uri);
        mongoc_cleanup();
        cor_mongo_delete(ctx);
        return NULL;
    }
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    for (int i = 0; i < threads_count; ++i) {
        pthread_create(&ctx->threads[i], &attr, cor_mongo_loop, ctx);
    }
    pthread_attr_destroy(&attr);
    ctx->threads_count = threads_count;
    /**/
    mongoc_uri_destroy(mongo_uri);

    return ctx;
}

void
cor_mongo_delete(cor_mongo_t *ctx)
{
    if (ctx) {
        if (ctx->threads_count) {
            ctx->is_exit = 1;
            pthread_mutex_lock(&ctx->thread_mutex);
            pthread_cond_broadcast(&ctx->thread_cond);
            pthread_mutex_unlock(&ctx->thread_mutex);
            for (int i = 0; i < ctx->threads_count; ++i) {
                pthread_join(ctx->threads[i], NULL);
            }
        }
        if (ev_is_active(&ctx->response_queue_signal)) {
            ev_async_stop(ctx->loop, &ctx->response_queue_signal);
        }
        if (ctx->free_requests) {
            for (cor_mongo_request_t *r = ctx->free_requests; r; r = r->next) {
                if (r->docs) {
                    free(r->docs);
                }
            }
        }
        mongoc_cleanup();
        if (ctx->pool && ctx->delete_pool) {
            cor_pool_delete(ctx->pool);
        }
    }
}

int
cor_mongo_gets(cor_mongo_t *ctx, const char *db, const char *collection, const bson_t *query, 
    const bson_t *fields, int skip, int limit, cor_mongo_cb_t *cb, void *arg)
{
    cor_mongo_request_t *r = cor_mongo_request_new(ctx);
    if (!r) {
        cor_log_error(ctx->log, "can't cor_mongo_request_new");
        return cor_error;
    }
    r->cb = cb;
    r->arg = arg;
    r->db = db;
    r->collection = collection;
    r->cmd = COR_MONGO_CMD_GETS;
    r->docs_skip = skip;
    if (limit > ctx->docs_count_max) {
        limit = ctx->docs_count_max;
    }
    r->docs_limit = limit ? limit : ctx->docs_count_max;
    bson_copy_to(query, &r->query);
    if (fields) {
        bson_copy_to(fields, &r->fields);
        r->fields_limited = 1;
    }
    cor_mongo_queue_push(&ctx->request_queue, r);
    pthread_mutex_lock(&ctx->thread_mutex);
    pthread_cond_signal(&ctx->thread_cond);
    pthread_mutex_unlock(&ctx->thread_mutex);

    return cor_ok;
}

int
cor_mongo_set(cor_mongo_t *ctx, const char *db, const char *collection, const bson_t *data,
    cor_mongo_cb_t *cb, void *arg)
{
    cor_mongo_request_t *r = cor_mongo_request_new(ctx);
    if (!r) {
        cor_log_error(ctx->log, "can't cor_mongo_request_new");
        return cor_error;
    }
    r->cb = cb;
    r->arg = arg;
    r->db = db;
    r->collection = collection;
    r->cmd = COR_MONGO_CMD_SET;
    bson_copy_to(data, &r->query);
    cor_mongo_queue_push(&ctx->request_queue, r);
    pthread_mutex_lock(&ctx->thread_mutex);
    pthread_cond_signal(&ctx->thread_cond);
    pthread_mutex_unlock(&ctx->thread_mutex);

    return cor_ok;
}

static void *
cor_mongo_loop(void *arg)
{
    cor_mongo_t *ctx = (cor_mongo_t *) arg;
    pthread_mutex_lock(&ctx->thread_mutex);
    while (!ctx->is_exit) {
        pthread_cond_wait(&ctx->thread_cond, &ctx->thread_mutex);
        /*  get request  */
        while (1) {
            /*  get request from request queue  */
            cor_mongo_request_t *r = cor_mongo_queue_pop(&ctx->request_queue);
            if (!r) {
                break;
            }
            /*  perform request  */
            switch (r->cmd) {
                case COR_MONGO_CMD_GETS:
                    r->status = cor_mongo_gets_query(ctx, r);
                    break;
                case COR_MONGO_CMD_SET:
                    r->status = cor_mongo_set_query(ctx, r);
                    break;
            }
            /*  put request to response queue  */
            cor_mongo_queue_push(&ctx->response_queue, r);
            ev_async_send(ctx->loop, &ctx->response_queue_signal);
        }
    }
    pthread_mutex_unlock(&ctx->thread_mutex);

    return NULL;
}

static void
cor_mongo_response_queue_cb(struct ev_loop *loop, ev_async *w, int revents)
{
    cor_mongo_t *ctx = (cor_mongo_t *) w->data;
    while (1) {
        cor_mongo_request_t *r = cor_mongo_queue_pop(&ctx->response_queue);
        if (!r) {
            break;
        }
        if (r->cb) {
            switch (r->cmd) {
                case COR_MONGO_CMD_GETS:
                    r->cb(r->status, r->docs, r->docs_count, r->arg);
                    break;
                case COR_MONGO_CMD_SET:
                    r->cb(r->status, NULL, 0, r->arg);
                    break;
            }
        }
        cor_mongo_request_delete(ctx, r);
    }
}

static inline cor_mongo_request_t *
cor_mongo_request_new(cor_mongo_t *ctx)
{
    cor_mongo_request_t *r;
    if (ctx->free_requests) {
        r = ctx->free_requests;
        ctx->free_requests = r->next;
        int docs_count_max = r->docs_count_max;
        bson_t *docs = r->docs;
        memset(r, 0, sizeof(cor_mongo_request_t));
        r->docs_count_max = docs_count_max;
        r->docs = docs;
    } else {
        r = (cor_mongo_request_t *) cor_pool_calloc(ctx->pool, sizeof(cor_mongo_request_t));;
        if (!r) {
            cor_log_error(ctx->log, "can't cor_pool_alloc");
            return NULL;
        }
    }

    return r;
}

static inline void
cor_mongo_request_delete(cor_mongo_t *ctx, cor_mongo_request_t *r)
{
    bson_destroy(&r->query);
    for (int i = 0; i < r->docs_count; ++i) {
        bson_destroy(&r->docs[i]);
    }
    r->next = ctx->free_requests;
    ctx->free_requests = r;
}

static inline int
cor_mongo_doc_append(cor_mongo_t *ctx, cor_mongo_request_t *r, const bson_t *doc)
{
    if (r->docs_count == r->docs_count_max) {
        if (r->docs_count_max == r->docs_limit) {
            cor_log_error(ctx->log, "docs_count_max limited by %d", r->docs_limit);
            return cor_error;
        }
        if (!r->docs_count_max) {
            r->docs = (bson_t *) malloc(sizeof(bson_t) * ctx->docs_count_min);
            if (!r->docs) {
                cor_log_error(ctx->log, "can't malloc");
                return cor_error;
            }
            r->docs_count_max = ctx->docs_count_min;
        } else {
            r->docs_count_max = r->docs_count_max * 2 - (r->docs_count_max / 2);
            if (r->docs_count_max > r->docs_limit) {
                r->docs_count_max = r->docs_limit;
            }
        }
    }
    bson_copy_to(doc, &r->docs[r->docs_count++]);

    return cor_ok;
}

static inline void
cor_mongo_queue_push(cor_mongo_queue_t *q, cor_mongo_request_t *r)
{
    pthread_mutex_lock(&q->mutex);
    if (q->head) {
        q->tail->next = r;
        q->tail = r;
    } else {
        q->head = q->tail = r;
    }
    r->next = NULL;
    pthread_mutex_unlock(&q->mutex);
}

static inline cor_mongo_request_t *
cor_mongo_queue_pop(cor_mongo_queue_t *q)
{
    pthread_mutex_lock(&q->mutex);
    cor_mongo_request_t *r = q->head;
    if (!r) {
        pthread_mutex_unlock(&q->mutex);
        return NULL;
    }
    if (r->next) {
        q->head = r->next;
    } else {
        q->head = q->tail = NULL;
    }
    pthread_mutex_unlock(&q->mutex);

    return r;
}

static int
cor_mongo_gets_query(cor_mongo_t *ctx, cor_mongo_request_t *r)
{
    mongoc_client_t *client = mongoc_client_pool_pop(ctx->client_pool);
    if (!client) {
        cor_log_error(ctx->log, "can't mongoc_client_pool_push");
        return cor_error;
    }
    mongoc_collection_t *collection = mongoc_client_get_collection(client, r->db, r->collection);
    if (!collection) {
        cor_log_error(ctx->log, "can't mongoc_client_get_collection");
        mongoc_client_pool_push(ctx->client_pool, client);
        return cor_error;
    }
    /**/
    mongoc_cursor_t *cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, 0, 0, 
        &r->query, r->fields_limited ? &r->fields : NULL, NULL);
    if (!cursor) {
        cor_log_error(ctx->log, "can't mongoc_collection_find");
        mongoc_collection_destroy(collection);
        mongoc_client_pool_push(ctx->client_pool, client);
        return cor_error;
    }
    const bson_t *doc;
    while (mongoc_cursor_next(cursor, &doc)) {
        if (cor_mongo_doc_append(ctx, r, doc) != cor_ok) {
            cor_log_error(ctx->log, "can't cor_mongo_doc_append");
            return cor_error;
        }
    }
    bson_error_t error;
    if (mongoc_cursor_error(cursor, &error)) {
        cor_log_error(ctx->log, "can't mongoc_collection_find, error: %s", error.message);
        mongoc_cursor_destroy(cursor);
        mongoc_collection_destroy(collection);
        mongoc_client_pool_push(ctx->client_pool, client);
        return cor_error;
    }
    mongoc_cursor_destroy(cursor);
    mongoc_collection_destroy(collection);
    mongoc_client_pool_push(ctx->client_pool, client);

    return cor_ok;
}

static int
cor_mongo_set_query(cor_mongo_t *ctx, cor_mongo_request_t *r)
{
    mongoc_client_t *client = mongoc_client_pool_pop(ctx->client_pool);
    if (!client) {
        cor_log_error(ctx->log, "can't mongoc_client_pool_push");
        return cor_error;
    }
    mongoc_collection_t *collection = mongoc_client_get_collection(client, r->db, r->collection);
    if (!collection) {
        cor_log_error(ctx->log, "can't mongoc_client_get_collection");
        mongoc_client_pool_push(ctx->client_pool, client);
        return cor_error;
    }
    /**/
    bson_error_t error;
    bool rc = mongoc_collection_save(collection, &r->query, 0, &error);
    if (!rc) {
        cor_log_error(ctx->log, "can't mongoc_collection_insert, error: %s", error.message);
        mongoc_collection_destroy(collection);
        mongoc_client_pool_push(ctx->client_pool, client);
        return cor_error;
    }

    return cor_ok;
}

