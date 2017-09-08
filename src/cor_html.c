#include "cor_html.h"

#include <stdio.h>
#include <stdlib.h>

#define COR_HTML_BUF_SIZE 1024
#define COR_HTML_POOL_SIZE (32 * 1024)

#define cor_html_char_tolower(_c) (_c >= 'A' && _c <= 'Z') ? (_c | 0x20) : _c
#define cor_html_char_toupper(_c) (_c >= 'a' && _c <= 'z') ? (_c & ~0x20) : _c

/* Внимание!!! Элементы в этой таблице строго соответствуют элементам в enum cor_html_tag_e.
 * Помимо этого хеш-функция подобрана под этот список тегов таким образом, чтобы не было коллизий.
 * Это нужно учитывать при обновлении этой таблицы, cor_html_tag_e или хеш-функции.
*/
static const char *cor_html_tag_names[] = {"(null)", "a", "abbr", "acronym", "address", "applet", "area", "article",
    "aside", "audio", "b", "base", "basefont", "bdi", "bdo", "big", "blockquote", "body",
    "br", "button", "canvas", "caption", "center", "cite", "code", "col", "colgroup",
    "command", "datalist", "dd", "del", "details", "dfn", "dialog", "dir", "div", "dl", "dt", "em",
    "embed", "fieldset", "figcaption", "figure", "font", "footer", "form", "frame",
    "frameset", "h1", "h2", "h3", "h4", "h5", "h6", "head", "header", "hgroup", "hr",
    "html", "i", "iframe", "img", "input", "ins", "kbd", "keygen", "label", "legend",
    "li", "link", "map", "mark", "menu", "meta", "meter", "nav", "noframes", "noscript",
    "object", "ol", "optgroup", "option", "output", "p", "param", "pre", "progress", "q",
    "rp", "rt", "ruby", "s", "samp", "script", "section", "select", "small", "source",
    "span", "strike", "strong", "style", "sub", "summary", "sup", "table", "tbody", "td",
    "textarea", "tfoot", "th", "thead", "time", "title", "tr", "track", "tt", "u", "ul",
    "var", "video", "wbr", NULL};

static inline cor_html_doc_node_t *cor_html_parser_add_node(cor_html_t *ctx, cor_html_doc_node_t *parent, int id);
static inline int cor_html_doc_parse_add_node_content(cor_html_t *ctx, cor_html_doc_node_t *node, const char *str, size_t size);
static char *cor_html_doc_to_plain_cat(cor_html_t *ctx, cor_html_doc_node_t *node, char *p);
static char *cor_html_doc_to_plain_cat_normalized(cor_html_t *ctx, char *dst, const char *content, int content_size);
static void cor_html_make_tag_hash_table(cor_html_t *ctx);
static void cor_html_make_void_tags(cor_html_t *ctx);
static inline int cor_html_tag_hash(const char *tag, size_t size);
static inline int cor_html_get_tag_id(cor_html_t *ctx, const char *tag, size_t size);
static int cor_html_expand_buf(cor_html_t *ctx, int size);
static inline int cor_html_is_tag_bitmap_set(int *bitmap, int id);

cor_html_t *
cor_html_new(cor_log_t *log)
{
    /**/
    cor_html_t *ctx = malloc(sizeof(cor_html_t));
    if (!ctx) {
        cor_log_error(log, "can't malloc");
        return NULL;
    }
    memset(ctx, 0, sizeof(cor_html_t));
    /**/
    ctx->pool = cor_pool_new(COR_HTML_POOL_SIZE);
    if (!ctx->pool) {
        cor_log_error(log, "can't cor_pool_new");
        return NULL;
    }
    ctx->log = log;
    /**/
    ctx->buf.data = malloc(COR_HTML_BUF_SIZE);
    if (!ctx->buf.data) {
        cor_log_error(log, "can't malloc");
        cor_html_delete(ctx);
        return NULL;
    }
    ctx->buf_size = COR_HTML_BUF_SIZE;
    /**/
    cor_html_make_tag_hash_table(ctx);
    cor_html_make_void_tags(ctx);

    return ctx;
}

