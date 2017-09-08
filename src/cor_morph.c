#include "cor_morph.h"

#define COR_MORPH_POOL_SIZE 1024
#define COR_MORPH_SA_INDEX_SIZE 4096
#define COR_MORPH_WORD_BUFFER_SIZE 256
#define COR_MORPH_CONTENT_BUFFER_SIZE 4096

static inline int cor_morph_content_to_base_form_cat(cor_morph_t *morph, char **p, char **end,
    const char *word, size_t size);
static int cor_morph_sa_index_expand(cor_morph_t *morph);
static inline void cor_morph_sa_index_sort(cor_morph_doc_sa_range_t *sa_ranges, const char **sa_src,
    const char **sa_dst, int sa_size, int shift, int *radix, int *radix2, const char *end);

cor_morph_t *
cor_morph_new(cor_pool_t *pool, cor_log_t *log)
{
    int delete_pool = 0;
    if (!pool) {
        pool = cor_pool_new(COR_MORPH_POOL_SIZE);
        if (!pool) {
            cor_log_error(log, "can't cor_pool_new");
            return NULL;
        }
        delete_pool = 1;
    }
    cor_morph_t *morph = (cor_morph_t *) cor_pool_calloc(pool, sizeof(cor_morph_t));
    if (!morph) {
        cor_log_error(log, "can't cor_pool_calloc");
        if (delete_pool) {
            cor_pool_delete(pool);
        }
        return NULL;
    }
    morph->pool = pool;
    morph->delete_pool = delete_pool;
    morph->log = log;
    morph->content.data = (char *) malloc(COR_MORPH_CONTENT_BUFFER_SIZE);
    if (!morph->content.data) {
        cor_log_error(log, "can't malloc");
        cor_morph_delete(morph);
        return NULL;
    }
    morph->content_buffer_size = COR_MORPH_CONTENT_BUFFER_SIZE;
    if (cor_morph_sa_index_expand(morph) != cor_ok) {
        cor_log_error(log, "can't cor_morph_sa_index_expand");
        cor_morph_delete(morph);
        return NULL;
    }

    return morph;
}

void
cor_morph_delete(cor_morph_t *morph)
{
    if (morph) {
        if (morph->sa_index) {
            free(morph->sa_index);
        }
        if (morph->sa_index2) {
            free(morph->sa_index2);
        }
        if (morph->sa_ranges) {
            free(morph->sa_ranges);
        }
        if (morph->content.data) {
            free(morph->content.data);
        }
        if (morph->pool && morph->delete_pool) {
            cor_pool_delete(morph->pool);
        }
    }
}

void
cor_morph_set_dict(cor_morph_t *morph, cor_dict_t *dict)
{
    morph->dict = dict;
}

static inline int
cor_morph_content_to_base_form_cat(cor_morph_t *morph, char **p, char **end, const char *word, size_t size)
{
    /*  проверяем что хватит места  */
    if (*end - *p < size) {
        size_t new_size = morph->content_buffer_size * 2 - (morph->content_buffer_size / 2);
        char *new_buffer = (char *) realloc(morph->content.data, new_size);
        if (!new_buffer) {
            cor_log_error(morph->log, "can't realloc");
            return cor_error;
        }
        *p = new_buffer + (*p - morph->content.data);
        morph->content.data = new_buffer;
        *end = morph->content.data + new_size;
        morph->content_buffer_size = new_size;
    }
    /*  добавляем строку вконец  */
    memcpy(*p, word, size);
    *p += size;

    return cor_ok;
}

cor_str_t *
cor_morph_content_to_base_form(cor_morph_t *morph, const char *content, size_t size)
{
    int word_index = 0;
    char word[COR_MORPH_WORD_BUFFER_SIZE];
    const char *p = content;
    const char *end = p + size;
    char *k = morph->content.data;
    char *k_end = k + morph->content_buffer_size;
    for (; p < end; ++p) {
        if (p[0] == ' ') {
            if (word_index) {
                word[word_index] = '\0';
                cor_dict_word_t *word_info = cor_dict_get_word(morph->dict, word);
                if (word_info) {
                    if (cor_morph_content_to_base_form_cat(morph, &k, &k_end,
                        word_info->base->data, word_info->base->size) != cor_ok)
                    {
                        return NULL;
                    }
                    *(k++) = ' ';
                } else {
                    if (cor_morph_content_to_base_form_cat(morph, &k, &k_end, word, word_index) != cor_ok) {
                        return NULL;
                    }
                    *(k++) = ' ';
                }
                word_index = 0;
            }
        } else {
            word[word_index++] = p[0];
        }
    }
    if (word_index) {
        word[word_index] = '\0';
        cor_dict_word_t *word_info = cor_dict_get_word(morph->dict, word);
        if (word_info) {
            if (cor_morph_content_to_base_form_cat(morph, &k, &k_end,
                word_info->base->data, word_info->base->size) != cor_ok)
            {
                return NULL;
            }
        } else {
            if (cor_morph_content_to_base_form_cat(morph, &k, &k_end, word, word_index) != cor_ok) {
                return NULL;
            }
        }
    }
    *k = '\0';
    morph->content.size = k - morph->content.data;

    return &morph->content;
}

