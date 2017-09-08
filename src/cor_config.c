#include "cor_config.h"
#include "cor_mmap.h"

#define COR_CONFIG_POOL_SIZE 1024

static int cor_config_load(cor_config_t *config, char *file);
static int cor_config_parse(cor_config_t *config, cor_mmap_t *mmap);
static int cor_config_parse_add_key_value(cor_config_t *config, const char *key_begin, const char *key_end,
    const char *value_begin, const char *value_end);

cor_config_t *
cor_config_new(char *file, cor_pool_t *pool)
{
    int delete_pool = 0;
    if (!pool) {
        pool = cor_pool_new(COR_CONFIG_POOL_SIZE);
        if (!pool) {
            fprintf(stderr, "can't cor_pool_new\n");
            return NULL;
        }
        delete_pool = 1;
    }
    cor_config_t *config = (cor_config_t *) cor_pool_calloc(pool, sizeof(cor_config_t));
    if (!config) {
        if (delete_pool) {
            fprintf(stderr, "can't cor_pool_calloc\n");
            cor_pool_delete(pool);
        }
        return NULL;
    }
    config->pool = pool;
    config->delete_pool = delete_pool;
    if (cor_config_load(config, file) != cor_ok) {
        cor_config_delete(config);
        fprintf(stderr, "can't cor_config_load\n");
        return NULL;
    }
    
    return config;
}

void
cor_config_delete(cor_config_t *config)
{
    if (config) {
        if (config->data) {
            int rc;
            JSLFA(rc, config->data);
        }
        if (config->pool && config->delete_pool) {
            cor_pool_delete(config->pool);
        }
    }
}

const char *
cor_config_get_str(cor_config_t *config, const char *key, const char *def)
{
    PWord_t pw;
    JSLG(pw, config->data, (uint8_t *) key);
    return pw ? (const char *) *pw : def;
}

int
cor_config_get_int(cor_config_t *config, const char *key, int def)
{
    PWord_t pw;
    JSLG(pw, config->data, (uint8_t *) key);
    return pw ? atoi((const char *) *pw) : def;
}

int
cor_config_get_bool(cor_config_t *config, const char *key, int def)
{
    PWord_t pw;
    JSLG(pw, config->data, (uint8_t *) key);
    return pw ? strncmp((const char *) *pw, "yes", 3) == 0 : def;
}

static int
cor_config_load(cor_config_t *config, char *file)
{
    cor_mmap_t *mmap = cor_mmap_open(file, NULL);
    if (!mmap) {
        fprintf(stderr, "can't open file: %s\n", file);
        return cor_error;
    }
    config->buf_size = 256;
    config->buf = malloc(config->buf_size);
    if (!config->buf) {
        fprintf(stderr, "can't malloc\n");
        return cor_error;
    }
    if (cor_config_parse(config, mmap) != cor_ok) {
        fprintf(stderr, "can't cor_config_parse\n");
        cor_mmap_close(mmap);
        free(config->buf);
        config->buf = NULL;
        config->buf_size = 0;
        return cor_error;
    }
    free(config->buf);
    config->buf = NULL;
    config->buf_size = 0;
    cor_mmap_close(mmap);

    return cor_ok;
}

