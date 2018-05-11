#include "cor_nmap.h"

#include <x86intrin.h>

#include "cor_core.h"

static int cor_nmap_extend(cor_nmap_t *m, int size);
static inline int cor_nmap_size(int size);
static inline int cor_nmap_distance(int size);

cor_nmap_t *
cor_nmap_new(int size)
{
    cor_nmap_t *m = (cor_nmap_t *) malloc(sizeof(cor_nmap_t));
    if (!m) {
        return NULL;
    }
    memset(m, 0, sizeof(cor_nmap_t));
    m->size = cor_nmap_size(size);
    if (cor_nmap_extend(m, m->size) != cor_ok) {
        cor_nmap_delete(m);
        return NULL;
    }
    return m;
}

void
cor_nmap_delete(cor_nmap_t *map)
{

}

int
cor_nmap_set(cor_nmap_t *m, cor_nmap_key_t key, void *value)
{
    cor_nmap_bucket_t *b = b->buckets + (key & m->mask);
    cor_nmap_bucket_t *end = b + m->distance;
    for (; b < end; b++) {
        if (b->value) {
            /*  bucket is occupied  */
            
        } else {
            /*  bucket is empty  */
            b->key = key;
            b->value = value;
            b->distance = 0;
            return cor_ok;
        }
    }



}

void *
cor_nmap_get(cor_nmap_t *m, cor_nmap_key_t key)
{
    cor_nmap_bucket_t *b = b->buckets + (key & m->mask);
    cor_nmap_bucket_t *end = b + m->distance;
    for (; b < end; b++) {
        if (b->key == key) {
            return b->value;
        }
    }
    return NULL;
}

void
cor_nmap_del(cor_nmap_t *m, cor_nmap_key_t key)
{

}

static int
cor_nmap_extend(cor_nmap_t *m, int size)
{
    cor_nmap_t n;
    n.size = size; 
    n.distance = cor_nmap_distance(size);
    n.mask = size - 1;
    n.buckets = (cor_nmap_bucket_t *) malloc(sizeof(cor_nmap_bucket_t) * size);
    if (!n.buckets) {
        return cor_error;
    }
    memset(n.buckets, 0, sizeof(cor_nmap_bucket_t) * size);
    if (m->buckets) {
        cor_nmap_bucket_t *b = m->buckets;
        cor_nmap_bucket_t *end = m->buckets + m->size + m->distance;
        for (; b < end; b++) {
            if (b->value) {
                cor_nmap_set(&n, b->key, b->value);
            }
        }
        free(m->buckets);
    }
    memcpy(m, &n, sizeof(cor_nmap_t));
    return cor_ok;
}

static inline int
cor_nmap_size(int size)
{
    if (size <= 2) {
        return 4;
    }
    size--;
    size |= size >> 1;
    size |= size >> 2;
    size |= size >> 4;
    size |= size >> 8;
    size |= size >> 16;
    size++;
    return size;
}

static inline int
cor_nmap_distance(int size)
{
    if (size <= 2) {
        return 4;
    }
    return (8 * sizeof(cor_nmap_key_t) - __builtin_clzll(size) - 1);
}