void
cor_html_delete(cor_html_t *ctx)
{
    if (ctx) {
        if (ctx->buf.data) {
            free(ctx->buf.data);
        }
        if (ctx->pool) {
            cor_pool_delete(ctx->pool);
        }
        free(ctx);
    }
}

cor_html_doc_node_t *
cor_html_parse(cor_html_t *ctx, const char *str, int size)
{
    enum {
        s_space,
        s_tag_lt,
        s_comment,
        s_comment_dash,
        s_comment_skip,
        s_comment_skip_dash,
        s_comment_skip_dash_dash,
        s_skip_tag_to_end,
        s_open_tag_name,
        s_open_tag_attr,
        s_open_tag_slash,
        s_close_tag_name,
        s_script,
        s_script_gt,
        s_script_lt,
        s_script_lt_slash,
        s_script_lt_slash_s,
        s_script_lt_slash_sc,
        s_script_lt_slash_scr,
        s_script_lt_slash_scri,
        s_script_lt_slash_scrip,
        s_script_lt_slash_script
    } state;
    /**/
    char *bp = ctx->buf.data;
    int bp_index = 0;
    cor_pool_reset(ctx->pool);
    memset(&ctx->doc, 0, sizeof(cor_html_doc_node_t));
    cor_html_doc_node_t *node = &ctx->doc;
    /**/
    state = s_space;
    const char *p = str;
    const char *end = p + size;
    const char *begin = p;
    for (; p < end; p++) {
        char c = *p;
        switch (state) {
            case s_space:
                if (c == '<') {
                    if (node != &ctx->doc && p > begin) {
                        if (cor_html_doc_parse_add_node_content(ctx, node, begin, p - begin) != cor_ok) {
                            cor_log_error(ctx->log, "can't cor_html_doc_parse_add_node_content");
                            return NULL;
                        }
                    }
                    state = s_tag_lt;
                }
                break;
            case s_tag_lt:
                if (c == '/') {
                    bp_index = 0;
                    state = s_close_tag_name;
                } else if (c == '!') {
                    state = s_comment;
                } else {
                    bp_index = 0;
                    bp[bp_index++] = cor_html_char_tolower(c);
                    state = s_open_tag_name;
                }
                break;
            case s_comment:
                if (c == '-') {
                    state = s_comment_dash;
                } else {
                    state = s_skip_tag_to_end;
                }
                break;
            case s_comment_dash:
                if (c == '-') {
                    state = s_comment_skip;
                } else {
                    state = s_skip_tag_to_end;
                }
                break;
            case s_comment_skip:
                if (c == '-') {
                    state = s_comment_skip_dash;
                }
                break;
            case s_comment_skip_dash:
                if (c == '-') {
                    state = s_comment_skip_dash_dash;
                } else {
                    state = s_comment_skip;
                }
                break;
            case s_comment_skip_dash_dash:
                if (c == '>') {
                    begin = p + 1;
                    state = s_space;
                } else {
                    state = s_comment_skip;
                }
                break;
            case s_skip_tag_to_end:
                if (c == '>') {
                    begin = p + 1;
                    state = s_space;
                }
                break;
            case s_open_tag_name:
                if (c == ' ' || c == '>' || c == '/' || c == '\t' || c == '\n' || c == '\r') {
                    int id = cor_html_get_tag_id(ctx, bp, bp_index);
                    /*  скрипт мы просто пропускаем  */
                    if (id == COR_HTML_TAG_SCRIPT) {
                        if (c == '>') {
                            state = s_script_gt;
                        } else {
                            state = s_script;
                        }
                        break;
                    }
                    /*  добавляем ноду  */
                    node = cor_html_parser_add_node(ctx, node, id);
                    if (!node) {
                        cor_log_error(ctx->log, "can't cor_html_parser_add_node");
                        return NULL;
                    }
                    if (c == '>') {
                        begin = p + 1;
                        state = s_space;
                    } else if (c == '/') {
                        state = s_open_tag_slash;
                    } else {
                        state = s_open_tag_attr;
                    }
                } else {
                    bp[bp_index++] = cor_html_char_tolower(c);
                    if (bp_index == ctx->buf_size) {
                        if (cor_html_expand_buf(ctx, 0) != cor_ok) {
                            cor_log_error(ctx->log, "can't cor_html_expand_buf");
                            return NULL;
                        }
                        bp = ctx->buf.data;
                    }
                }
                break;
            case s_open_tag_attr:
                if (c == '>') {
                    begin = p + 1;
                    state = s_space;
                } else if (c == '/') {
                    state = s_open_tag_slash;
                }
                break;
            case s_open_tag_slash:
                if (c == '>') {
                    begin = p + 1;
                    state = s_space;
                }
                break;
            case s_close_tag_name:
                if (c == '>') {
                    /*  возвращаемся к предку  */
                    int id = cor_html_get_tag_id(ctx, bp, bp_index);
                    for (; node != &ctx->doc; node = node->parent) {
                        if (node->tag == id) {
                            node = node->parent;
                            break;
                        }
                    }
                    begin = p + 1;
                    state = s_space;
                } else {
                    bp[bp_index++] = cor_html_char_tolower(c);
                    if (bp_index == ctx->buf_size) {
                        if (cor_html_expand_buf(ctx, 0) != cor_ok) {
                            cor_log_error(ctx->log, "can't cor_html_expand_buf");
                            return NULL;
                        }
                        bp = ctx->buf.data;
                    }
                }
                break;
            case s_script:
                if (c == '>') {
                    state = s_script_gt;
                }
                break;
            case s_script_gt:
                if (c == '<') {
                    state = s_script_lt;
                }
                break;
            case s_script_lt:
                if (c == '/') {
                    state = s_script_lt_slash;
                } else {
                    state = s_script_lt;
                }
                break;
            case s_script_lt_slash:
                if (cor_html_char_tolower(c) == 's') {
                    state = s_script_lt_slash_s;
                } else {
                    state = s_script_lt;
                }
                break;
            case s_script_lt_slash_s:
                if (cor_html_char_tolower(c) == 'c') {
                    state = s_script_lt_slash_sc;
                } else {
                    state = s_script_lt;
                }
                break;
            case s_script_lt_slash_sc:
                if (cor_html_char_tolower(c) == 'r') {
                    state = s_script_lt_slash_scr;
                } else {
                    state = s_script_lt;
                }
                break;
            case s_script_lt_slash_scr:
                if (cor_html_char_tolower(c) == 'i') {
                    state = s_script_lt_slash_scri;
                } else {
                    state = s_script_lt;
                }
                break;
            case s_script_lt_slash_scri:
                if (cor_html_char_tolower(c) == 'p') {
                    state = s_script_lt_slash_scrip;
                } else {
                    state = s_script_lt;
                }
                break;
            case s_script_lt_slash_scrip:
                if (cor_html_char_tolower(c) == 't') {
                    state = s_script_lt_slash_script;
                } else {
                    state = s_script_lt;
                }
                break;
            case s_script_lt_slash_script:
                if (c == '>') {
                    begin = p + 1;
                    state = s_space;
                } else {
                    state = s_script_lt;
                }
                break;
        }
    }

    return &ctx->doc;
}

