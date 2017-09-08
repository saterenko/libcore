#include "cor_spider.h"

static int cor_spider_set_conn_url(cor_spider_conn_t *conn, const char *url, int url_size);
static cor_spider_conn_t *cor_spider_get_conn(cor_spider_t *spider);
static void cor_spider_free_conn(cor_spider_conn_t *conn);
static void cor_spider_curl_check_done(cor_spider_t *spider);
static size_t cor_spider_curl_write_cb(void *ptr, size_t size, size_t nmemb, void *userdata);
static int cor_spider_curl_socket_cb(CURL *e, curl_socket_t sd, int what, void *userp, void *socketp);
static void cor_spider_curl_event_cb(struct ev_loop *loop, ev_io *w, int revents);
static int cor_spider_curl_timer_set_cb(CURL *e, long timeout, void *userp);
static void cor_spider_curl_timer_cb(struct ev_loop *loop, ev_timer *w, int revents);

cor_spider_t *
cor_spider_new(struct ev_loop *loop, cor_log_t *log, int buf_count, int buf_size)
{
    if (!buf_count || !buf_size) {
        cor_log_error(log, "buf_count or buf_size is zero");
        return NULL;
    }
    cor_spider_t *spider = malloc(sizeof(cor_spider_t));
    if (!spider) {
        cor_log_error(log, "can't malloc");
        return NULL;
    }
    memset(spider, 0, sizeof(cor_spider_t));
    spider->buf_pool = cor_buf_pool_new(buf_count, buf_size);
    if (!spider->buf_pool) {
        cor_log_error(log, "can't cor_buf_pool_new");
        cor_spider_delete(spider);
        return NULL;
    }
    spider->conns = cor_list_new(buf_count, sizeof(cor_spider_conn_t));
    if (!spider->conns) {
        cor_log_error(log, "can't cor_list_new");
        cor_spider_delete(spider);
        return NULL;
    }
    /**/
    curl_global_init(CURL_GLOBAL_DEFAULT);
    spider->curl = curl_multi_init();
    if (!spider->curl) {
        cor_log_error(log, "can't curl_multi_init");
        cor_spider_delete(spider);
        return NULL;
    }
    curl_multi_setopt(spider->curl, CURLMOPT_SOCKETFUNCTION, cor_spider_curl_socket_cb);
    curl_multi_setopt(spider->curl, CURLMOPT_SOCKETDATA, spider);
    curl_multi_setopt(spider->curl, CURLMOPT_TIMERFUNCTION, cor_spider_curl_timer_set_cb);
    curl_multi_setopt(spider->curl, CURLMOPT_TIMERDATA, spider);
    /**/
    spider->loop = loop;
    spider->log = log;
    /**/
    ev_timer_init(&spider->curl_timer, cor_spider_curl_timer_cb, 0.0, 0.0);
    spider->curl_timer.data = spider;

    return spider;
}

void
cor_spider_delete(cor_spider_t *spider)
{
    if (spider) {
        if (spider->curl) {
            curl_multi_cleanup(spider->curl);
        }
        if (spider->conns) {
            cor_list_block_t *b;
            b = &spider->conns->root;
            cor_spider_conn_t *conns = (cor_spider_conn_t *) b->elts;
            for (int i = 0; ; i++) {
                if (i == b->nelts) {
                    if (!b->next) {
                        break;
                    }
                    b = b->next;
                    conns = (cor_spider_conn_t *) b->elts;
                    i = 0; 
                }
                if (ev_is_active(&conns[i].ev)) {
                    ev_io_stop(spider->loop, &conns[i].ev);
                }
                if (ev_is_active(&conns[i].timer)) {
                    ev_timer_stop(spider->loop, &conns[i].timer);
                }
                if (conns[i].easy_active) {
                    curl_multi_remove_handle(spider->curl, conns[i].easy);
                    conns[i].easy_active = 0;
                }
                if (conns[i].url.data) {
                    free(conns[i].url.data);
                }
            }
            cor_list_delete(spider->conns);
        }
        if (spider->buf_pool) {
            cor_buf_pool_delete(spider->buf_pool);
        }
        free(spider);
    }
}

