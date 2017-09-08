#ifndef COR_MMAP_H
#define COR_MMAP_H

#include "cor_core.h"
#include "cor_log.h"

typedef struct
{
    const char *src;
    size_t size;
} cor_mmap_t;

cor_mmap_t *cor_mmap_open(const char *file, cor_log_t *log);
void cor_mmap_close(cor_mmap_t *mmap);

#endif