cor_html_doc_node_t *
cor_html_parser_add_node(cor_html_t *ctx, cor_html_doc_node_t *parent, int id)
{
    /*  проверяем, что это void-tag (br, hr и т.п.)*/
    if (cor_html_is_tag_bitmap_set(&ctx->void_tags_table[0], id)) {
        return parent;
    }
    /*  добавляем ноду  */
    cor_html_doc_node_t *node = cor_pool_calloc(ctx->pool, sizeof(cor_html_doc_node_t));
    if (!node) {
        cor_log_error(ctx->log, "can't cor_pool_calloc");
        return NULL;
    }
    node->tag = id;
    node->parent = parent;
    /*  прописываем родительской ноде в список nodes  */
    if (parent->nodes_head) {
        parent->nodes_tail->nodes_next = node; 
        parent->nodes_tail = node;
    } else {
        parent->nodes_head = parent->nodes_tail = node;
    }

    return node;
}

int
cor_html_doc_parse_add_node_content(cor_html_t *ctx, cor_html_doc_node_t *parent, const char *str, size_t size)
{
    cor_html_doc_node_t *node = cor_html_parser_add_node(ctx, parent, COR_HTML_TAG_NULL);
    if (!node) {
        cor_log_error(ctx->log, "can't cor_html_parser_add_node");
        return cor_error;
    }
    node->content = str;
    node->content_size = size;

    return cor_ok;
}