static int
cor_morph_sa_index_expand(cor_morph_t *morph)
{
    if (morph->sa_index_size) {
        int new_size = morph->sa_index_size * 2 - (morph->sa_index_size / 2);
        const char **new_index = (const char **) realloc(morph->sa_index, sizeof(char *) * new_size);
        if (!new_index) {
            cor_log_error(morph->log, "can't realloc");
            return cor_error;
        }
        const char **new_index2 = malloc(sizeof(char *) * new_size);
        if (!new_index2) {
            cor_log_error(morph->log, "can't realloc");
            free(new_index);
            return cor_error;
        }
        cor_morph_doc_sa_range_t *new_ranges = malloc(sizeof(cor_morph_doc_sa_range_t) * COR_MORPH_SA_INDEX_SIZE);
        if (!new_ranges) {
            cor_log_error(morph->log, "can't malloc");
            free(new_index);
            free(new_index2);
            return cor_error;
        }
        free(morph->sa_index2);
        free(morph->sa_ranges);
        morph->sa_index = new_index;
        morph->sa_index2 = new_index2;
        morph->sa_ranges = new_ranges;
        morph->sa_index_size = new_size;
    } else {
        morph->sa_index = malloc(sizeof(char *) * COR_MORPH_SA_INDEX_SIZE);
        if (!morph->sa_index) {
            cor_log_error(morph->log, "can't malloc");
            return cor_error;
        }
        morph->sa_index2 = malloc(sizeof(char *) * COR_MORPH_SA_INDEX_SIZE);
        if (!morph->sa_index2) {
            cor_log_error(morph->log, "can't malloc");
            return cor_error;
        }
        morph->sa_ranges = malloc(sizeof(cor_morph_doc_sa_range_t) * COR_MORPH_SA_INDEX_SIZE);
        if (!morph->sa_ranges) {
            cor_log_error(morph->log, "can't malloc");
            return cor_error;
        }
        morph->sa_index_size = COR_MORPH_SA_INDEX_SIZE;
    }

    return cor_ok;
}

static inline void
cor_morph_sa_index_sort(cor_morph_doc_sa_range_t *sa_ranges, const char **sa_src,
    const char **sa_dst, int sa_size, int shift, int *radix, int *radix2, const char *end)
{
    memset(radix, 0, 256 * sizeof(int));
    /*  radix sort  */
    for (int i = 0; i < sa_size; ++i) {
        uint8_t c = (uint8_t) sa_src[i][shift];
        radix[c]++;
    }
    int cs = 0;
    for (int i = 0; i < 256; ++i) {
        int m = radix[i];
        radix[i] = cs;
        cs += m;
    }
    memcpy(radix2, radix, sizeof(int) * 256);
    for (int i = 0; i < sa_size; ++i) {
        uint8_t c = (uint8_t) sa_src[i][shift];
        sa_dst[radix[c]] = sa_src[i];
        radix[c]++;
    }
    /*  make ranges  */
    int sa_range_index = 0;
    for (int i = 0; i < sa_size; ++i) {
        uint8_t c = (uint8_t) sa_src[i][shift];
        int sa_begin = radix2[c];
        int sa_end = radix[c];
        if (sa_end - sa_begin > 1) {
            sa_ranges[sa_range_index].begin = sa_begin;
            sa_ranges[sa_range_index].end = sa_end;
            sa_range_index++;
            i += sa_end - sa_begin;
        }
    }
    memcpy(sa_src, sa_dst, sizeof(char *) * sa_size);
    for (int i = 0; i < sa_range_index; ++i) {
        cor_morph_sa_index_sort(sa_ranges, &sa_dst[sa_ranges[i].begin], &sa_src[sa_ranges[i].begin],
            sa_ranges[i].end - sa_ranges[i].begin, shift + 1, radix, radix2, end);
    }
}

