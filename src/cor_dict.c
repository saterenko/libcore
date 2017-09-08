#include "cor_dict.h"

#include "cor_mmap.h"
#include "cor_time.h"

#define COR_DICT_POLL_SIZE  1024 * 1024
#define COR_DICT_MAX_COR_DICT_GRMS 32
#define COR_DICT_MAX_WORD_SIZE 256

typedef struct
{
    int id;
    const char *key;
    const char *description;
} cor_dict_gramemme_t;

static const cor_dict_gramemme_t cor_dict_gramemmes[] = {
    {COR_DICT_GRM_NOUN, "noun", "имя существительное"},
    {COR_DICT_GRM_ADJF, "adjf", "имя прилагательное (полное)"},
    {COR_DICT_GRM_ADJS, "adjs", "имя прилагательное (краткое)"},
    {COR_DICT_GRM_COMP, "comp", "компаратив"},
    {COR_DICT_GRM_VERB, "verb", "глагол (личная форма)"},
    {COR_DICT_GRM_INFN, "infn", "глагол (инфинитив)"},
    {COR_DICT_GRM_PRTF, "prtf", "причастие (полное)"},
    {COR_DICT_GRM_PRTS, "prts", "причастие (краткое)"},
    {COR_DICT_GRM_GRND, "grnd", "деепричастие"},
    {COR_DICT_GRM_NUMR, "numr", "числительное"},
    {COR_DICT_GRM_ADVB, "advb", "наречие"},
    {COR_DICT_GRM_NPRO, "npro", "местоимение-существительное"},
    {COR_DICT_GRM_PRED, "pred", "предикатив"},
    {COR_DICT_GRM_PREP, "prep", "предлог"},
    {COR_DICT_GRM_CONJ, "conj", "союз"},
    {COR_DICT_GRM_PRCL, "prcl", "частица"},
    {COR_DICT_GRM_INTJ, "intj", "междометие"},
    {COR_DICT_GRM_ANIM, "anim", "одушевлённое"},
    {COR_DICT_GRM_INAN, "inan", "неодушевлённое"},
    {COR_DICT_GRM_GNDR, "gndr", "род / род не выражен"},
    {COR_DICT_GRM_MASC, "masc", "мужской род"},
    {COR_DICT_GRM_FEMN, "femn", "женский род"},
    {COR_DICT_GRM_NEUT, "neut", "средний род"},
    {COR_DICT_GRM_MS_F, "ms-f", "общий род"},
    {COR_DICT_GRM_SING, "sing", "единственное число"},
    {COR_DICT_GRM_PLUR, "plur", "множественное число"},
    {COR_DICT_GRM_SGTM, "sgtm", "singularia tantum"},
    {COR_DICT_GRM_PLTM, "pltm", "pluralia tantum"},
    {COR_DICT_GRM_FIXD, "fixd", "неизменяемое"},
    {COR_DICT_GRM_NOMN, "nomn", "именительный падеж"},
    {COR_DICT_GRM_GENT, "gent", "родительный падеж"},
    {COR_DICT_GRM_DATV, "datv", "дательный падеж"},
    {COR_DICT_GRM_ACCS, "accs", "винительный падеж"},
    {COR_DICT_GRM_ABLT, "ablt", "творительный падеж"},
    {COR_DICT_GRM_LOCT, "loct", "предложный падеж"},
    {COR_DICT_GRM_VOCT, "voct", "звательный падеж"},
    {COR_DICT_GRM_GEN1, "gen1", "первый родительный падеж"},
    {COR_DICT_GRM_GEN2, "gen2", "второй родительный (частичный) падеж"},
    {COR_DICT_GRM_ACC2, "acc2", "второй винительный падеж"},
    {COR_DICT_GRM_LOC1, "loc1", "первый предложный падеж"},
    {COR_DICT_GRM_LOC2, "loc2", "второй предложный (местный) падеж"},
    {COR_DICT_GRM_ABBR, "abbr", "аббревиатура"},
    {COR_DICT_GRM_NAME, "name", "имя"},
    {COR_DICT_GRM_SURN, "surn", "фамилия"},
    {COR_DICT_GRM_PATR, "patr", "отчество"},
    {COR_DICT_GRM_GEOX, "geox", "топоним"},
    {COR_DICT_GRM_ORGN, "orgn", "организация"},
    {COR_DICT_GRM_TRAD, "trad", "торговая марка"},
    {COR_DICT_GRM_SUBX, "subx", "возможна субстантивация"},
    {COR_DICT_GRM_SUPR, "supr", "превосходная степень"},
    {COR_DICT_GRM_QUAL, "qual", "качественное"},
    {COR_DICT_GRM_APRO, "apro", "местоименное"},
    {COR_DICT_GRM_ANUM, "anum", "порядковое"},
    {COR_DICT_GRM_POSS, "poss", "притяжательное"},
    {COR_DICT_GRM_V_EY, "v-ey", "форма на -ею"},
    {COR_DICT_GRM_V_OY, "v-oy", "форма на -ою"},
    {COR_DICT_GRM_CMP2, "cmp2", "сравнительная степень на по-"},
    {COR_DICT_GRM_V_EJ, "v-ej", "форма компаратива на -ей"},
    {COR_DICT_GRM_PERF, "perf", "совершенный вид"},
    {COR_DICT_GRM_IMPF, "impf", "несовершенный вид"},
    {COR_DICT_GRM_TRAN, "tran", "переходный"},
    {COR_DICT_GRM_INTR, "intr", "непереходный"},
    {COR_DICT_GRM_IMPE, "impe", "безличный"},
    {COR_DICT_GRM_IMPX, "impx", "возможно безличное употребление"},
    {COR_DICT_GRM_MULT, "mult", "многократный"},
    {COR_DICT_GRM_REFL, "refl", "возвратный"},
    {COR_DICT_GRM_PERS, "pers", "категория лица"},
    {COR_DICT_GRM_1PER, "1per", "1 лицо"},
    {COR_DICT_GRM_2PER, "2per", "2 лицо"},
    {COR_DICT_GRM_3PER, "3per", "3 лицо"},
    {COR_DICT_GRM_PRES, "pres", "настоящее время"},
    {COR_DICT_GRM_PAST, "past", "прошедшее время"},
    {COR_DICT_GRM_FUTR, "futr", "будущее время"},
    {COR_DICT_GRM_INDC, "indc", "изъявительное наклонение"},
    {COR_DICT_GRM_IMPR, "impr", "повелительное наклонение"},
    {COR_DICT_GRM_INCL, "incl", "говорящий включён (идем, идемте)"},
    {COR_DICT_GRM_EXCL, "excl", "говорящий не включён в действие (иди, идите)"},
    {COR_DICT_GRM_ACTV, "actv", "действительный залог"},
    {COR_DICT_GRM_PSSV, "pssv", "страдательный залог"},
    {COR_DICT_GRM_INFR, "infr", "разговорное"},
    {COR_DICT_GRM_SLNG, "slng", "жаргонное"},
    {COR_DICT_GRM_ARCH, "arch", "устаревшее"},
    {COR_DICT_GRM_LITR, "litr", "литературный вариант"},
    {COR_DICT_GRM_ERRO, "erro", "опечатка"},
    {COR_DICT_GRM_DIST, "dist", "искажение"},
    {COR_DICT_GRM_QUES, "ques", "вопросительное"},
    {COR_DICT_GRM_DMNS, "dmns", "указательное"},
    {COR_DICT_GRM_PRNT, "prnt", "вводное слово"},
    {COR_DICT_GRM_V_BE, "v-be", "форма на -ье"},
    {COR_DICT_GRM_V_EN, "v-en", "форма на -енен"},
    {COR_DICT_GRM_V_IE, "v-ie", "отчество через -ие-"},
    {COR_DICT_GRM_V_BI, "v-bi", "форма на -ьи"},
    {COR_DICT_GRM_FIMP, "fimp", "деепричастие от глагола несовершенного вида"},
    {COR_DICT_GRM_PRDX, "prdx", "может выступать в роли предикатива"},
    {COR_DICT_GRM_COUN, "coun", "счётная форма"},
    {COR_DICT_GRM_COLL, "coll", "собирательное числительное"},
    {COR_DICT_GRM_V_SH, "v-sh", "деепричастие на -ши"},
    {COR_DICT_GRM_AF_P, "af-p", "форма после предлога"},
    {COR_DICT_GRM_INMX, "inmx", "может использоваться как одуш. / неодуш."},
    {COR_DICT_GRM_VPRE, "vpre", "вариант предлога ( со, подо, ...)"},
    {COR_DICT_GRM_ANPH, "anph", "анафорическое (местоимение)"},
    {COR_DICT_GRM_INIT, "init", "инициал"},
    {COR_DICT_GRM_ADJX, "adjx", "может выступать в роли прилагательного"},
    {0, NULL, NULL}
};

