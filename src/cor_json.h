#ifndef COR_JSON_H
#define COR_JSON_H

#include <unistd.h>

#include "cor_core.h"
#include "cor_pool.h"
#include "cor_str.h"

#define COR_JSON_ERROR_SIZE 128
#define COR_JSON_STACK_SIZE 64
#define COR_JSON_POOL_SIZE 32 * 1024

enum cor_json_node_type_e
{
    COR_JSON_NODE_UNDEFINED = 0,
    COR_JSON_NODE_NULL,
    COR_JSON_NODE_STRING,
    COR_JSON_NODE_INT,
    COR_JSON_NODE_UINT,
    COR_JSON_NODE_FLOAT,
    COR_JSON_NODE_BOOL,
    COR_JSON_NODE_OBJECT,
    COR_JSON_NODE_ARRAY
};

typedef struct cor_json_node_s cor_json_node_t;

struct cor_json_node_s
{
    enum cor_json_node_type_e type;
    cor_str_t name;
    cor_str_t value;
    cor_json_node_t *first_child;
    cor_json_node_t *last_child;
    cor_json_node_t *next_sibling;
};

typedef struct
{
    cor_json_node_t root;
    char error[COR_JSON_ERROR_SIZE];
    cor_pool_t *pool;
} cor_json_t;

cor_json_t *cor_json_new();
void cor_json_delete(cor_json_t *json);
const char *cor_json_error(cor_json_t *json);
int cor_json_parse(cor_json_t *json, const char *p, size_t size);
cor_str_t *cor_json_stringify(const cor_json_t *json);

#endif
