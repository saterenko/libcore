#include "cor_test.h"
#include "cor_spider.h"
#include "cor_http.h"

static int get_url_status = 0;

int
check_get_url_status(cor_buf_chain_t *bufs, int status, long http_code, void *arg)
{
    test_int_eq(status, COR_SPIDER_OK);
    test_int_eq(http_code, 200);
    test_ptr_ne(arg, NULL);
    test_ptr_ne(bufs, NULL);
    test_int_eq(bufs->count, 1);
    test_ptr_ne(bufs->head, NULL);
    test_ptr_ne(bufs->head->begin, NULL);

    test_int_ne(bufs->count, 0);

    return 0;
}

void
test_cor_spider_get_url_cb(cor_str_t *url, cor_buf_chain_t *bufs, int status, long http_code, void *arg)
{
    cor_spider_t *spider = (cor_spider_t *) arg;
    get_url_status = check_get_url_status(bufs, status, http_code, arg);
    ev_break(spider->loop, EVBREAK_ALL);
}

void
test_cor_spider_http_cb(cor_http_request_t *r, void *arg)
{
    cor_http_response_t res;
    cor_http_response_init(&res, r);
    cor_http_response_set_code(&res, 200);
    cor_http_response_set_body(&res, "answer", 6);
    cor_http_response_send(&res);
}

BEGIN_TEST(test_cor_spider_get_url_content)
{
    const char url[] = "http://127.0.0.1:8000/test";
    struct ev_loop *loop = EV_DEFAULT;
    cor_spider_t *spider = cor_spider_new(loop, NULL, 8, 32768);
    test_ptr_ne(spider, NULL);
    cor_http_t *http = cor_http_new(loop, "127.0.0.1", 8000, NULL, NULL); 
    test_ptr_ne(http, NULL);
    int rc = cor_http_start(http, test_cor_spider_http_cb, NULL);
    test_int_eq(rc, cor_ok);
    rc = cor_spider_get_url_content(spider, url, strlen(url), test_cor_spider_get_url_cb, spider);
    test_int_eq(rc, cor_ok);
    ev_run(loop, 0);
    test_int_eq(get_url_status, 0);
    cor_spider_delete(spider);
    cor_http_delete(http);
}
END;

int
main(int argc, char **argv)
{
    RUN_TEST(test_cor_spider_get_url_content);

    exit(0);
}
