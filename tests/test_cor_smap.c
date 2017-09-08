#include "cor_test.h"
#include "cor_time.h"
#include "../src/cor_smap.c"

BEGIN_TEST(test_cor_smap_new)
{
    cor_smap_t *m = cor_smap_new(10);
    test_ptr_ne(m, NULL);
    /*  test map size is power of 2  */
    test_int_eq(m->key_mask, 15);
    /*  test memory alligned to cache line size  */
    for (int i = 0; i < 16; i++) {
        cor_smap_bucket_t *b = &m->buckets[i];
        test_int_eq((((unsigned long long) b) & 63), 0);
    }
    cor_smap_delete(m);
}
END;

BEGIN_TEST(test_cor_smap_make_bucked_id_and_tag)
{
    cor_smap_t *m = cor_smap_new(10);
    test_ptr_ne(m, NULL);
    int bucked_id;
    uint8_t tag;
    cor_smap_make_bucked_id_and_tag(m, "key", 3, &bucked_id, &tag);
    test_int_eq(bucked_id, 4);
    test_int_eq(tag, 86);
    cor_smap_delete(m);
}
END;

BEGIN_TEST(test_cor_smap_el_new)
{
    cor_smap_el_t *el = cor_smap_el_new("key", 3, "value", 5);
    test_ptr_ne(el, NULL);
    test_int_eq(el->key.size, 3);
    test_strn_eq(el->key.data, "key", 3);
    test_int_eq(el->value.size, 5);
    test_strn_eq(el->value.data, "value", 5);
    free(el);
}
END;

BEGIN_TEST(test_cor_smap_set_el)
{
    cor_smap_t *m = cor_smap_new(10);
    test_ptr_ne(m, NULL);
    cor_smap_el_t *el = cor_smap_el_new("key", 3, "value", 5);
    test_ptr_ne(el, NULL);
    /*  add alement  */
    int rc = cor_smap_set_el(m, el);
    test_int_eq(rc, 0);
    cor_smap_bucket_t *b = &m->buckets[4];
    /*  test tags  */
    test_int_eq(b->tags[0], 86);
    for (int i = 1; i < COR_SMAP_BUCKET_SIZE; i++) {
        test_int_eq(b->tags[i], 0);

    }
    /*  test pointers  */
    test_ptr_eq(b->els[0], el);
    for (int i = 1; i < COR_SMAP_BUCKET_SIZE; i++) {
        test_ptr_eq(b->els[i], NULL);

    }
    /**/
    cor_smap_delete(m);
}
END;

BEGIN_TEST(test_cor_smap_ops)
{
    cor_smap_t *m = cor_smap_new(10);
    test_ptr_ne(m, NULL);
    int rc = cor_smap_extend(m);
    test_int_eq(rc, 0);
    test_int_eq(m->key_mask, 31);
    cor_smap_delete(m);
    /**/
    m = cor_smap_new(4);
    test_ptr_ne(m, NULL);
    for (int i = 0; i < 1000; i++) {
        char key[16];
        char value[16];
        sprintf(key, "key-%d", i);
        sprintf(value, "value-%d", i);
        rc = cor_smap_set(m, key, strlen(key), value, strlen(value));
        test_int_eq(rc, 0);
    }
    for (int i = 0; i < 100; i++) {
        char key[16];
        char value[16];
        sprintf(key, "key-%d", i);
        sprintf(value, "value-%d", i);
        cor_smap_str_t *v = cor_smap_get(m, key, strlen(key));
        test_ptr_ne(v, NULL);
        test_int_eq(v->size, strlen(value));
        test_strn_eq(v->data, value, v->size);
    }
    int count_tags = 0;
    int count_els = 0;
    for (int i = 0; i <= m->key_mask; i++) {
        cor_smap_bucket_t *b = &m->buckets[i];
        for (int n = 0; n < COR_SMAP_BUCKET_SIZE; n++) {
            count_tags += b->tags[n] != 0;
            count_els += b->els[n] != 0;
        }
    }
    test_int_eq(count_tags, 1000);
    test_int_eq(count_els, 1000);
    for (int i = 0; i < 1000; i += 2) {
        char key[16];
        sprintf(key, "key-%d", i);
        cor_smap_del(m, key, strlen(key));
    }
    for (int i = 0; i < 1000; i++) {
        char key[16];
        char value[16];
        sprintf(key, "key-%d", i);
        sprintf(value, "value-%d", i);
        cor_smap_str_t *v = cor_smap_get(m, key, strlen(key));
        if (i % 2 == 0) {
            test_ptr_eq(v, NULL);
        } else {
            test_ptr_ne(v, NULL);
            test_int_eq(v->size, strlen(value));
            test_strn_eq(v->data, value, v->size);
        }
    }
    cor_smap_delete(m);
}
END;

BEGIN_TEST(test_cor_smap_perf)
{
    char key[16 * 10000];
    int len[10000];
    char value[16];
    cor_smap_t *m = cor_smap_new(10000);
    test_ptr_ne(m, NULL);
    sprintf(value, "value-000");
    for (int i = 0; i < 10000; i++) {
        sprintf(&key[i * 16], "key-%d", i);
        len[i] = strlen(&key[i * 16]);
    }
    struct timeval begin;
    struct timeval end;
    gettimeofday(&begin, NULL);
    for (int i = 0; i < 10000; i++) {
        int rc = cor_smap_set(m, &key[i * 16], len[i], value, 9);
        test_int_eq(rc, 0);
    }
    gettimeofday(&end, NULL);
    LOG_PINFO("%f sec, %f sets/sec", cor_time_diff(&begin, &end), 10000.0 / cor_time_diff(&begin, &end));
    gettimeofday(&begin, NULL);
    for (int i = 0; i < 10000; i++) {
        cor_smap_str_t *v = cor_smap_get(m, &key[i * 16], len[i]);
        test_ptr_ne(v, NULL);
    }
    gettimeofday(&end, NULL);
    LOG_PINFO("%f sec, %f gets/sec", cor_time_diff(&begin, &end), 10000.0 / cor_time_diff(&begin, &end));

    cor_smap_delete(m);
}
END;

int
main(int argc, char **argv)
{
    RUN_TEST(test_cor_smap_new);
    RUN_TEST(test_cor_smap_make_bucked_id_and_tag);
    RUN_TEST(test_cor_smap_el_new);
    RUN_TEST(test_cor_smap_set_el);
    RUN_TEST(test_cor_smap_ops);
    RUN_TEST(test_cor_smap_perf);

    exit(0);
}