static int
cor_config_parse(cor_config_t *config, cor_mmap_t *mmap)
{
    enum {
        s_space,
        s_key,
        s_key_sp,
        s_value_quoted,
        s_value_quoted_slash,
        s_value,
        s_comment
    } state;
    state = s_space;
    int line = 1;
    int position = 1;
    const char *p = mmap->src;
    const char *end = mmap->src + mmap->size;
    const char *key_begin = NULL;
    const char *key_end = NULL;
    const char *value_begin = NULL;
    for ( ; p < end; p++, position++) {
        char c = p[0];
        switch (state) {
            case s_space:
            {
                if (c == ' ' || c == '\r' || c == '\n') {
                    break;
                } else if (c == '\n') {
                    line++;
                    position = 1;
                    break;
                } else if (c == '#') {
                    state = s_comment;
                    break;
                }
                char ch = c | 0x20;
                if ((ch >= 'a' && ch <= 'z') || (c >= '0' && c <= '9') || c == '.' || c =='-' || c == '_') {
                    key_begin = p;
                    state = s_key;
                } else {
                    fprintf(stderr, "bad char %c in %d:%d\n", c, line, position);
                    return cor_error;
                }
                break;
            }
            case s_key:
            {
                char ch = c | 0x20;
                if ((ch >= 'a' && ch <= 'z') || (c >= '0' && c <= '9') || c == '.' || c =='-' || c == '_') {
                    break;
                }
                if (c == ' ' || c == '\t') {
                    key_end = p;
                    state = s_key_sp;
                } else {
                    fprintf(stderr, "bad char %c in %d:%d\n", c, line, position);
                    return cor_error;
                }
                break;
                
            }
            case s_key_sp:
                if (c == ' ' || c == '\t') {
                    break;
                }
                if (c == '"') {
                    state = s_value_quoted;
                    value_begin = p + 1;
                } else {
                    state = s_value;
                    value_begin = p;
                }
                break;
            case s_value_quoted:
                if (c == '"') {
                    int rc = cor_config_parse_add_key_value(config, key_begin, key_end, value_begin, p);
                    if (rc != cor_ok) {
                        fprintf(stderr, "bad key/value in %d:%d\n", line, position);
                        return cor_error;
                    }
                    state = s_space;
                } else if (c == '\\') {
                    state = s_value_quoted_slash;
                } else if (c == '\n') {
                    position = 1;
                    line++;
                }
                break;
            case s_value_quoted_slash:
                if (c == '\\') {
                    break;
                }
                if (c == '\n') {
                    position = 1;
                    line++;
                }
                state = s_value_quoted;
                break;
            case s_value:
                if (c == '\n') {
                    int rc = cor_config_parse_add_key_value(config, key_begin, key_end, value_begin, p);
                    if (rc != cor_ok) {
                        fprintf(stderr, "bad key/value in %d:%d\n", line, position);
                        return cor_error;
                    }
                    state = s_space;
                    position = 1;
                    line++;
                }
                break;
            case s_comment:
                if (c == '\n') {
                    position = 1;
                    line++;
                    state = s_space;
                }
                break;
        }
    }

    return cor_ok;
}

static int
cor_config_parse_add_key_value(cor_config_t *config, const char *key_begin, const char *key_end,
    const char *value_begin, const char *value_end)
{
    int key_size = key_end - key_begin;
    if (key_size == 0) {
        fprintf(stderr, "key size is zero\n");
        return cor_error;
    }
    int value_size = value_end - value_begin;
    if (value_size == 0) {
        fprintf(stderr, "value size is zero\n");
        return cor_error;
    }
    /*  make key  */
    if ((key_size + 1) > config->buf_size) {
        char *nb = realloc(config->buf, key_size + 1);
        if (!nb) {
            fprintf(stderr, "can't realloc\n");
            return cor_error;
        }
        config->buf = nb;
        config->buf_size = key_size + 1;
    }
    memcpy(config->buf, key_begin, key_size);
    config->buf[key_size] = '\0';
    /*  make value  */
    char *value = cor_pool_alloc(config->pool, value_size + 1);
    if (!value) {
        fprintf(stderr, "can't cor_pool_alloc\n");
        return cor_error;
    }
    memcpy(value, value_begin, value_size);
    value[value_size] = '\0';
    /**/
    PWord_t pw;
    JSLI(pw, config->data, (uint8_t *) config->buf);
    if (pw == PJERR) {
        fprintf(stderr, "can't JSLI\n");
        return cor_error;
    }
    *pw = (Word_t) value;

    return cor_ok;
}

