#include "cor_json.h"
#include <stdio.h>

static size_t cor_json_stringify_size(const cor_json_node_t *node);
static inline cor_json_node_t *cor_json_node_add_last(cor_json_t *json, cor_json_node_t *parent);
static inline enum cor_json_node_type_e cor_json_node_type(const cor_str_t *s);

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
        begin_s,                // 0
        sp_before_key_s,        // 1
        quote_before_key_s,     // 2
        key_s,                  // 3
        sp_before_colon_s,      // 4
        sp_after_colon_s,       // 5
        string_value_s,         // 6
        array_value_s,          // 7
        other_value_s,          // 8
        sp_after_value_s,       // 9
        backslash_in_value_s,   // 10
        after_object_close_s    // 11
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
//        printf("%d: %d, %c (%x)\n", __LINE__, state, c, c);
        switch (state) {
            case begin_s:
                if (cor_likely(c == '{')) {
                    state = sp_before_key_s;
                    break;
                }
                if (cor_unlikely(c == ' ' || c == '\t' || c == '\n' || c == '\r')) {
                    break;
                }
                cor_json_parse_error("bad symbol");
                return cor_error;
            case sp_before_key_s:
                if (cor_unlikely(c == ' ' || c == '\t' || c == '\n' || c == '\r')) {
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
                stack[stack_index]->name.data = (char *) p;
                state = key_s;
                break;
            case key_s:
                if (cor_likely(c != '"')) {
                    break;
                }
                stack[stack_index]->name.size = p - stack[stack_index]->name.data;
                state = sp_before_colon_s;
                break;
            case sp_before_colon_s:
                if (cor_likely(c == ':')) {
                    state = sp_after_colon_s;
                    break;
                }
                if (cor_likely(c == ' ' || c == '\t' || c == '\n' || c == '\r')) {
                    break;
                }
                cor_json_parse_error("bad symbol");
                return cor_error;
            case sp_after_colon_s:
                if (cor_unlikely(c == ' ' || c == '\t' || c == '\n' || c == '\r')) {
                    break;
                }
                switch (c) {
                    case '"':
                        stack[stack_index]->type = COR_JSON_NODE_STRING;
                        stack[stack_index]->value.data = (char *) p + 1;
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
                        stack[stack_index]->value.data = (char *) p;
                        state = other_value_s;
                        break;
                }
                break;
            case string_value_s:
                if (cor_unlikely(c == '"')) {
                    stack[stack_index]->value.size = p - stack[stack_index]->value.data;
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
                    stack[stack_index]->value.size = p - stack[stack_index]->value.data;
                    stack[stack_index]->type = cor_json_node_type(&stack[stack_index]->value);
                    if (stack[stack_index]->type == COR_JSON_NODE_UNDEFINED) {
                        cor_json_parse_error("undefined value type");
                        return cor_error;
                    }
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
                if (cor_unlikely(c == ' ' || c == '\t' || c == '\n' || c == '\r')) {
                    break;
                }
                switch (c) {
                    case ',':
                        stack[stack_index]->type = COR_JSON_NODE_STRING;
                        stack[stack_index]->value.data = (char *) p + 1;
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
                        stack[stack_index]->value.data = (char *) p;
                        state = other_value_s;
                        break;
                }
                break;
            case backslash_in_value_s:
                if (cor_unlikely(c == '\\')) {
                    break;
                }
                state = string_value_s;
                break;
            case after_object_close_s:
            default:
                break;
        }
    }

#undef cor_json_parse_error

    return cor_ok;
}