void
cor_html_dump(cor_html_t *ctx, cor_html_doc_node_t *node, int level)
{
    for (cor_html_doc_node_t *n = node->nodes_head; n; n = n->nodes_next) {
        if (n->tag == COR_HTML_TAG_NULL) {
            if (n->content_size) {
                printf("%d: content: %.*s\n", level, n->content_size, n->content);
            }
        } else {
            printf("%d: node: %s\n", level, cor_html_tag_names[n->tag]);
            cor_html_dump(ctx, n, level + 1);
        }
    }
}

cor_str_t *
cor_html_doc_to_plain(cor_html_t *ctx, cor_html_doc_node_t *node)
{
    char *end = cor_html_doc_to_plain_cat(ctx, node, ctx->buf.data);
    if (!end) {
        cor_log_error(ctx->log, "can't cor_html_doc_to_plain_cat");
        return NULL;
    }
    for (int i = end - ctx->buf.data - 1; i >= 0; --i, --end) {
        if (ctx->buf.data[i] != ' ') {
            break;
        }
    }
    ctx->buf.size = end - ctx->buf.data;
    cor_str_utf8_to_lower(ctx->buf.data, ctx->buf.size);

    return &ctx->buf;
}

static char *
cor_html_doc_to_plain_cat(cor_html_t *ctx, cor_html_doc_node_t *node, char *p)
{
    for (cor_html_doc_node_t *n = node->nodes_head; n; n = n->nodes_next) {
        if (n->tag == COR_HTML_TAG_NULL) {
            if (n->content_size) {
                p = cor_html_doc_to_plain_cat_normalized(ctx, p, n->content, n->content_size);
                if (!p) {
                    cor_log_error(ctx->log, "can't cor_html_doc_to_plain_cat_normalized");
                    return NULL;
                }
            }
        } else {
            p = cor_html_doc_to_plain_cat(ctx, n, p);
            if (!p) {
                cor_log_error(ctx->log, "can't cor_html_doc_to_plain_cat");
                return NULL;
            }
        }
    }

    return p;
}

static char *
cor_html_doc_to_plain_cat_normalized(cor_html_t *ctx, char *dst, const char *content, int content_size)
{
    static const uint32_t allowed_table[] = {0x00000000, 0x03ff2000, 0x87fffffe,
        0x07fffffe, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};
    /*  check for free space  */
    int position = dst - ctx->buf.data;
    int need_size = position + content_size + 1;
    if (need_size > ctx->buf_size) {
        if (cor_html_expand_buf(ctx, need_size) != cor_ok) {
            cor_log_error(ctx->log, "can't cor_html_expand_buf");
            return NULL;
        }
        dst = ctx->buf.data + position;
    }
    /*  parse  */
    enum {
        s_begin,
        s_char,
        s_amp,
        s_special_c2,
        s_special_e2,
        s_special_e2_2
    } state;
    state = s_begin;
    int space = 0;
    char *begin = dst;
    const char *p = content;
    const char *end = p + content_size;
    for (; p < end; ++p) {
        uint8_t c = (uint8_t) *p;
        switch (state) {
            case s_begin:
                if (c == '&') {
                    state = s_amp;
                } else if (!(allowed_table[c >> 5] & (0x1 << (c & 0x1f)))) {
                    break;
                } else if (c == 0xc2) {
                    state = s_special_c2;
                } else if (c == 0xe2) {
                    state = s_special_e2;
                } else {
                    *(dst++) = c;
                    state = s_char;
                }
                break;
            case s_char:
                if (c == '&') {
                    state = s_amp;
                } else if (!(allowed_table[c >> 5] & (0x1 << (c & 0x1f)))) {
                    if (!space) {
                        *(dst++) = ' ';
                        space = 1;
                    }
                } else if (c == 0xc2) {
                    state = s_special_c2;
                } else if (c == 0xe2) {
                    state = s_special_e2;
                } else {
                    space = 0;
                    *(dst++) = c;
                }
                break;
            case s_amp:
                if (c == ';') {
                    if (!space) {
                        *(dst++) = ' ';
                        space = 1;
                    }
                    state = s_char;
                }
                break;
            case s_special_c2:
                state = s_char;
                break;
            case s_special_e2:
                if (c >= 0x80 && c <= 0xad) {
                    state = s_special_e2_2;
                } else {
                    dst[0] = 0xe2;
                    dst[1] = c;
                    dst += 2;
                    state = s_char;
                }
                break;
            case s_special_e2_2:
                state = s_char;
                break;
        }
    }
    if (dst > begin && dst[-1] != ' ') {
        (*dst++) = ' ';
    }

    return dst;
}