int
cor_spider_get_url_content(cor_spider_t *spider, const char *url, int url_size, cor_spider_cb_t *cb, void *arg)
{
    cor_spider_conn_t *conn = cor_spider_get_conn(spider);
    if (!conn) {
        cor_log_error(spider->log, "can't cor_spider_get_conn");
        return cor_error;
    }
    if (cor_spider_set_conn_url(conn, url, url_size) != cor_ok) {
        cor_log_error(spider->log, "can't cor_spider_set_conn_url");
        return cor_error;
    }
    conn->cb = cb;
    conn->arg = arg;
    /**/
    curl_easy_setopt(conn->easy, CURLOPT_URL, conn->url.data);
    CURLMcode rc = curl_multi_add_handle(spider->curl, conn->easy);
    if (rc != CURLM_OK) {
        cor_log_error(spider->log, "can't curl_multi_add_handle");
        return cor_error;
    }
    conn->easy_active = 1;

    return cor_ok;
}

static int
cor_spider_set_conn_url(cor_spider_conn_t *conn, const char *url, int url_size)
{
    if (conn->url_buf_size) {
        if (conn->url_buf_size <= url_size) {
            char *nb = realloc(conn->url.data, url_size + 1);
            if (!nb) {
                cor_log_error(conn->spider->log, "can't realloc");
                return cor_error;
            }
            conn->url_buf_size = url_size + 1;
        }
    } else {
        conn->url.data = malloc(url_size + 1);
        if (!conn->url.data) {
            cor_log_error(conn->spider->log, "can't malloc");
            return cor_error;
        }
        conn->url_buf_size = url_size + 1;
    }
    memcpy(conn->url.data, url, url_size);
    conn->url.data[url_size] = '\0';
    conn->url.size = url_size;

    return cor_ok;
}

