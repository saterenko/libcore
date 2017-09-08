#include <fcntl.h>
#include <sys/stat.h>

#include "cor_test.h"
#include "cor_list.h"

BEGIN_TEST(test_cor_list)
{
    cor_list_t *l = cor_list_new(4, 32);
    test_ptr_ne(l, NULL);
    test_int_eq(l->nelts, 4);
    test_int_eq(l->size, 32);
    for (int i = 0; i < 100; i++) {
        void *p = cor_list_append(l);
        test_ptr_ne(p, NULL);
    }
    test_int_eq(cor_list_nelts(l), 100);
    cor_list_delete(l);
}
END;

int
main(int argc, char **argv)
{
    RUN_TEST(test_cor_list);

    exit(0);
}
