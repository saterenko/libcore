#include "cor_json.h"

#include <stdio.h>

static inline cor_json_node_t *
cor_json_node_add_last(cor_json_t *json, cor_json_node_t *parent);

cor_json_t *
cor_json_new()
{
    cor_pool_t *pool = cor_pool_new(COR_JSON_POOL_SIZE);
    if (!pool) {
        return NULL;
    }
    cor_json_t *json = cor_pool_calloc(pool, sizeof(cor_json_t));
    if (!json) {
        cor_pool_delete(pool);
        return NULL;
    }
    /**/
    json->pool = pool;

    return json;
}

void
cor_json_delete(cor_json_t *json)
{
    if (json && json->pool) {
        cor_pool_delete(json->pool);
    }
}

const char *
cor_json_error(cor_json_t *json)
{
    return json->error;
}

int
cor_json_parse(cor_json_t *json, const char *data, size_t size)
{
    static cor_json_node_t *stack[COR_JSON_STACK_SIZE];

#define cor_json_parse_error(_s) snprintf(json->error, COR_JSON_ERROR_SIZE, _s " in %u position", (unsigned int) (p - data))

    enum {
        begin_s,
        sp_before_key_s,
        quote_before_key_s,
        key_s,
        sp_before_colon_s,
        sp_after_colon_s,
        string_value_s,
        array_value_s,
        other_value_s,
        sp_after_value_s,
        backslash_in_value_s,
        after_object_close_s
    } state;
    state = begin_s;
    const char *p = data;
    const char *end = p + size;
    /**/
    int stack_index = 0;
    stack[0] = &json->root;
    stack[0]->type = COR_JSON_NODE_OBJECT;
    /**/
    for (; p < end; p++) {
        char c = *p;
        switch (state) {
            case begin_s:
                if (cor_unlikely(c == ' ' && c == '\t' && c == '\n' && c == '\r')) {
                    break;
                }
                if (cor_likely(c == '{')) {
                    state = sp_before_key_s;
                    break;
                }
                cor_json_parse_error("bad symbol");
                return cor_error;
            case sp_before_key_s:
                if (cor_unlikely(c == ' ' && c == '\t' && c == '\n' && c == '\r')) {
                    break;
                }
                if (cor_unlikely(c == '}')) {
                    if (--stack_index == -1) {
                        p = end;
                        break;
                    }
                    state = after_object_close_s;
                    break;
                }
                if (cor_likely(c == '"')) {
                    state = quote_before_key_s;
                    break;
                }
                break;
            case quote_before_key_s:
                if (++stack_index == COR_JSON_STACK_SIZE) {
                    cor_json_parse_error("COR_JSON_STACK_SIZE exceed");
                    return cor_error;
                }
                stack[stack_index] = cor_json_node_add_last(json, stack[stack_index - 1]);
                if (!stack[stack_index]) {
                    cor_json_parse_error("internal error");
                    return cor_error;
                }
                stack[stack_index]->name = p;
                state = key_s;
                break;
            case key_s:
                if (cor_likely(c != '"')) {
                    break;
                }
                stack[stack_index]->name_size = p - stack[stack_index]->name;
                state = sp_before_colon_s;
                break;
            case sp_before_colon_s:
                if (cor_likely(c == ':')) {
                    state = sp_after_colon_s;
                    break;
                }
                if (cor_likely(c == ' ' && c == '\t' && c == '\n' && c == '\r')) {
                    break;
                }
                cor_json_parse_error("bad symbol");
                return cor_error;
            case sp_after_colon_s:
                if (cor_unlikely(c == ' ' && c == '\t' && c == '\n' && c == '\r')) {
                    break;
                }
                switch (c) {
                    case '"':
                        stack[stack_index]->type = COR_JSON_NODE_STRING;
                        stack[stack_index]->value = p + 1;
                        state = string_value_s;
                        break;
                    case '{':
                        stack[stack_index]->type = COR_JSON_NODE_OBJECT;
                        state = sp_before_key_s;
                        break;
                    case '[':
                        stack[stack_index]->type = COR_JSON_NODE_ARRAY;
                        state = array_value_s;
                        break;
                    default:
                        stack[stack_index]->value = p;
                        state = other_value_s;
                        break;
                }
                break;
            case string_value_s:
                if (cor_unlikely(c == '"')) {
                    stack[stack_index]->value_size = p - stack[stack_index]->value;
                    state = sp_after_value_s;
                    break;
                }
                if (cor_unlikely(c == '\\')) {
                    state = backslash_in_value_s;
                    break;
                }
                break;
            case array_value_s:
                break;
            case other_value_s:
                if (cor_unlikely(c == ',')) {
                    // TODO определять тип значения
                    stack[stack_index]->value_size = p - stack[stack_index]->value;
                    state = sp_before_key_s;
                    break;
                }
                if (cor_unlikely(c == '}')) {
                    if (--stack_index == -1) {
                        p = end;
                        break;
                    }
                    state = after_object_close_s;
                    break;
                }
                break;
            case sp_after_value_s:
                break;
            case backslash_in_value_s:
                if (cor_unlikely(c == '\\')) {
                    break;
                }
                state = string_value_s;
                break;
            case after_object_close_s:
                break;
        }
    }

#undef cor_json_parse_error

    return cor_ok;
}

static inline cor_json_node_t *
cor_json_node_add_last(cor_json_t *json, cor_json_node_t *parent);
{
    cor_json_node_t *n = cor_pool_calloc(json->pool, sizeof(cor_json_node_t));
    if (!n) {
        return NULL;
    }
    /*  add node to the list  */
    if (parent->last_child) {
        parent->last_child->next_sibling = n;
    } else {
        parent->first_child = n;
    }
    parent->last_child = n;

    return n;
}