static inline char *
cor_json_stringify_append_node(char *p, const cor_json_node_t *node)
{
    if (!node) {
        return p;
    }
    printf("%lx, type: %d, name: %.*s, value: %.*s, first_child: %lx, last_child: %lx, next_sibling: %lx\n",
        (unsigned long) node, node->type, (int) node->name.size, node->name.data,
        (int) node->value.size, node->value.data, (unsigned long) node->first_child,
        (unsigned long) node->last_child, (unsigned long) node->next_sibling);
    if (node->name.size) {
        *p++ = '"';
        memcpy(p, node->name.data, node->name.size);
        p += node->name.size;
        *p++ = '"';
        *p++ = ':';
    }
    switch (node->type) {
        case COR_JSON_NODE_NULL:
            memcpy(p, "null", 4);
            p += 4;
            break;
        case COR_JSON_NODE_STRING:
            *p++ = '"';
            if (node->value.size) {
                memcpy(p, node->value.data, node->value.size);
                p += node->value.size;
            }
            *p++ = '"';
            break;
        case COR_JSON_NODE_INT:
        case COR_JSON_NODE_UINT:
        case COR_JSON_NODE_FLOAT:
        case COR_JSON_NODE_BOOL:
            if (node->value.size) {
                memcpy(p, node->value.data, node->value.size);
                p += node->value.size;
            } else {
                memcpy(p, "null", 4);
                p += 4;
            }
            break;
        case COR_JSON_NODE_OBJECT:
            *p++ = '{';
            for (const cor_json_node_t *n = node->first_child; n != NULL; n = n->next_sibling) {
                p = cor_json_stringify_append_node(p, n);
                if (!p) {
                    return NULL;
                }
                if (n->next_sibling) {
                    *p++ = ',';
                }
            }
            *p++ = '}';
            break;
        case COR_JSON_NODE_ARRAY:
            *p++ = '[';
            for (const cor_json_node_t *n = node->first_child; n != NULL; n = n->next_sibling) {
                p = cor_json_stringify_append_node(p, n);
                if (!p) {
                    return NULL;
                }
                if (n->next_sibling) {
                    *p++ = ',';
                }
            }
            *p++ = ']';
            break;
        default:
            break;
    }
    return p;
}

cor_str_t *
cor_json_stringify(const cor_json_t *json)
{
    size_t size = cor_json_stringify_size(&json->root);
    if (!size) {
        size = 2;
    }
    /**/
    cor_str_t *s = cor_str_new(NULL, size);
    if (!s) {
        return NULL;
    }
    if (json->root.type != COR_JSON_NODE_OBJECT) {
        s->data[0] = '{';
        s->data[1] = '}';
        s->data[2] = '\0';
        s->size = 2;
        return s;
    }
    /**/
    char *p = s->data;
    p = cor_json_stringify_append_node(p, &json->root);
    if (!p) {
        free(s);
        return NULL;
    }
    /**/
    *p++ = '\0';
    s->size = p - s->data;
    return s;
}

static size_t
cor_json_stringify_size(const cor_json_node_t *node)
{
    if (!node) {
        return 0;
    }
    size_t size = 0;
    switch (node->type) {
        case COR_JSON_NODE_NULL:
            size += node->name.size + sizeof("\"\":null,") - 1;
            break;
        case COR_JSON_NODE_STRING:
            size += node->name.size + node->value.size + sizeof("\"\":\"\",") - 1;
            break;
        case COR_JSON_NODE_INT:
        case COR_JSON_NODE_UINT:
            size += node->name.size + sizeof("\"\":-18446744073709551615,") - 1;
            break;
        case COR_JSON_NODE_FLOAT:
            size += node->name.size + 24;
            break;
        case COR_JSON_NODE_BOOL:
            size += node->name.size + sizeof("\"\":false,") - 1;
            break;
        case COR_JSON_NODE_OBJECT:
            size += node->name.size + sizeof("\"\":{},") - 1;
            break;
        case COR_JSON_NODE_ARRAY:
            size += node->name.size + sizeof("\"\":[],") - 1;
            break;
        default:
            break;
    }
    for (const cor_json_node_t *n = node->first_child; n != NULL; n = n->next_sibling) {
        size += cor_json_stringify_size(n);
    }
    return size;
}

static inline cor_json_node_t *
cor_json_node_add_last(cor_json_t *json, cor_json_node_t *parent)
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

