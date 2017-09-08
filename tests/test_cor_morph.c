#include "cor_test.h"
#include "cor_dict.h"
#include "cor_morph.h"

typedef struct
{
    const char *word;
    const char *base;
} test_cor_words_t;

int test_cor_morph_fill_dict(cor_dict_t *dict, cor_pool_t *pool)
{
    static test_cor_words_t words[] = {
        {"МЫ", "МЫ"},
        {"ПОСТРОИМ", "ПОСТРОИЛ"},
        {"САМЫЙ", "САМЫЙ"},
        {"СОВРЕМЕННЫЙ", "СОВРЕМЕННЫЙ"},
        {"МАШИННЫЙ", "МАШИННЫЙ"},
        {"ЗАВОД", "ЗАВОД"},
        {"НА", "НА"},
        {"ГОРАХ", "ГОРА"},
        {NULL, NULL}
    };
    for (int i = 0; words[i].word; ++i) {
        cor_dict_word_t *word = (cor_dict_word_t *) cor_pool_calloc(pool, sizeof(cor_dict_word_t));
        test_ptr_ne(word, NULL);
        word->base = (cor_str_t *) cor_pool_alloc(pool, sizeof(cor_str_t));
        test_ptr_ne(word->base, NULL);
        word->base->size = strlen(words[i].base);
        word->base->data = (char *) cor_pool_alloc(pool, word->base->size + 1);
        test_ptr_ne(word->base->data, NULL);
        memcpy(word->base->data, words[i].base, word->base->size + 1);
        PWord_t pw;
        JSLI(pw, dict->words, (uint8_t *) words[i].word);
        if (pw == PJERR) {
            LOG_FAIL("can't JSLI");
            return -1;
        }
        *pw = (Word_t) word;
    }

    return 0;
}

BEGIN_TEST(test_cor_morph_content_to_base_form)
{
    cor_pool_t *pool = cor_pool_new(4096);
    test_ptr_ne(pool, NULL);
    cor_dict_t *dict = cor_dict_new(pool, NULL);
    test_ptr_ne(dict, NULL);
    int rc = test_cor_morph_fill_dict(dict, pool);
    test_int_eq(rc, cor_ok);
    cor_morph_t *morph = cor_morph_new(pool, NULL);
    test_ptr_ne(morph, NULL);
    cor_morph_set_dict(morph, dict);
    /**/
    static char source[] = "МЫ ПОСТРОИМ САМЫЙ СОВРЕМЕННЫЙ МАШИННЫЙ ЗАВОД НА КУДЫКИНЫХ ГОРАХ";
    static char expected[] = "МЫ ПОСТРОИЛ САМЫЙ СОВРЕМЕННЫЙ МАШИННЫЙ ЗАВОД НА КУДЫКИНЫХ ГОРА";
    cor_str_t *str = cor_morph_content_to_base_form(morph, source, strlen(source));
    test_ptr_ne(str, NULL);
    test_int_eq(str->size, strlen(expected));
    test_strn_eq(str->data, expected, str->size);
    /**/
    cor_morph_delete(morph);
    cor_dict_delete(dict);
    cor_pool_delete(pool);
}
END;

