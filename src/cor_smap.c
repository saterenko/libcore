#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xxhash.h"

#include "cor_smap.h"

cor_smap_t *
cor_smap_new(int need_size)
{
    cor_smap_t *map = (cor_smap_t *) malloc(sizeof(cor_smap_t));
    if (!map) {
        return NULL;
    }
    memset(map, 0, sizeof(cor_smap_t));
    /*  determine backets size  */
    int size = 4; /*  minimal buckets size  */
    while (size < need_size) {
        size <<= 1;
    }
    map->key_mask = size - 1;
    /**/
    map->_buckets = (cor_smap_bucket_t *) malloc(sizeof(cor_smap_bucket_t) * (size + 1));
    if (!map->_buckets) {
        cor_smap_delete(map);
        return NULL;
    }
    memset(map->_buckets, 0, sizeof(cor_smap_bucket_t) * (size + 1));
    /*  allign to cache line size  */
    map->buckets = (cor_smap_bucket_t *) (((uintptr_t) map->_buckets + ((uintptr_t) COR_SMAP_CACHE_LINE_SIZE - 1)) & ~((uintptr_t) COR_SMAP_CACHE_LINE_SIZE - 1));
    return map;
}

void
cor_smap_delete(cor_smap_t *map)
{
    if (map) {
        if (map->buckets) {
            for (int i = 0; i <= map->key_mask; i++) {
                cor_smap_bucket_t *b = &map->buckets[i];
                for (int n = 0; n < COR_SMAP_BUCKET_SIZE; n++) {
                    if (!b->els[n]) {
                        break;
                    }
                    free(b->els[n]);
                }
            }
            free(map->_buckets);
        }
        free(map);
    }
}

static inline void
cor_smap_make_bucked_id_and_tag(cor_smap_t *map, const char *key, int key_size, int *bucket_id, uint8_t *tag)
{
    XXH64_hash_t hash = XXH64(key, key_size, 0);
    *bucket_id = hash & map->key_mask;
    *tag = (hash >> 32) & 0xff;
    *tag += *tag == 0;
}

static inline cor_smap_el_t *
cor_smap_el_new(const char *key, int key_size, const char *value, int value_size)
{
    cor_smap_el_t *el = (cor_smap_el_t *) malloc(sizeof(cor_smap_el_t) + key_size + value_size);
    if (!el) {
        return NULL;
    }
    el->key.size = key_size;
    el->key.data = (char *) el + sizeof(cor_smap_el_t);
    memcpy(el->key.data, key, key_size);
    el->value.size = value_size;
    el->value.data = el->key.data + key_size;
    memcpy(el->value.data, value, value_size);
    return el;
}

static int
cor_smap_set_el(cor_smap_t *map, cor_smap_el_t *el)
{
    /*  search for slot  */
    int bucket_id;
    uint8_t tag;
    cor_smap_make_bucked_id_and_tag(map, el->key.data, el->key.size, &bucket_id, &tag);
    cor_smap_bucket_t *bucket = &map->buckets[bucket_id];
    for (int i = 0; i < COR_SMAP_BUCKET_SIZE; i++) {
        if (bucket->tags[i]) {
            if (bucket->tags[i] == tag) {
                cor_smap_el_t *e = bucket->els[i];
                if (e->key.size == el->key.size && strncmp(e->key.data, el->key.data, e->key.size) == 0) {
                    /*  replace element  */
                    free(el);
                    bucket->els[i] = el;
                    return 0;
                }
            }
        } else {
            /*  add new element  */
            bucket->tags[i] = tag;
            bucket->els[i] = el;
            return 0;
        }
    }
    return -2;
}

static int
cor_smap_extend(cor_smap_t *map)
{
    cor_smap_t *nmap = cor_smap_new((map->key_mask + 1) * 2);
    if (!nmap) {
        return -1;
    }
    for (int i = 0; i <= map->key_mask; i++) {
        cor_smap_bucket_t *b = &map->buckets[i];
        for (int n = 0; n < COR_SMAP_BUCKET_SIZE; n++) {
            if (!b->els[n]) {
                break;
            }
            cor_smap_el_t *el = b->els[n];
            if (cor_smap_set_el(nmap, el) != 0) {
                return -1;
            }
        }
    }
    free(map->_buckets);
    memcpy(map, nmap, sizeof(cor_smap_t));
    free(nmap);
    return 0;
}

int
cor_smap_set(cor_smap_t *map, const char *key, int key_size, const char *value, int value_size)
{
    cor_smap_el_t *el = cor_smap_el_new(key, key_size, value, value_size);
    if (!el) {
        return -1;
    }
    while (1) {
        if (cor_smap_set_el(map, el) == 0) {
            return 0;
        }
        /*  no empty slot found, extend hash table  */
        if (cor_smap_extend(map) != 0) {
            free(el);
            return -1;
        }
    }
}

cor_smap_str_t *
cor_smap_get(cor_smap_t *map, const char *key, int key_size)
{
    int bucket_id;
    uint8_t tag;
    cor_smap_make_bucked_id_and_tag(map, key, key_size, &bucket_id, &tag);
    cor_smap_bucket_t *bucket = &map->buckets[bucket_id];
    for (int i = 0; i < COR_SMAP_BUCKET_SIZE; i++) {
        if (bucket->tags[i]) {
            if (bucket->tags[i] == tag) {
                cor_smap_el_t *el = bucket->els[i];
                if (el->key.size == key_size && strncmp(key, el->key.data, key_size) == 0) {
                    return &el->value;
                }
            }
        } else {
            break;
        }
    }
    return NULL;
}

void
cor_smap_del(cor_smap_t *map, const char *key, int key_size)
{
    int bucket_id;
    uint8_t tag;
    cor_smap_make_bucked_id_and_tag(map, key, key_size, &bucket_id, &tag);
    cor_smap_bucket_t *bucket = &map->buckets[bucket_id];
    for (int i = 0; i < COR_SMAP_BUCKET_SIZE; i++) {
        if (bucket->tags[i]) {
            if (bucket->tags[i] == tag) {
                cor_smap_el_t *el = bucket->els[i];
                if (el->key.size == key_size && strncmp(key, el->key.data, key_size) == 0) {
                    /*  remove element  */
                    free(el);
                    bucket->tags[i] = 0;
                    bucket->els[i] = NULL;
                    /*  move elements to top  */
                    for (int n = i + 1; n < COR_SMAP_BUCKET_SIZE && bucket->tags[n]; n++) {
                        bucket->tags[n - 1] = bucket->tags[n];
                        bucket->els[n - 1] = bucket->els[n];
                        bucket->tags[n] = 0;
                        bucket->els[n] = NULL;
                    }
                    return;
                }
            }
        } else {
            break;
        }
    }
    return;
}
