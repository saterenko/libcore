#ifndef COR_KUKU_H
#define COR_KUKU_H

#include <stdint.h>
#include "cor_pool.h"

#define COR_KUKU_DEFAULT_NSLOTS 7

/*  TODO cache cor_kuku_el_t on delete  */

typedef struct
{
    int key_size;
    char *key;
    void *value;
} cor_kuku_el_t;

typedef struct
{
    uint8_t tags[COR_KUKU_DEFAULT_NSLOTS];
    cor_kuku_el_t *els[COR_KUKU_DEFAULT_NSLOTS];
    unsigned int reallocated;
} __attribute__((__packed__)) cor_kuku_slot_t;

typedef struct
{
    int nslots;
    int slot_size;
    cor_kuku_slot_t *slots;
    cor_pool_t *pool;
} cor_kuku_t;

cor_kuku_t *cor_kuku_new(int nels);
void cor_kuku_delete(cor_kuku_t *kuku);
int cor_kuku_set(cor_kuku_t *kuku, const char *key, int key_size, void *value);
void *cor_kuku_get(cor_kuku_t *kuku, const char *key, int key_size);
void cor_kuku_del(cor_kuku_t *kuku, const char *key, int key_size);


#endif
