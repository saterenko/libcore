#include "cor_test.h"
#include "cor_spider.h"
#include "../src/cor_http.c"

// TODO check for two request in sequency

int
test_cor_http_parse_check_param(cor_http_request_t *r, const char *key, const char *value)
{
    cor_str_t *str = cor_http_request_get_param(r, key, strlen(key));
    test_ptr_ne(str, NULL);
    int value_len =  strlen(value);
    if (value_len) {
        test_strn_eq(str->data, value, value_len);
    }

    return 0;
}

int
test_cor_http_parse_check_header(cor_http_request_t *r, const char *key, const char *value)
{
    cor_str_t *str = cor_http_request_get_header(r, key, strlen(key));
    test_ptr_ne(str, NULL);
    int value_len =  strlen(value);
    if (value_len) {
        test_strn_eq(str->data, value, value_len);
    }

    return 0;
}

int
test_cor_http_parse_check_cookie(cor_http_request_t *r, const char *key, const char *value)
{
    cor_str_t *str = cor_http_request_get_cookie(r, key, strlen(key));
    test_ptr_ne(str, NULL);
    int value_len =  strlen(value);
    if (value_len) {
        test_strn_eq(str->data, value, value_len);
    }

    return 0;
}

BEGIN_TEST(test_cor_http_parse)
{
    struct ev_loop *loop = EV_DEFAULT;
    cor_http_t *http = cor_http_new(loop, "127.0.0.1", 8000, NULL, NULL); 
    test_ptr_ne(http, NULL);
    /**/
    static char request[] = "GET /code?pid=123&gid=234&rid=345&ul=http%3A//saterenko.ru/testsbn1.html&ur= HTTP/1.1\r\n"
        "Host: saterenko.ru\r\n"
        "Cookie: _ads_yadro=W.6.90.M; _ads_uuid=932916f8f75fa52f; _ads_freq3=1427359869759.1427349680057.1427347858406\r\n\r\n";
    cor_http_request_t *r = cor_http_request_new(http);
    test_ptr_ne(r, NULL);
    int rc = cor_buf_chain_append_data(http->buf_pool, &r->read_bufs, request, strlen(request));
    test_int_eq(rc, cor_ok);
    rc = cor_http_parse(r);
    test_int_eq(rc, cor_ok);
    test_int_eq(r->http_major, 1);
    test_int_eq(r->http_minor, 1);
    test_int_eq(r->port, 0);
    test_int_eq(r->schema.size, 0);
    test_int_eq(r->host.size, 12);
    test_strn_eq(r->host.data, "saterenko.ru", (int) r->host.size);
    test_int_eq(r->path.size, 5);
    test_strn_eq(r->path.data, "/code",  (int) r->path.size);
    /**/
    test_int_eq(test_cor_http_parse_check_param(r, "pid", "123"), 0);
    test_int_eq(test_cor_http_parse_check_param(r, "gid", "234"), 0);
    test_int_eq(test_cor_http_parse_check_param(r, "rid", "345"), 0);
    test_int_eq(test_cor_http_parse_check_param(r, "ul", "http://saterenko.ru/testsbn1.html"), 0);
    test_int_eq(test_cor_http_parse_check_param(r, "ur", ""), 0);
    /**/
    test_int_eq(test_cor_http_parse_check_cookie(r, "_ads_yadro", "W.6.90.M"), 0);
    test_int_eq(test_cor_http_parse_check_cookie(r, "_ads_uuid", "932916f8f75fa52f"), 0);
    test_int_eq(test_cor_http_parse_check_cookie(r, "_ads_freq3", "1427359869759.1427349680057.1427347858406"), 0);
    cor_http_request_close(r);
    /**/
    static char request2[] = "GET http://saterenko.ru/code?pid=123&gid=234&rid=345&ul=http%3A//saterenko.ru/testsbn1.html&ur= HTTP/1.1\r\n"
        "Connection: keep-alive\r\n"
        "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_2) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/41.0.2272.104 Safari/537.36\r\n"
        "Accept:text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\n"
        "Cookie: _ads_yadro=W.6.90.M; _ads_uuid=932916f8f75fa52f; _ads_freq3=1427359869759.1427349680057.1427347858406\r\n\r\n";
    r = cor_http_request_new(http);
    test_ptr_ne(r, NULL);
    rc = cor_buf_chain_append_data(http->buf_pool, &r->read_bufs, request2, strlen(request2));
    test_int_eq(rc, cor_ok);
    rc = cor_http_parse(r);
    test_int_eq(rc, cor_ok);
    test_int_eq(r->http_major, 1);
    test_int_eq(r->http_minor, 1);
    test_int_eq(r->port, 0);
    test_int_eq(r->schema.size, 4);
    test_strn_eq(r->schema.data, "http", (int) r->schema.size);
    test_int_eq(r->host.size, 12);
    test_strn_eq(r->host.data, "saterenko.ru", (int) r->host.size);
    test_int_eq(r->path.size, 5);
    test_strn_eq(r->path.data, "/code",  (int) r->path.size);
    /**/
    test_int_eq(test_cor_http_parse_check_param(r, "pid", "123"), 0);
    test_int_eq(test_cor_http_parse_check_param(r, "gid", "234"), 0);
    test_int_eq(test_cor_http_parse_check_param(r, "rid", "345"), 0);
    test_int_eq(test_cor_http_parse_check_param(r, "ul", "http://saterenko.ru/testsbn1.html"), 0);
    test_int_eq(test_cor_http_parse_check_param(r, "ur", ""), 0);
    /**/
    test_int_eq(test_cor_http_parse_check_header(r, "connection", "keep-alive"), 0);
    test_int_eq(test_cor_http_parse_check_header(r, "user-agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_2) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/41.0.2272.104 Safari/537.36"), 0);
    test_int_eq(test_cor_http_parse_check_header(r, "accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8"), 0);
    /**/
    test_int_eq(test_cor_http_parse_check_cookie(r, "_ads_yadro", "W.6.90.M"), 0);
    test_int_eq(test_cor_http_parse_check_cookie(r, "_ads_uuid", "932916f8f75fa52f"), 0);
    test_int_eq(test_cor_http_parse_check_cookie(r, "_ads_freq3", "1427359869759.1427349680057.1427347858406"), 0);
    cor_http_request_close(r);
    /**/
    static char request3[] = "GET /js-scripts/240x400.html?pid=123&gid=345 HTTP/1.1\r\n"
        "Host: 127.0.0.1:8000\r\n"
        "Accept: */*\r\n\r\n";
    r = cor_http_request_new(http);
    test_ptr_ne(r, NULL);
    rc = cor_buf_chain_append_data(http->buf_pool, &r->read_bufs, request3, strlen(request3));
    test_int_eq(rc, cor_ok);
    rc = cor_http_parse(r);
    test_int_eq(rc, cor_ok);
    test_int_eq(r->http_major, 1);
    test_int_eq(r->http_minor, 1);
    test_int_eq(r->port, 8000);
    test_int_eq(r->host.size, strlen("127.0.0.1"));
    test_strn_eq(r->host.data, "127.0.0.1", (int) r->host.size);
    test_int_eq(r->path.size, strlen("/js-scripts/240x400.html"));
    test_strn_eq(r->path.data, "/js-scripts/240x400.html", (int) r->path.size);
    /**/
    test_int_eq(test_cor_http_parse_check_param(r, "pid", "123"), 0);
    test_int_eq(test_cor_http_parse_check_param(r, "gid", "345"), 0);
    /**/
    test_int_eq(test_cor_http_parse_check_header(r, "accept", "*/*"), 0);
    cor_http_request_close(r);
    cor_http_delete(http);
}
END;