cor_morph_doc_t *
cor_morph_doc_new(cor_morph_t *morph, const char *content, size_t size)
{
    /*  запоминаем позиции каждого слова  */
    int index = 0;
    const char *p = content;
    const char *end = p + size;
    morph->sa_index[index++] = p;
    for (; p < end; ++p) {
        if (p[0] == ' ') {
            morph->sa_index[index++] = p + 1;
            if (index == morph->sa_index_size) {
                if (cor_morph_sa_index_expand(morph) != cor_ok) {
                    return NULL;
                }
            }
        }
    }
    /*  сортируем индекс по порядку  */
    cor_morph_sa_index_sort(morph->sa_ranges, morph->sa_index, morph->sa_index2, index, 0,
        morph->radix, morph->radix2, end);
    /*  создаём документ  */
    cor_morph_doc_t *doc = (cor_morph_doc_t *) malloc(sizeof(cor_morph_doc_t)
        + sizeof(cor_morph_doc_sa_el_t) * index + size + 1);
    if (!doc) {
        cor_log_error(morph->log, "can't malloc");
        return NULL;
    }
    doc->content.data = (char *) doc + sizeof(cor_morph_doc_t);
    memcpy(doc->content.data, content, size);
    doc->content.size = size;
    doc->content.data[size] = '\0';
    doc->sa_index = (cor_morph_doc_sa_el_t *) (doc->content.data + size + 1);
    for (int i = 0; i < index; ++i) {
        doc->sa_index[i].position = morph->sa_index[i] - content;
    }
    doc->sa_index_size = index;

    return doc;
}

void
cor_morph_doc_delete(cor_morph_doc_t *doc)
{
    if (doc) {
        free(doc);
    }
}

cor_str_t *
cor_morph_doc_pack(cor_morph_t *morph, cor_morph_doc_t *doc)
{
    size_t size = doc->content.size + (doc->sa_index_size + 1) * sizeof("4294967296");
    if (size > morph->content_buffer_size) {
        char *new_buffer = (char *) malloc(size);
        if (!new_buffer) {
            cor_log_error(morph->log, "can't malloc");
            return NULL;
        }
        free(morph->content.data);
        morph->content.data = new_buffer;
        morph->content_buffer_size = size;
    }
    char *p = morph->content.data;
    /*  write format version  */
    *p++ = '1';
    *p++ = '\n';
    /*  write content size  */
    p += cor_str_itoa(doc->content.size, p);
    *p++ = '\n';
    /*  write content  */
    memcpy(p, doc->content.data, doc->content.size);
    p += doc->content.size;
    *p++ = '\n';
    /*  write sa  */
    for (int i = 0; i < doc->sa_index_size; ++i) {
        p += cor_str_itoa(doc->sa_index[i].position, p);
        *p++ = ' ';
    }
    *p = '\n';
    morph->content.size = p - morph->content.data;

    return &morph->content;
}

cor_morph_doc_t *
cor_morph_doc_unpack(cor_morph_t *morph, const char *content, size_t size)
{
    enum {
        s_version,
        s_content_size,
        s_sa_position
    } state;
    int version = 0;
    int sa_index = 0;
    int position = 0;
    int content_size = 0;
    const char *content_begin = NULL;
    state = s_version;
    const char *p = content;
    const char *end = p + size;
    for (; p < end; ++p) {
        char c = *p;
        switch (state) {
            case s_version:
                if (c >= '0' && c <= '9') {
                    version = version * 10 + (c - '0');
                    break;
                }
                if (c == '\n') {
                    state = s_content_size;
                } else {
                    cor_log_error(morph->log, "bad char");
                    return NULL;
                }
                break;
            case s_content_size:
                if (c >= '0' && c <= '9') {
                    content_size = content_size * 10 + (c - '0');
                    break;
                }
                if (c == '\n') {
                    content_begin = p + 1;
                    p += content_size + 1 /*  for \n  */;
                    state = s_sa_position;
                } else {
                    cor_log_error(morph->log, "bad char");
                    return NULL;
                }
                break;
            case s_sa_position:
                if (c >= '0' && c <= '9') {
                    position = position * 10 + (c - '0');
                    break;
                }
                if (c == ' ' || c == '\n') {
                    morph->sa_ranges[sa_index++].begin = position;
                    if (sa_index == morph->sa_index_size) {
                        if (cor_morph_sa_index_expand(morph) != cor_ok) {
                            cor_log_error(morph->log, "can't cor_morph_sa_index_expand");
                            return NULL;
                        }
                    }
                    position = 0;
                } else {
                    cor_log_error(morph->log, "bad char");
                    return NULL;
                }
                break;
        }
    }
    /*  create document  */
    cor_morph_doc_t *doc = (cor_morph_doc_t *) malloc(sizeof(cor_morph_doc_t)
        + sizeof(cor_morph_doc_sa_el_t) * sa_index + size + 1);
    if (!doc) {
        cor_log_error(morph->log, "can't malloc");
        return NULL;
    }
    doc->content.data = (char *) doc + sizeof(cor_morph_doc_t);
    memcpy(doc->content.data, content_begin, content_size);
    doc->content.size = content_size;
    doc->content.data[content_size] = '\0';
    doc->sa_index = (cor_morph_doc_sa_el_t *) (doc->content.data + size + 1);
    for (int i = 0; i < sa_index; ++i) {
        doc->sa_index[i].position = morph->sa_ranges[i].begin;
    }
    doc->sa_index_size = sa_index;

    return doc;
}

double
cor_morph_doc_intersection_ratio(cor_morph_t *morph, cor_morph_doc_t *doc1, cor_morph_doc_t *doc2)
{
    return 0.0;
}

