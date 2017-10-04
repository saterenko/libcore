#include "cor_test.h"
#include "cor_thread.h"

#include <unistd.h>

#define THREADS_COUNT 4

static int test_threads_init;
static int test_threads_shutdown;

void
test_thread_on_init(cor_thread_t *t, void *arg)
{
    __sync_fetch_and_add(&test_threads_init, 1);
}

void
test_thread_on_shutdown(cor_thread_t *t, void *arg)
{
    __sync_fetch_and_add(&test_threads_shutdown, 1);
}

BEGIN_TEST(test_cor_thread)
{
    test_threads_init = 0;
    test_threads_shutdown = 0;
    cor_thread_t *threads[4];
    for (int i = 0; i < 4; i++) {
        threads[i] = cor_thread_new(test_thread_on_init, test_thread_on_shutdown, NULL);
        test_ptr_ne(threads[i], NULL);
    }
    usleep(100000);
    test_int_eq(test_threads_init, 4);
    for (int i = 0; i < 4; i++) {
        cor_thread_delete(threads[i]);
    }
    test_int_eq(test_threads_shutdown, 4);
}
END;

int
main(int argc, char **argv)
{
    RUN_TEST(test_cor_thread);

    exit(0);
}