static int cor_dict_make_gramemmes(cor_dict_t *dict);
static int cor_dict_parse_opencorpa(cor_dict_t *dict, cor_mmap_t *mmap);
static inline void cor_dict_parse_replace_yo(cor_str_t *str);

cor_dict_t *
cor_dict_new(cor_pool_t *pool, cor_log_t *log)
{
    int delete_pool = 0;
    if (!pool) {
        pool = cor_pool_new(COR_DICT_POLL_SIZE);
        if (!pool) {
            cor_log_error(log, "can't cor_pool_new");
            return NULL;
        }
        delete_pool = 1;
    }
    cor_dict_t *dict = (cor_dict_t *) cor_pool_calloc(pool, sizeof(cor_dict_t));
    if (!dict) {
        cor_log_error(log, "can't cor_pool_new");
        if (delete_pool) {
            cor_pool_delete(pool);
        }
        return NULL;
    }
    dict->pool = pool;
    dict->delete_pool = delete_pool;
    dict->log = log;
    if (cor_dict_make_gramemmes(dict) != cor_ok) {
        cor_log_error(log, "can't wrk_dict_make_gramemmes");
        cor_dict_delete(dict);
        return NULL;
    }

    return dict;
}

void
cor_dict_delete(cor_dict_t *dict)
{
    if (dict) {
        if (dict->pool && dict->delete_pool) {
            cor_pool_delete(dict->pool);
        }
    }
}

