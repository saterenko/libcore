#ifndef COR_HTML_H
#define COR_HTML_H

#include <stdint.h>
#include <string.h>

#include "cor_core.h"
#include "cor_log.h"
#include "cor_pool.h"
#include "cor_str.h"

enum cor_html_tag_e
{
    COR_HTML_TAG_NULL = 0,
    COR_HTML_TAG_A, // 1
    COR_HTML_TAG_ABBR,
    COR_HTML_TAG_ACRONYM,
    COR_HTML_TAG_ADDRESS,
    COR_HTML_TAG_APPLET,
    COR_HTML_TAG_AREA,
    COR_HTML_TAG_ARTICLE,
    COR_HTML_TAG_ASIDE,
    COR_HTML_TAG_AUDIO,
    COR_HTML_TAG_B, // 10
    COR_HTML_TAG_BASE,
    COR_HTML_TAG_BASEFONT,
    COR_HTML_TAG_BDI,
    COR_HTML_TAG_BDO,
    COR_HTML_TAG_BIG,
    COR_HTML_TAG_BLOCKQUOTE,
    COR_HTML_TAG_BODY,
    COR_HTML_TAG_BR,
    COR_HTML_TAG_BUTTON,
    COR_HTML_TAG_CANVAS, // 20
    COR_HTML_TAG_CAPTION,
    COR_HTML_TAG_CENTER,
    COR_HTML_TAG_CITE,
    COR_HTML_TAG_CODE,
    COR_HTML_TAG_COL,
    COR_HTML_TAG_COLGROUP,
    COR_HTML_TAG_COMMAND,
    COR_HTML_TAG_DATALIST,
    COR_HTML_TAG_DD,
    COR_HTML_TAG_DEL, // 30
    COR_HTML_TAG_DETAILS,
    COR_HTML_TAG_DFN,
    COR_HTML_TAG_DIALOG,
    COR_HTML_TAG_DIR,
    COR_HTML_TAG_DIV,
    COR_HTML_TAG_DL,
    COR_HTML_TAG_DT,
    COR_HTML_TAG_EM,
    COR_HTML_TAG_EMBED,
    COR_HTML_TAG_FIELDSET, // 40
    COR_HTML_TAG_FIGCAPTION,
    COR_HTML_TAG_FIGURE,
    COR_HTML_TAG_FONT,
    COR_HTML_TAG_FOOTER,
    COR_HTML_TAG_FORM,
    COR_HTML_TAG_FRAME,
    COR_HTML_TAG_FRAMESET,
    COR_HTML_TAG_H1,
    COR_HTML_TAG_H2,
    COR_HTML_TAG_H3, // 50
    COR_HTML_TAG_H4,
    COR_HTML_TAG_H5,
    COR_HTML_TAG_H6,
    COR_HTML_TAG_HEAD,
    COR_HTML_TAG_HEADER,
    COR_HTML_TAG_HGROUP,
    COR_HTML_TAG_HR,
    COR_HTML_TAG_HTML,
    COR_HTML_TAG_I,
    COR_HTML_TAG_IFRAME, // 60
    COR_HTML_TAG_IMG,
    COR_HTML_TAG_INPUT,
    COR_HTML_TAG_INS,
    COR_HTML_TAG_KBD,
    COR_HTML_TAG_KEYGEN,
    COR_HTML_TAG_LABEL,
    COR_HTML_TAG_LEGEND,
    COR_HTML_TAG_LI,
    COR_HTML_TAG_LINK,
    COR_HTML_TAG_MAP, // 70
    COR_HTML_TAG_MARK,
    COR_HTML_TAG_MENU,
    COR_HTML_TAG_META,
    COR_HTML_TAG_METER,
    COR_HTML_TAG_NAV,
    COR_HTML_TAG_NOFRAMES,
    COR_HTML_TAG_NOSCRIPT,
    COR_HTML_TAG_OBJECT,
    COR_HTML_TAG_OL,
    COR_HTML_TAG_OPTGROUP, // 80
    COR_HTML_TAG_OPTION,
    COR_HTML_TAG_OUTPUT,
    COR_HTML_TAG_P,
    COR_HTML_TAG_PARAM,
    COR_HTML_TAG_PRE,
    COR_HTML_TAG_PROGRESS,
    COR_HTML_TAG_Q,
    COR_HTML_TAG_RP,
    COR_HTML_TAG_RT,
    COR_HTML_TAG_RUBY, // 90
    COR_HTML_TAG_S,
    COR_HTML_TAG_SAMP,
    COR_HTML_TAG_SCRIPT,
    COR_HTML_TAG_SECTION,
    COR_HTML_TAG_SELECT,
    COR_HTML_TAG_SMALL,
    COR_HTML_TAG_SOURCE,
    COR_HTML_TAG_SPAN,
    COR_HTML_TAG_STRIKE,
    COR_HTML_TAG_STRONG, // 100
    COR_HTML_TAG_STYLE,
    COR_HTML_TAG_SUB,
    COR_HTML_TAG_SUMMARY,
    COR_HTML_TAG_SUP,
    COR_HTML_TAG_TABLE,
    COR_HTML_TAG_TBODY,
    COR_HTML_TAG_TD,
    COR_HTML_TAG_TEXTAREA,
    COR_HTML_TAG_TFOOT,
    COR_HTML_TAG_TH, // 110
    COR_HTML_TAG_THEAD,
    COR_HTML_TAG_TIME,
    COR_HTML_TAG_TITLE,
    COR_HTML_TAG_TR,
    COR_HTML_TAG_TRACK,
    COR_HTML_TAG_TT,
    COR_HTML_TAG_U,
    COR_HTML_TAG_UL,
    COR_HTML_TAG_VAR,
    COR_HTML_TAG_VIDEO, // 120
    COR_HTML_TAG_WBR,
    /**/
    COR_HTML_TAG_END
};

#define COR_HTML_TAGS_BITMAP_SIZE ((COR_HTML_TAG_END / (8 * sizeof(int))) + 1)

typedef struct cor_html_doc_node_s cor_html_doc_node_t;
struct cor_html_doc_node_s
{
    int tag;
    cor_html_doc_node_t *parent;
    cor_html_doc_node_t *nodes_head;
    cor_html_doc_node_t *nodes_tail;
    cor_html_doc_node_t *nodes_next;
    int content_size;
    const char *content;
};

typedef struct
{
    cor_str_t buf;
    int buf_size;
    cor_html_doc_node_t doc;
    /**/
    int tags_hash_table[1024];
    int void_tags_table[COR_HTML_TAGS_BITMAP_SIZE];
    cor_log_t *log;
    cor_pool_t *pool;
} cor_html_t;

cor_html_t *cor_html_new(cor_log_t *log);
void cor_html_delete(cor_html_t *ctx);
cor_html_doc_node_t *cor_html_parse(cor_html_t *ctx, const char *str, int size);
cor_str_t *cor_html_doc_to_plain(cor_html_t *ctx, cor_html_doc_node_t *node);
void cor_html_dump(cor_html_t *ctx, cor_html_doc_node_t *node, int level);

#endif
