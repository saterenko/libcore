#include <fcntl.h>
#include <sys/stat.h>

#include "cor_test.h"
#include "cor_buf.h"

BEGIN_TEST(test_cor_buf_pool)
{
    cor_buf_pool_t *bp = cor_buf_pool_new(4, 32);
    test_ptr_ne(bp, NULL);
    test_ptr_ne(bp->bufs, NULL);
    test_int_eq(bp->bufs->nelts, 4);
    test_int_eq(bp->bufs->size, (32 + sizeof(cor_buf_t)));
    /*  create new buffers  */
    cor_buf_t *bufs[10];
    for (int i = 0; i < 10; i++) {
        bufs[i] = cor_buf_new(bp);
        test_ptr_ne(bufs[i], NULL);
    }
    test_int_eq(cor_list_nelts(bp->bufs), 10);
    /*  free buffers  */
    for (int i = 0; i < 10; i++) {
        cor_buf_free(bp, bufs[i]);
    }
    int count = 0;
    for (cor_buf_t *b = bp->free_bufs; b; b = b->next) {
        count++;
    }
    test_int_eq(count, 10);
    cor_buf_pool_delete(bp);
}
END;

BEGIN_TEST(test_cor_buf_chain)
{
    cor_buf_pool_t *bp = cor_buf_pool_new(4, 32);
    test_ptr_ne(bp, NULL);
    /*  create chain  */
    cor_buf_chain_t bc;
    memset(&bc, 0, sizeof(cor_buf_chain_t));
    cor_buf_t *bufs[10];
    for (int i = 0; i < 10; i++) {
        bufs[i] = cor_buf_chain_append_buf(bp, &bc);
        test_ptr_ne(bufs[i], NULL);
    }
    test_int_eq(cor_list_nelts(bp->bufs), 10);
    test_int_eq(bc.count, 10);
    test_int_eq(cor_buf_chain_size(&bc), 0);
    test_ptr_eq(bc.head, bufs[0]);
    test_ptr_eq(bc.tail, bufs[9]);
    int count = 0;
    for (cor_buf_t *b = bc.head; b; b = b->next) {
        count++;
    }
    test_int_eq(count, 10);
    /*  freee chain  */
    cor_buf_chain_free(bp, &bc);
    test_int_eq(bc.count, 0);
    test_ptr_eq(bc.head, NULL);
    test_ptr_eq(bc.tail, NULL);
    count = 0;
    for (cor_buf_t *b = bp->free_bufs; b; b = b->next) {
        count++;
    }
    test_int_eq(count, 10);
    /*  append data  */
    for (int i = 0; i < 10; i++) {
        int rc = cor_buf_chain_append_data(bp, &bc, "01234567", 8);
        test_int_eq(rc, cor_ok);
    }
    test_int_eq(bc.count, 3);
    count = 0;
    for (cor_buf_t *b = bc.head; b; b = b->next) {
        count++;
    }
    test_int_eq(count, 3);
    count = 0;
    for (cor_buf_t *b = bp->free_bufs; b; b = b->next) {
        count++;
    }
    test_int_eq(count, 7);
    test_int_eq(cor_buf_chain_size(&bc), 80);
    cor_buf_t *b = bc.head;
    test_ptr_ne(b, NULL);
    test_int_eq((int) (b->last - b->begin), 32);
    test_strn_eq(b->begin, "01234567012345670123456701234567", 32);
    b = b->next;
    test_ptr_ne(b, NULL);
    test_int_eq((int) (b->last - b->begin), 32);
    test_strn_eq(b->begin, "01234567012345670123456701234567", 32);
    b = b->next;
    test_ptr_ne(b, NULL);
    test_int_eq((int) (b->last - b->begin), 16);
    test_strn_eq(b->begin, "0123456701234567", 16);
    /*  revome head  */
    cor_buf_chain_remove_head(bp, &bc);
    test_int_eq(bc.count, 2);
    test_int_eq(cor_buf_chain_size(&bc), 48);
    b = bc.head;
    test_ptr_ne(b, NULL);
    test_int_eq((int) (b->last - b->begin), 32);
    test_strn_eq(b->begin, "01234567012345670123456701234567", 32);
    b = b->next;
    test_ptr_ne(b, NULL);
    test_int_eq((int) (b->last - b->begin), 16);
    test_strn_eq(b->begin, "0123456701234567", 16);
    count = 0;
    for (cor_buf_t *b = bp->free_bufs; b; b = b->next) {
        count++;
    }
    test_int_eq(count, 8);
    /*  revome head  */
    cor_buf_chain_remove_head(bp, &bc);
    test_int_eq(bc.count, 1);
    test_int_eq(cor_buf_chain_size(&bc), 16);
    b = bc.head;
    test_ptr_ne(b, NULL);
    test_int_eq((int) (b->last - b->begin), 16);
    test_strn_eq(b->begin, "0123456701234567", 16);
    count = 0;
    for (cor_buf_t *b = bp->free_bufs; b; b = b->next) {
        count++;
    }
    test_int_eq(count, 9);
    /*  revome head  */
    cor_buf_chain_remove_head(bp, &bc);
    test_int_eq(bc.count, 0);
    test_int_eq(cor_buf_chain_size(&bc), 0);
    b = bc.head;
    test_ptr_eq(b, NULL);
    count = 0;
    for (cor_buf_t *b = bp->free_bufs; b; b = b->next) {
        count++;
    }
    test_int_eq(count, 10);

    cor_buf_pool_delete(bp);
}
END;

int
main(int argc, char **argv)
{
    RUN_TEST(test_cor_buf_pool);
    RUN_TEST(test_cor_buf_chain);

    exit(0);
}