static int get_url_status = 0;
static int http_request_status = 0;

int
test_cor_http_server_request(cor_http_request_t *r)
{
    test_int_eq(r->method, COR_HTTP_GET);
    test_int_eq(r->port, 8000);
    test_int_eq(r->host.size, strlen("127.0.0.1"));
    test_strn_eq(r->host.data, "127.0.0.1", (int) r->host.size);
    test_strn_eq(r->path.data, "/js-scripts/240x400.html", (int) r->path.size);
    test_int_eq(test_cor_http_parse_check_param(r, "pid", "123"), 0);
    test_int_eq(test_cor_http_parse_check_param(r, "gid", "345"), 0);

    return 0;
}

void
test_cor_http_server_cb(cor_http_request_t *r, void *arg)
{
    if (test_cor_http_server_request(r) == 0) {
        http_request_status = 1;
    }
    cor_http_response_t res;
    cor_http_response_init(&res, r);
    cor_http_response_set_code(&res, 200);
    cor_http_response_set_body(&res, "answer", 6);
    cor_http_response_send(&res);
}

void
test_cor_http_spider_cb(cor_str_t *url, cor_buf_chain_t *bufs, int status, long http_code, void *arg)
{
    cor_spider_t *spider = (cor_spider_t *) arg;
    if (status == cor_ok && http_code == 200 && bufs->head && bufs->head->begin
        && strncmp(bufs->head->begin, "answer", 6) == 0)
    {
        get_url_status = 1;
    }
    ev_break(spider->loop, EVBREAK_ALL);
}

BEGIN_TEST(test_cor_http_server)
{
    struct ev_loop *loop = EV_DEFAULT;
    cor_http_t *http = cor_http_new(loop, "127.0.0.1", 8000, NULL, NULL); 
    test_ptr_ne(http, NULL);
    cor_spider_t *spider = cor_spider_new(loop, NULL, 8, 32768);
    test_ptr_ne(spider, NULL);
    int rc = cor_http_start(http, test_cor_http_server_cb, http);
    test_int_eq(rc, cor_ok);
    const char url[] = "http://127.0.0.1:8000/js-scripts/240x400.html?pid=123&gid=345";
    rc = cor_spider_get_url_content(spider, url, strlen(url), test_cor_http_spider_cb, spider);
    test_int_eq(rc, cor_ok);
    ev_run(loop, 0);
    test_int_eq(http_request_status, 1);
    test_int_eq(get_url_status, 1);
    cor_spider_delete(spider);
    cor_http_delete(http);
}
END;

int
main(int argc, char **argv)
{
    RUN_TEST(test_cor_http_parse);
    RUN_TEST(test_cor_http_server);

    exit(0);
}