static void
cor_html_make_tag_hash_table(cor_html_t *ctx)
{
    memset(ctx->tags_hash_table, 0, sizeof(int) * 1024);
    for (int i = 1; cor_html_tag_names[i]; ++i) {
        int key = cor_html_tag_hash(cor_html_tag_names[i], strlen(cor_html_tag_names[i]));
        ctx->tags_hash_table[key] = i;
    }
}

static void
cor_html_make_void_tags(cor_html_t *ctx)
{
    static const int void_tags[] = {
        COR_HTML_TAG_AREA,
        COR_HTML_TAG_BASE,
        COR_HTML_TAG_BR,
        COR_HTML_TAG_COL,
        COR_HTML_TAG_COMMAND,
        COR_HTML_TAG_EMBED,
        COR_HTML_TAG_HR,
        COR_HTML_TAG_IMG,
        COR_HTML_TAG_INPUT,
        COR_HTML_TAG_KEYGEN,
        COR_HTML_TAG_LINK,
        COR_HTML_TAG_META,
        COR_HTML_TAG_PARAM,
        COR_HTML_TAG_SOURCE,
        COR_HTML_TAG_TRACK,
        COR_HTML_TAG_WBR,
        0
    };
    for (int i = 1; void_tags[i]; ++i) {
        int index = void_tags[i] / (8 * sizeof(int));
        unsigned int mask = 1U << (void_tags[i] - (index * 8 * sizeof(int)));
        ctx->void_tags_table[index] |= mask;
    }
}

static int
cor_html_tag_hash(const char *tag, size_t size)
{
    unsigned int key = 374761393;
    for (int i = 0; i < size; ++i) {
        key = key * 729 + tag[i];
    }
    return key & 0x3ff; /*  10 bits  */
}

static int
cor_html_get_tag_id(cor_html_t *ctx, const char *tag, size_t size)
{
    int id = ctx->tags_hash_table[cor_html_tag_hash(tag, size)];
    const char *name = cor_html_tag_names[id];
    if (strncmp(tag, name, size) == 0) {
        return id;
    }
    return 0;
}

static int
cor_html_expand_buf(cor_html_t *ctx, int size)
{
    if (size == 0) {
        size = ctx->buf_size * 2 - ctx->buf_size / 2;
    } else if (size <= ctx->buf_size) {
        return cor_ok;
    } else {
        int ns = ctx->buf_size;
        while (ns < size) {
            ns = ns * 2 - ns / 2;
        }
        size = ns;
    }
    char *buf = realloc(ctx->buf.data, size);
    if (!buf) {
        cor_log_error(ctx->log, "can't realloc");
        return cor_error;
    }
    ctx->buf.data = buf;
    ctx->buf_size = size;

    return cor_ok;
}

static int
cor_html_is_tag_bitmap_set(int *bitmap, int id)
{
    int index = id / (8 * sizeof(int));
    unsigned int mask = 1U << (id - (index * 8 * sizeof(int)));
    return bitmap[index] & mask;
}