cor_spider_conn_t *
cor_spider_get_conn(cor_spider_t *spider)
{
    cor_spider_conn_t *conn;
    if (spider->free_conns) {
        conn = spider->free_conns;
        spider->free_conns = conn->next;
        CURL *easy = conn->easy;
        memset(conn, 0, sizeof(cor_spider_conn_t));
        conn->easy = easy;
    } else {
        conn = (cor_spider_conn_t *) cor_list_append(spider->conns);
        memset(conn, 0, sizeof(cor_spider_conn_t));
        conn->easy = curl_easy_init();
        if (!conn->easy) {
            cor_log_error(spider->log, "can't curl_easy_init");
            return NULL;
        }
        curl_easy_setopt(conn->easy, CURLOPT_WRITEFUNCTION, cor_spider_curl_write_cb);
        curl_easy_setopt(conn->easy, CURLOPT_WRITEDATA, conn);
        curl_easy_setopt(conn->easy, CURLOPT_VERBOSE, 0L);
        curl_easy_setopt(conn->easy, CURLOPT_ERRORBUFFER, conn->error);
        curl_easy_setopt(conn->easy, CURLOPT_PRIVATE, conn);
        curl_easy_setopt(conn->easy, CURLOPT_LOW_SPEED_TIME, 3L);
        curl_easy_setopt(conn->easy, CURLOPT_LOW_SPEED_LIMIT, 10L);
        curl_easy_setopt(conn->easy, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(conn->easy, CURLOPT_CONNECTTIMEOUT, 5L);
    }
    conn->fd = -1;
    conn->spider = spider;

    return conn;
}

void
cor_spider_free_conn(cor_spider_conn_t *conn)
{
    if (conn->bufs.count) {
        cor_buf_chain_free(conn->spider->buf_pool, &conn->bufs);
    }
    conn->next = conn->spider->free_conns;
    conn->spider->free_conns = conn;
}

void
cor_spider_curl_check_done(cor_spider_t *spider)
{
    int left;
    CURLMsg *msg;
    while ((msg = curl_multi_info_read(spider->curl, &left))) {
        if (msg->msg == CURLMSG_DONE) {
            CURL* easy = msg->easy_handle;
            CURLcode rc = msg->data.result;
            cor_spider_conn_t *conn;
            long http_code;
            curl_easy_getinfo(easy, CURLINFO_PRIVATE, &conn);
            curl_easy_getinfo(easy, CURLINFO_HTTP_CODE, &http_code);
            curl_multi_remove_handle(spider->curl, easy);
            conn->easy_active = 0;
            if (ev_is_active(&conn->ev)) {
                ev_io_stop(spider->loop, &conn->ev);
            }
            if (conn->cb) {
                int status = rc == CURLE_OK ? COR_SPIDER_OK : COR_SPIDER_ERROR;
                conn->cb(&conn->url, &conn->bufs, status, http_code, conn->arg);
            }
            cor_spider_free_conn(conn);
        }
    }
}

size_t
cor_spider_curl_write_cb(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    cor_spider_conn_t *conn = (cor_spider_conn_t *) userdata;
    cor_buf_chain_append_data(conn->spider->buf_pool, &conn->bufs, ptr, size * nmemb);

    return size * nmemb;
}

int
cor_spider_curl_socket_cb(CURL *e, curl_socket_t sd, int what, void *userp, void *socketp)
{
    cor_spider_t *spider = (cor_spider_t *) userp;
    cor_spider_conn_t *conn = (cor_spider_conn_t *) socketp;
    if (what == CURL_POLL_REMOVE) {
        if (ev_is_active(&conn->ev)) {
            ev_io_stop(spider->loop, &conn->ev);
        }
    } else {
        if (!conn) {
            curl_easy_getinfo(e, CURLINFO_PRIVATE, &conn);
            conn->flags = (what & CURL_POLL_IN ? EV_READ : 0) | (what & CURL_POLL_OUT ? EV_WRITE : 0);
            if (ev_is_active(&conn->ev)) {
                ev_io_stop(spider->loop, &conn->ev);
            }
            ev_io_init(&conn->ev, cor_spider_curl_event_cb, sd, conn->flags);
            conn->ev.data = conn;
            ev_io_start(spider->loop, &conn->ev);
            curl_multi_assign(spider->curl, sd, conn);
        } else {
            int flags = (what & CURL_POLL_IN ? EV_READ : 0) | (what & CURL_POLL_OUT ? EV_WRITE : 0);
            if (conn->flags != flags) {
                if (ev_is_active(&conn->ev)) {
                    ev_io_stop(spider->loop, &conn->ev);
                }
                conn->flags = flags;
                ev_io_set(&conn->ev, sd, conn->flags);
                ev_io_start(spider->loop, &conn->ev);
            }
        }
    }

    return 0;
}

void
cor_spider_curl_event_cb(struct ev_loop *loop, ev_io *w, int revents)
{
    cor_spider_conn_t *conn = (cor_spider_conn_t *) w->data;
    cor_spider_t *spider = conn->spider;
    int left;
    int flags = (revents & EV_READ ? CURL_POLL_IN : 0) | (revents & EV_WRITE ? CURL_POLL_OUT : 0);
    CURLMcode rc = curl_multi_socket_action(spider->curl, w->fd, flags, &left);
    if (rc != CURLM_OK) {
        cor_log_error(spider->log, "can't curl_multi_socket_action");
    }
    cor_spider_curl_check_done(spider);
    if (left <= 0 && ev_is_active(&spider->curl_timer)) {
        ev_timer_stop(spider->loop, &spider->curl_timer);
    }
}

int
cor_spider_curl_timer_set_cb(CURL *e, long timeout, void *userp)
{
    cor_spider_t *spider = (cor_spider_t *) userp;
    if (ev_is_active(&spider->curl_timer)) {
        return 0;
    }
    if (timeout <= 0) {
        timeout = 1;
    }
    ev_timer_set(&spider->curl_timer, (double) timeout / 1000.0, 0.0);
    ev_timer_start(spider->loop, &spider->curl_timer);

    return 0;
}

void
cor_spider_curl_timer_cb(struct ev_loop *loop, ev_timer *w, int revents)
{
    cor_spider_t *spider = (cor_spider_t *) w->data;
    if (ev_is_active(&spider->curl_timer)) {
        ev_timer_stop(spider->loop, &spider->curl_timer);
    }
    int left;
    CURLMcode rc = curl_multi_socket_action(spider->curl, CURL_SOCKET_TIMEOUT, 0, &left);
    if (rc != CURLM_OK) {
        cor_log_error(spider->log, "can't curl_multi_socket_action");
    }
    cor_spider_curl_check_done(spider);
}


