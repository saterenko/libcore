#ifndef COR_CONFIG_H
#define COR_CONFIG_H

#include "cor_core.h"
#include "cor_pool.h"
#include <Judy.h>

typedef struct
{
    Pvoid_t data;
    unsigned delete_pool:1;
    char *buf;
    size_t buf_size;
    cor_pool_t *pool;
} cor_config_t;

cor_config_t *cor_config_new(char *file, cor_pool_t *pool);
void cor_config_delete(cor_config_t *config);
const char *cor_config_get_str(cor_config_t *config, const char *key, const char *def);
int cor_config_get_int(cor_config_t *config, const char *key, int def);
int cor_config_get_bool(cor_config_t *config, const char *key, int def);

#endif
