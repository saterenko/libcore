#ifndef COR_MORPH_H
#define COR_MORPH_H

#include "cor_core.h"
#include "cor_dict.h"
#include "cor_log.h"
#include "cor_pool.h"

typedef struct
{
    int begin;
    int end;
} cor_morph_doc_sa_range_t;

typedef struct
{
    int position;
    int lsb;
} cor_morph_doc_sa_el_t;

typedef struct
{
    int sa_index_size;
    cor_morph_doc_sa_el_t *sa_index;
    cor_str_t content;
} cor_morph_doc_t;

typedef struct
{
    size_t content_buffer_size;
    cor_str_t content;
    cor_dict_t *dict;
    int sa_index_size;
    const char **sa_index;
    const char **sa_index2;
    cor_morph_doc_sa_range_t *sa_ranges;
    int radix[256];
    int radix2[256];
    /**/
    unsigned delete_pool:1;
    cor_pool_t *pool;
    cor_log_t *log;
} cor_morph_t;

cor_morph_t *cor_morph_new(cor_pool_t *pool, cor_log_t *log);
void cor_morph_delete(cor_morph_t *morph);
void cor_morph_set_dict(cor_morph_t *morph, cor_dict_t *dict);
/*  на вход получаем текст, где слова разделены пробелами  */
cor_str_t *cor_morph_content_to_base_form(cor_morph_t *morph, const char *content, size_t size);
cor_morph_doc_t *cor_morph_doc_new(cor_morph_t *morph, const char *content, size_t size);
void cor_morph_doc_delete(cor_morph_doc_t *doc);
cor_str_t *cor_morph_doc_pack(cor_morph_t *morph, cor_morph_doc_t *doc);
cor_morph_doc_t *cor_morph_doc_unpack(cor_morph_t *morph, const char *content, size_t size);
double cor_morph_doc_intersection_ratio(cor_morph_t *morph, cor_morph_doc_t *doc1, cor_morph_doc_t *doc2);


#endif
