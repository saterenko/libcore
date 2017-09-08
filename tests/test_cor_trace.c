#include "cor_test.h"
#include "cor_trace.h"
#include <string.h>

BEGIN_TEST(test_cor_trace)
{
    cor_trace_t *t = cor_trace_new(256);
    test_ptr_ne(t, NULL);
    cor_trace(t, "test error");
    const char *p = cor_trace_get(t);
    test_ptr_ne(strstr(p, "test_cor_trace.c:9 test error"), NULL);
    /**/
    char msg[3000];
    static char *chars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    size_t chars_len = strlen(chars);
    for (int i = 0; i < 2999; i++) {
        msg[i] = chars[rand() % chars_len];
    }
    msg[2999] = '\0';
    cor_trace(t, msg);
    p = cor_trace_get(t);
    test_ptr_ne(strstr(p, msg), NULL);

    cor_trace_delete(t);
}
END;

int
main(int argc, char **argv)
{
    RUN_TEST(test_cor_trace);

    exit(0);
}