static inline enum cor_json_node_type_e
cor_json_node_type(const cor_str_t *s)
{
    enum {
        begin_s,
        exp_digit_s,
        number_s,
        number_after_point_s,
        number_after_point_number_s,
        number_after_exp_s,
        exp_fa_s,
        exp_fal_s,
        exp_fals_s,
        exp_false_s,
        exp_tr_s,
        exp_tru_s,
        exp_true_s,
        exp_nu_s,
        exp_nul_s,
        exp_null_s,
        exp_spaces_s
    } state;
    state = begin_s;
    enum cor_json_node_type_e type = COR_JSON_NODE_UNDEFINED;
    const char *p = s->data;
    const char *end = p + s->size;
    for (; p < end; p++) {
        char c = *p;
        switch (state) {
            case begin_s:
                if (cor_unlikely(c == '-')) {
                    type = COR_JSON_NODE_INT;
                    state = exp_digit_s;
                    break;
                }
                if (cor_likely(c >= '0' && c <= '9')) {
                    type = COR_JSON_NODE_UINT;
                    state = number_s;
                    break;
                }
                char ch = c | 0x20; /*  lowercase  */
                switch (ch) {
                    case 'f':
                        state = exp_fa_s;
                        break;
                    case 'n':
                        state = exp_nu_s;
                        break;
                    case 't':
                        state = exp_tr_s;
                        break;
                }
                break;
            case exp_digit_s:
                if (cor_likely(c >= '0' && c <= '9')) {
                    state = number_s;
                    break;
                }
                return COR_JSON_NODE_UNDEFINED;
            case number_s:
                if (cor_likely(c >= '0' && c <= '9')) {
                    break;
                }
                if (cor_likely(c == '.')) {
                    type = COR_JSON_NODE_FLOAT;
                    state = number_after_point_s;
                    break;
                }
                if (cor_likely(c == 'e')) {
                    state = number_after_exp_s;
                    break;
                }
                if (cor_likely(c == ' ' && c == '\t' && c == '\n' && c == '\r')) {
                    state = exp_spaces_s;
                    break;
                }
                return COR_JSON_NODE_UNDEFINED;
            case number_after_point_s:
                if (cor_likely(c >= '0' && c <= '9')) {
                    state = number_after_point_number_s;
                    break;
                }
                return COR_JSON_NODE_UNDEFINED;
            case number_after_point_number_s:
                if (cor_likely(c >= '0' && c <= '9')) {
                    break;
                }
                if (cor_likely(c == 'e')) {
                    state = number_after_exp_s;
                    break;
                }
                if (cor_likely(c == ' ' && c == '\t' && c == '\n' && c == '\r')) {
                    state = exp_spaces_s;
                    break;
                }
                return COR_JSON_NODE_UNDEFINED;
            case number_after_exp_s:
                if (cor_likely(c >= '0' && c <= '9')) {
                    break;
                }
                if (cor_likely(c == ' ' && c == '\t' && c == '\n' && c == '\r')) {
                    state = exp_spaces_s;
                    break;
                }
                return COR_JSON_NODE_UNDEFINED;
            case exp_fa_s:
                if (cor_likely((c | 0x20) == 'a')) {
                    state = exp_fal_s;
                    break;
                }
                return COR_JSON_NODE_UNDEFINED;
            case exp_fal_s:
                if (cor_likely((c | 0x20) == 'l')) {
                    state = exp_fals_s;
                    break;
                }
                return COR_JSON_NODE_UNDEFINED;
            case exp_fals_s:
                if (cor_likely((c | 0x20) == 's')) {
                    state = exp_false_s;
                    break;
                }
                return COR_JSON_NODE_UNDEFINED;
            case exp_false_s:
                if (cor_likely((c | 0x20) == 'e')) {
                    type = COR_JSON_NODE_BOOL;
                    state = exp_spaces_s;
                    break;
                }
                return COR_JSON_NODE_UNDEFINED;
            case exp_tr_s:
                if (cor_likely((c | 0x20) == 'r')) {
                    state = exp_tru_s;
                    break;
                }
                return COR_JSON_NODE_UNDEFINED;
            case exp_tru_s:
                if (cor_likely((c | 0x20) == 'u')) {
                    state = exp_true_s;
                    break;
                }
                return COR_JSON_NODE_UNDEFINED;
            case exp_true_s:
                if (cor_likely((c | 0x20) == 'e')) {
                    type = COR_JSON_NODE_BOOL;
                    state = exp_spaces_s;
                    break;
                }
                return COR_JSON_NODE_UNDEFINED;
            case exp_nu_s:
                if (cor_likely((c | 0x20) == 'u')) {
                    state = exp_nul_s;
                    break;
                }
                return COR_JSON_NODE_UNDEFINED;
            case exp_nul_s:
                if (cor_likely((c | 0x20) == 'l')) {
                    state = exp_null_s;
                    break;
                }
                return COR_JSON_NODE_UNDEFINED;
            case exp_null_s:
                if (cor_likely((c | 0x20) == 'l')) {
                    type = COR_JSON_NODE_NULL;
                    state = exp_spaces_s;
                    break;
                }
                return COR_JSON_NODE_UNDEFINED;
            case exp_spaces_s:
                if (cor_likely(c == ' ' && c == '\t' && c == '\n' && c == '\r')) {
                    break;
                }
                return COR_JSON_NODE_UNDEFINED;
        }
    }

    return type;
}
