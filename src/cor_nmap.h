/*
 * This is a hash table for map unsigned int to pointer. Optimized for fast search. Insertion is slow. 
 *
*/
#ifndef COR_NMAP_H
#define COR_NMAP_H

#include <stdint.h>

#define COR_NMAP_CACHE_LINE_SIZE 64

typedef unsigned int cor_nmap_key_t; 

typedef struct
{
    cor_nmap_key_t key;
    void *value;
    uint8_t distance;
} cor_nmap_bucket_t;

typedef struct
{
    int size;
    int distance;
    cor_nmap_key_t mask;
    cor_nmap_bucket_t *buckets;
} cor_nmap_t;

cor_nmap_t *cor_nmap_new(int size);
void cor_nmap_delete(cor_nmap_t *map);
int cor_nmap_set(cor_nmap_t *m, cor_nmap_key_t key, void *value);
void *cor_nmap_get(cor_nmap_t *m, cor_nmap_key_t key);
void cor_nmap_del(cor_nmap_t *m, cor_nmap_key_t key);

#endif