BEGIN_TEST(test_cor_morph_doc_new)
{
    cor_pool_t *pool = cor_pool_new(4096);
    test_ptr_ne(pool, NULL);
    cor_dict_t *dict = cor_dict_new(pool, NULL);
    test_ptr_ne(dict, NULL);
    int rc = test_cor_morph_fill_dict(dict, pool);
    test_int_eq(rc, cor_ok);
    cor_morph_t *morph = cor_morph_new(pool, NULL);
    test_ptr_ne(morph, NULL);
    cor_morph_set_dict(morph, dict);
    /**/
    static char source[] = "ABCDE ABCD AB ABC A МЫ ПОСТРОИМ САМЫЙ СОВРЕМЕННЫЙ МАШИННЫЙ ЗАВОД НА КУДЫКИНЫХ ГОРАХ";
    static char *sa_words[] = {"A", "AB", "ABC", "ABCD", "ABCDE", "ГОРАХ", "ЗАВОД", "КУДЫКИНЫХ", "МАШИННЫЙ", "МЫ", "НА", "ПОСТРОИМ", "САМЫЙ", "СОВРЕМЕННЫЙ"};
    cor_morph_doc_t *doc = cor_morph_doc_new(morph, source, strlen(source));
    test_ptr_ne(doc, NULL);
    test_strn_eq(doc->content.data, source, sizeof(source) - 1);
    test_int_eq(doc->sa_index_size, 14);
    for (int i = 0; i < doc->sa_index_size; ++i) {
        test_strn_eq(doc->content.data + doc->sa_index[i].position, sa_words[i], strlen(sa_words[i]));
    }
    /**/
    cor_morph_delete(morph);
    cor_dict_delete(dict);
    cor_pool_delete(pool);
}
END;

BEGIN_TEST(test_cor_morph_doc_pack)
{
    cor_pool_t *pool = cor_pool_new(4096);
    test_ptr_ne(pool, NULL);
    cor_dict_t *dict = cor_dict_new(pool, NULL);
    test_ptr_ne(dict, NULL);
    int rc = test_cor_morph_fill_dict(dict, pool);
    test_int_eq(rc, cor_ok);
    cor_morph_t *morph = cor_morph_new(pool, NULL);
    test_ptr_ne(morph, NULL);
    cor_morph_set_dict(morph, dict);
    /**/
    static char source[] = "ABCDE ABCD AB ABC A МЫ ПОСТРОИМ САМЫЙ СОВРЕМЕННЫЙ МАШИННЫЙ ЗАВОД НА КУДЫКИНЫХ ГОРАХ";
    static char *sa_words[] = {"A", "AB", "ABC", "ABCD", "ABCDE", "ГОРАХ", "ЗАВОД", "КУДЫКИНЫХ", "МАШИННЫЙ", "МЫ", "НА", "ПОСТРОИМ", "САМЫЙ", "СОВРЕМЕННЫЙ"};
    cor_morph_doc_t *doc = cor_morph_doc_new(morph, source, strlen(source));
    test_ptr_ne(doc, NULL);
    test_int_eq(doc->content.size, sizeof(source) - 1);
    test_strn_eq(doc->content.data, source, sizeof(source) - 1);
    test_int_eq(doc->sa_index_size, 14);
    for (int i = 0; i < doc->sa_index_size; ++i) {
        test_strn_eq(doc->content.data + doc->sa_index[i].position, sa_words[i], strlen(sa_words[i]));
    }
    /**/
    cor_str_t *doc_packed = cor_morph_doc_pack(morph, doc);
    test_ptr_ne(doc_packed, NULL);
    cor_morph_doc_delete(doc);
    doc = cor_morph_doc_unpack(morph, doc_packed->data, doc_packed->size);
    test_ptr_ne(doc, NULL);
    test_int_eq(doc->content.size, sizeof(source) - 1);
    test_strn_eq(doc->content.data, source, sizeof(source) - 1);
    test_int_eq(doc->sa_index_size, 14);
    for (int i = 0; i < doc->sa_index_size; ++i) {
        test_strn_eq(doc->content.data + doc->sa_index[i].position, sa_words[i], strlen(sa_words[i]));
    }
    cor_morph_doc_delete(doc);
    /**/
    cor_morph_delete(morph);
    cor_dict_delete(dict);
    cor_pool_delete(pool);
}
END;

int
main(int argc, char **argv)
{
    RUN_TEST(test_cor_morph_content_to_base_form);
    RUN_TEST(test_cor_morph_doc_new);
    RUN_TEST(test_cor_morph_doc_pack);

    exit(0);
}
