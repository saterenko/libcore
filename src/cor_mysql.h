#ifndef COR_MYSQL_H
#define COR_MYSQL_H

#include <stdarg.h>
#include <mysql.h>

#define COR_MYSQL_ERROR_SIZE 256

typedef struct
{
    char *host;
    int port;
    char *db;
    char *usr;
    char *pwd;
    char *query;
    size_t query_size;
    int retry_count;
    size_t max_query_size;
    char *charset;
    MYSQL *conn;
    char error[COR_MYSQL_ERROR_SIZE + 1];
    unsigned connected:1;
} cor_mysql_t;

typedef struct
{
    MYSQL_RES *res;
    cor_mysql_t *mysql;
} cor_mysql_res_t;

cor_mysql_t *cor_mysql_new(const char *host, int port, const char *db, const char *usr, const char *pwd);
void cor_mysql_delete(cor_mysql_t *m);
void cor_mysql_set_retry_count(cor_mysql_t *m, int retry_count);
void cor_mysql_set_max_query_size(cor_mysql_t *m, size_t max_query_size);
void cor_mysql_set_charset(cor_mysql_t *m, const char *charset);
MYSQL_RES *cor_mysql_query(cor_mysql_t *m, const char *query, ...);
int cor_mysql_update(cor_mysql_t *m, const char *query, ...);
char **cor_mysql_row(MYSQL_RES *res);
void cor_mysql_res_free(MYSQL_RES *res);
int cor_mysql_affected_rows(cor_mysql_t *m);

#endif
