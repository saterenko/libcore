#ifndef COR_UUID_H
#define COR_UUID_H

#include <stdint.h>

#include "cor_core.h"

typedef struct
{
    uint64_t uuid[2];
} cor_uuid_t __attribute__((aligned(16)));

typedef struct
{
    cor_uuid_t seed[2];
} cor_uuid_seed_t;

int cor_uuid_init(cor_uuid_seed_t *seed);
void cor_uuid_generate(cor_uuid_seed_t *seed, cor_uuid_t *uuid);
void cor_uuid_parse(const char *p, cor_uuid_t *uuid);
void cor_uuid_unparse(cor_uuid_t *uuid, char *p);

#endif
