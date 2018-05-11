#include "cor_struct.h"

#define cor_struct_align(_s) ((_s + (sizeof(long) - 1)) & ~(sizeof(long) - 1)

cor_struct_t *
cor_struct_new(int nelts, int max_elts, size_t size)
{
    if (!nelts || !max_elts || !size || (max_elts < nelts)) {
        return NULL;
    }
    /*  make alligned size  */
    size = (size + sizeof(cor_struct_el_t) + sizeof(size_t) - 1) & ~((size_t) (sizeof(size_t) - 1));
    /*  calculate blocks count  */
    int nblocks = nelts == max_elts ? 1 : (max_elts / nelts) + 1;
    /**/
    cor_struct_t *s = (cor_struct_t *) malloc(sizeof(cor_struct_t)
        + sizeof(cor_struct_block_t) * nblocks);
    if (!s) {
        return NULL;
    }
    memset(s, 0, sizeof(cor_struct_t) + sizeof(cor_struct_block_t *) * nblocks);
    s->nelts = nelts;
    s->size = size;
    s->max_free_elts = nelts;
    s->nblocks = nblocks;
    s->blocks = (cor_struct_block_t **) ((char *) s + sizeof(cor_struct_t));
    /*  init first block  */
    s->blocks[0].elts = malloc(size * nelts);
    if (!s->blocks[0].elts) {
        free(s);
        return NULL;
    }
    return s;
}

void
cor_struct_delete(cor_struct_t *s)
{
    if (s) {
        for (int i = 0; i < s->nblocks; i++) {
            if (s->blocks[i]->elts) {
                free(s->blocks[i]->elts);
            }
        }
        free(s);
    }
}

void *
cor_struct_el_new(cor_struct_t *s)
{
    if (s->free_elts) {
        cor_struct_el_t *el = s->free_elts;
        s->free_elts = el->next;
        return (void *) ((char *) el + sizeof(cor_struct_el_t));
    }
    /*  check if we have elements in current block  */
    cor_struct_block_t *b = &s->blocks[s->block_index];
    if (b->nelts == s->nelts) {
        /*  check if we have free block  */
        if (++s->block_index == s->nblocks) {
            s->block_index--;
            return NULL;
        }
        /*  alloc elements for block  */
        b = &s->blocks[s->block_index];
        b->elts = malloc(s->size * s->nelts);
        if (!b->elts) {
            s->block_index--;
            return NULL;
        }
    }
    /**/
    cor_struct_el_t *el = (char *) b->elts + b->nelts * s->size;
    b->nelts++;
    return (void *) ((char *) el + sizeof(cor_struct_el_t));
}

void
cor_struct_el_free(cor_struct_t *s, void *p)
{
    cor_struct_el_t *el = (cor_struct_el_t *) ((char *) p - sizeof(cor_struct_el_t));
    el->next = s->free_elts;
    s->free_elts = el;
}

