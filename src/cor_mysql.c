#include <stdlib.h>
#include <string.h>

#include "cor_mysql.h"
#include <errmsg.h>
#include <stdio.h>

#define COR_MYSQL_QUERY_SIZE 1024
#define COR_MYSQL_DEFAULT_RETRY_COUNT 2
#define COR_MYSQL_DEFAULT_MAX_QUERY_SIZE 1024 * 1024

static int cor_mysql_connect(cor_mysql_t *m);

cor_mysql_t *
cor_mysql_new(const char *host, int port, const char *db, const char *usr, const char *pwd)
{
    cor_mysql_t *m = (cor_mysql_t *) malloc(sizeof(cor_mysql_t));
    if (!m) {
        printf("%d\n", __LINE__);
        return NULL;
    }
    memset(m, 0, sizeof(cor_mysql_t));
    m->retry_count = COR_MYSQL_DEFAULT_RETRY_COUNT;
    m->max_query_size = COR_MYSQL_DEFAULT_MAX_QUERY_SIZE;
    /**/
    m->port = port;
    if (host) {
        m->host = strdup(host);
        if (!m->host) {
            cor_mysql_delete(m);
            return NULL;
        }
    }
    if (db) {
        m->db = strdup(db);
        if (!m->db) {
            cor_mysql_delete(m);
            return NULL;
        }
    }
    if (usr) {
        m->usr = strdup(usr);
        if (!m->usr) {
            cor_mysql_delete(m);
            return NULL;
        }
    }
    if (pwd) {
        m->pwd = strdup(pwd);
        if (!m->pwd) {
            cor_mysql_delete(m);
            return NULL;
        }
    }
    m->query_size = COR_MYSQL_QUERY_SIZE;
    m->query = (char *) malloc(m->query_size);
    if (!m->query) {
        cor_mysql_delete(m);
        return NULL;
    }
    m->conn = mysql_init(NULL);
    if (!m->conn) {
        cor_mysql_delete(m);
        return NULL;
    }
    return m;
}

void
cor_mysql_delete(cor_mysql_t *mysql)
{
    if (mysql) {
        if (mysql->conn) {
            mysql_close(mysql->conn);
        }
        if (mysql->query) {
            free(mysql->query);
        }
        if (mysql->host) {
            free(mysql->host);
        }
        if (mysql->db) {
            free(mysql->db);
        }
        if (mysql->usr) {
            free(mysql->usr);
        }
        if (mysql->pwd) {
            free(mysql->pwd);
        }
    }
}

void
cor_mysql_set_retry_count(cor_mysql_t *m, int retry_count)
{
    m->retry_count = retry_count;
}

void
cor_mysql_set_max_query_size(cor_mysql_t *m, size_t max_query_size)
{
    m->max_query_size = max_query_size;
}

void
cor_mysql_set_charset(cor_mysql_t *m, const char *charset)
{
    m->charset = strdup(charset);
}

#define COR_MYSQL_MAKE_QUERY(_ret_in_error) \
    va_list args; \
    int query_len; \
    while (1) { \
        va_start(args, query); \
        int rc = vsnprintf(m->query, m->query_size, query, args); \
        va_end(args); \
        if (rc < 0) { \
            snprintf(m->error, COR_MYSQL_ERROR_SIZE, "can't vsnprintf"); \
            return _ret_in_error; \
        } \
        if (rc >= m->query_size) { \
            size_t size = m->query_size; \
            while (size <= rc) { \
                size = (size << 1) - (size >> 1); \
            } \
            if (rc > m->max_query_size) { \
                snprintf(m->error, COR_MYSQL_ERROR_SIZE, "max_query_size exceed: %zu > %zu", size, m->max_query_size); \
                return _ret_in_error; \
            } \
            char *nb = (char *) malloc(size); \
            if (!nb) { \
                snprintf(m->error, COR_MYSQL_ERROR_SIZE, "can't malloc"); \
                return _ret_in_error; \
            } \
            free(m->query); \
            m->query = nb; \
            m->query_size = size; \
            continue; \
        } \
        query_len = rc; \
        break; \
    }

MYSQL_RES *
cor_mysql_query(cor_mysql_t *m, const char *query, ...)
{
    if (!m->connected) {
        if (cor_mysql_connect(m)) {
            return NULL;
        }
    }
    COR_MYSQL_MAKE_QUERY(NULL);
    for (int i = 0; i < m->retry_count; i++) {
        if (mysql_real_query(m->conn, m->query, query_len)) {
            int error = mysql_errno(m->conn);
            if (error == CR_UNKNOWN_ERROR || error == CR_SERVER_LOST || error == CR_SERVER_GONE_ERROR) {
                if (cor_mysql_connect(m) == 0) {
                    continue;
                }
            } else {
                snprintf(m->error, COR_MYSQL_ERROR_SIZE, "can't mysql_real_query, error: %s (%d)", mysql_error(m->conn), mysql_errno(m->conn));
                return NULL;
            }
        }
        MYSQL_RES *res = mysql_store_result(m->conn);
        if (!res) {
            snprintf(m->error, COR_MYSQL_ERROR_SIZE, "can't mysql_store_result, error: %s (%d)", mysql_error(m->conn), mysql_errno(m->conn));
            return NULL;
        }
        return res;
    }
    return NULL;
}

int
cor_mysql_update(cor_mysql_t *m, const char *query, ...)
{
    if (!m->connected) {
        if (cor_mysql_connect(m)) {
            return -1;
        }
    }
    COR_MYSQL_MAKE_QUERY(-1);
    for (int i = 0; i < m->retry_count; i++) {
        if (mysql_real_query(m->conn, m->query, query_len)) {
            int error = mysql_errno(m->conn);
            if (error == CR_UNKNOWN_ERROR || error == CR_SERVER_LOST || error == CR_SERVER_GONE_ERROR) {
                if (cor_mysql_connect(m) == 0) {
                    continue;
                }
            } else {
                snprintf(m->error, COR_MYSQL_ERROR_SIZE, "can't mysql_real_query, error: %s (%d)", mysql_error(m->conn), mysql_errno(m->conn));
                return -1;
            }
        }
        return 0;
    }
    return -1;
}

char **
cor_mysql_row(MYSQL_RES *res)
{
    return mysql_fetch_row(res);
}

void
cor_mysql_res_free(MYSQL_RES *res)
{
    mysql_free_result(res);
}

int
cor_mysql_connect(cor_mysql_t *m)
{
    if (!mysql_real_connect(m->conn, m->host, m->usr, m->pwd, m->db, m->port, NULL, 0)) {
        snprintf(m->error, COR_MYSQL_ERROR_SIZE, "can't mysql_real_connect, error: %s (%d)", mysql_error(m->conn), mysql_errno(m->conn));
        m->connected = 0;
        return -1;
    }
    if (m->charset) {
        if (cor_mysql_update(m, "set names '%s'", m->charset) != 0) {
            m->connected = 0;
            return -1;
        }
    }
    m->connected = 1;
    return 0;
}

int
cor_mysql_affected_rows(cor_mysql_t *m)
{
    return mysql_affected_rows(m->conn);
}
