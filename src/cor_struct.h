#ifndef COR_STRUCT_H
#define COR_STRUCT_H

#include <stdlib.h>
#include <string.h>

#include "cor_core.h"

typedef struct cor_struct_block_s cor_struct_block_t;

struct cor_struct_block_s
{
    int nelts;
    void *elts;
    cor_struct_block_t *next;
};

typedef struct cor_struct_el_s cor_struct_el_t;

struct cor_struct_el_s
{
    cor_struct_el_t *next;
};

typedef void (cor_struct_cb_t) (void *el, void *arg);

typedef struct
{
    int nelts;
    size_t size;
    int max_free_elts;
    cor_struct_cb_t *on_new;
    cor_struct_cb_t *on_free;
    cor_struct_cb_t *on_delete;

    int nblocks;
    int block_index;
    cor_struct_block_t *blocks;

    cor_struct_el_t *free_elts;

} cor_struct_t;

cor_struct_t *cor_struct_new(int nelts, int max_elts, size_t size);
void cor_struct_delete(cor_struct_t *s);
void *cor_struct_el_new(cor_struct_t *s);
void cor_struct_el_free(cor_struct_t *s, void *p);

#endif