int
cor_dict_get_gramemme_id(cor_dict_t *dict, const char *gramemme, size_t size)
{
    uint8_t key[8];
    if (size > 7) {
        cor_log_error(dict->log, "gramemme to long: %.*s", size, gramemme);
        return cor_error;
    }
    for (int i = 0; i < size; i++) {
        key[i] = cor_str_tolower(gramemme[i]);
    }
    key[size] = '\0';
    PWord_t pw;
    JSLG(pw, dict->gramemmes_by_key, key);

    return pw ? *pw : 0;
}

cor_dict_word_t *
cor_dict_get_word(cor_dict_t *dict, const char *word)
{
    PWord_t pw;
    JSLG(pw, dict->words, (uint8_t *) word);
    if (pw) {
        return (cor_dict_word_t *) *pw;
    }
    return NULL;
}

void
cor_dict_dump(cor_dict_t *dict)
{
    PWord_t pw;
    char key[COR_DICT_MAX_WORD_SIZE];
    key[0] = '\0';
    JSLF(pw, dict->words, (uint8_t *) key);
    while (pw) {
        cor_dict_word_t *word = (cor_dict_word_t *) *pw;
        printf("%s\t", key);
        if (word->base) {
            printf("%s\t", word->base->data);
        } else {
            printf("\t");
        }
        if (word->gramemmes) {
            for (int i = 0; word->gramemmes[i]; ++i) {
                if (i == 0) {
                    printf("%d", word->gramemmes[i]);
                } else {
                    printf(",%d", word->gramemmes[i]);
                }
            }
        }
        printf("\n");
        JSLN(pw, dict->words, (uint8_t *) key);
    }
}

