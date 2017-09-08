/*
 * This is a hash table optimized for fast search. Insertion is slow. 
 *
*/

#ifndef COR_SMAP_H
#define COR_SMAP_H

#include <stdint.h>

#define COR_SMAP_BUCKET_SIZE 7
#define COR_SMAP_CACHE_LINE_SIZE 64

typedef struct
{
    int size;
    char *data;
} cor_smap_str_t;

typedef struct
{
    cor_smap_str_t key;
    cor_smap_str_t value;
} cor_smap_el_t;

typedef struct
{
    uint8_t tags[COR_SMAP_BUCKET_SIZE];
    cor_smap_el_t *els[COR_SMAP_BUCKET_SIZE];
    uint8_t dummy;
} __attribute__((__packed__)) cor_smap_bucket_t;

typedef struct
{
    unsigned int key_mask;
    cor_smap_bucket_t *buckets;
    cor_smap_bucket_t *_buckets;
} cor_smap_t;


/**
@brief Create smap
@param size initial count of buckets
@return structure cor_smap_t * 
*/
cor_smap_t *cor_smap_new(int size);
void cor_smap_delete(cor_smap_t *map);
int cor_smap_set(cor_smap_t *map, const char *key, int key_size, const char *value, int value_size);
cor_smap_str_t *cor_smap_get(cor_smap_t *map, const char *key, int key_size);
void cor_smap_del(cor_smap_t *map, const char *key, int key_size);

#endif