int
cor_dict_load_opencorpa(cor_dict_t *dict, const char *file)
{
    struct timeval begin;
    struct timeval end;
    gettimeofday(&begin, NULL);
    /**/
    cor_mmap_t *mmap = cor_mmap_open(file, dict->log);
    if (!mmap) {
        cor_log_error(dict->log, "can't cor_mmap_open %s", file);
        return cor_error;
    }
    if (cor_dict_parse_opencorpa(dict, mmap) != cor_ok) {
        cor_log_error(dict->log, "can't parse_opencorpa");
        cor_mmap_close(mmap);
        if (dict->gramemmes_hash) {
            Word_t rc;
            JHSFA(rc, dict->gramemmes_hash);
        }
        return cor_error;
    }
    cor_mmap_close(mmap);
    if (dict->gramemmes_hash) {
        Word_t rc;
        JHSFA(rc, dict->gramemmes_hash);
    }
    gettimeofday(&end, NULL);
    cor_log_info(dict->log, "opencorpa loaded in %f sec", cor_time_diff(&begin, &end));

    return cor_ok;
}

int
cor_dict_make_gramemmes(cor_dict_t *dict)
{
    for (int i = 0; cor_dict_gramemmes[i].key; i++) {
        PWord_t pw;
        JSLI(pw, dict->gramemmes_by_key, (uint8_t *) cor_dict_gramemmes[i].key);
        if (pw == PJERR) {
            cor_log_error(dict->log, "can't JSLI");
            return cor_error;
        }
        *pw = (Word_t) cor_dict_gramemmes[i].id;
    }

    return cor_ok;
}

int
cor_dict_parse_opencorpa(cor_dict_t *dict, cor_mmap_t *mmap)
{
    enum {
        s_id,
        s_base_form,
        s_base_form_content,
        s_word,
        s_word_content,
        s_word_skip,
        s_word_gramemme
    } state;
    state = s_id;
    const char *p = mmap->src;
    const char *end = mmap->src + mmap->size;
    const char *begin = NULL;
    cor_str_t *base_form = NULL;
    cor_str_t *word_str = cor_str_new(dict->pool, COR_DICT_MAX_WORD_SIZE);
    if (!word_str) {
        cor_log_error(dict->log, "can't cor_str_new");
        return cor_error;
    }
    cor_dict_word_t *word = NULL;
    int gramemme_index = 0;
    int gramemmes[COR_DICT_MAX_COR_DICT_GRMS];
    int line = 1;
    for ( ; p < end; p++) {
        char c = *p;
        switch (state) {
            case s_id:
                if (c >= '0' && c <= '9') {
                    break;
                }
                if (c == '\n') {
                    line++;
                    begin = p + 1;
                    state = s_base_form;
                } else {
                    cor_log_error(dict->log, "unexpected number in line %d", line);
                    return cor_error;
                }
                break;
            case s_base_form:
                if (c == '\n') {
                    line++;
                    state = s_id;
                } else {
                    state = s_base_form_content;
                }
                break;
            case s_base_form_content:
                if (c == '\t' || c == '\n') {
                    /*  make base form string  */
                    base_form = cor_str_make_from_charptr(dict->pool, begin, p - begin);
                    if (!base_form) {
                        cor_log_error(dict->log, "can't cor_str_make_from_charptr");
                        return cor_error;
                    }
                    cor_dict_parse_replace_yo(base_form);
                    /*  get or make base form word  */
                    PWord_t pw;
                    JSLI(pw, dict->words, (uint8_t *) base_form->data);
                    if (pw == PJERR) {
                        cor_log_error(dict->log, "can't JSLI");
                        return cor_error;
                    } else if (*pw) {
                        word = (cor_dict_word_t *) *pw;
                    } else {
                        word = (cor_dict_word_t *) cor_pool_calloc(dict->pool, sizeof(cor_dict_word_t));
                        if (!word) {
                            cor_log_error(dict->log, "can't cor_pool_alloc");
                            return cor_error;
                        }
                        *pw = (Word_t) word;
                    }
                    /*  next  */
                    if (c == '\t') {
                        begin = p + 1;
                        gramemme_index = 0;
                        state = s_word_gramemme;
                    } else {
                        line++;
                        begin = p + 1;
                        state = s_word;
                    }
                }
                break;
            case s_word:
                if (c == '\n') {
                    line++;
                    state = s_id;
                } else {
                    state = s_word_content;
                }
                break;
            case s_word_content:
                if (c == '\t' || c == '\n') {
                    if (p - begin > COR_DICT_MAX_WORD_SIZE) {
                        cor_log_error(dict->log, "word %.*s in %d line to long", p - begin, begin, line);
                        return cor_error;
                    }
                    cor_str_fill_from_charptr(word_str, begin, p - begin);
                    cor_dict_parse_replace_yo(word_str);
                    PWord_t pw;
                    JSLI(pw, dict->words, (uint8_t *) word_str->data);
                    if (pw == PJERR) {
                        cor_log_error(dict->log, "can't JSLI");
                        return cor_error;
                    } else if (*pw) {
                        if (c == '\n') {
                            line++;
                            begin = p + 1;
                            state = s_word;
                        } else {
                            state = s_word_skip;
                        }
                    } else {
                        word = (cor_dict_word_t *) cor_pool_calloc(dict->pool, sizeof(cor_dict_word_t));
                        if (!word) {
                            cor_log_error(dict->log, "can't cor_pool_alloc");
                            return cor_error;
                        }
                        word->base = base_form;
                        *pw = (Word_t) word;
                        /*  next  */
                        begin = p + 1;
                        if (c == '\n') {
                            line++;
                            state = s_word;
                        } else {
                            gramemme_index = 0;
                            state = s_word_gramemme;
                        }
                    }
                }
                break;
            case s_word_skip:
                if (c == '\n') {
                    line++;
                    begin = p + 1;
                    state = s_word;
                }
                break;
            case s_word_gramemme:
                if (c == ',' || c == ' ' || c == '\n') {
                    if (p > begin) {
                        int gramemme_id = cor_dict_get_gramemme_id(dict, begin, p - begin);
                        if (gramemme_id) {
                            gramemmes[gramemme_index++] = gramemme_id;
                        } else {
                            cor_log_warn(dict->log, "unknown gramemme %.*s in line %d", p - begin, begin, line);
                        }
                    }
                    if (c == '\n') {
                        if (gramemme_index > 0) {
                            PWord_t pw;
                            JHSI(pw, dict->gramemmes_hash, &gramemmes[0], sizeof(int) * gramemme_index);
                            if (pw == PJERR) {
                                cor_log_error(dict->log, "can't JHSI");
                                return cor_error;
                            } else if (*pw) {
                                /*  gramemme exists  */
                                word->gramemmes = (int *) *pw;
                            } else {
                                /*  create new gramemme  */
                                int *word_gramemmes = (int *) cor_pool_alloc(dict->pool, sizeof(int) * (gramemme_index + 1));
                                if (!word_gramemmes) {
                                    cor_log_error(dict->log, "can't cor_pool_alloc");
                                    return cor_error;
                                }
                                memcpy(word_gramemmes, gramemmes, sizeof(int) * gramemme_index);
                                word_gramemmes[gramemme_index] = '\0';
                                /*  save gramemme in hash  */
                                *pw = (Word_t) word_gramemmes;
                            }
                        }
                        begin = p + 1;
                        state = s_word;
                    } else {
                        begin = p + 1;
                    }
                }
                break;
        }
    }

    return cor_ok;
}

void cor_dict_parse_replace_yo(cor_str_t *str)
{
    for (int i = 0; i < str->size; ++i) {
        if ((uint8_t) str->data[i] == 0xd0 && (uint8_t) str->data[i + 1] == 0x81) {
            str->data[++i] = 0x95;
        }
    }
}
