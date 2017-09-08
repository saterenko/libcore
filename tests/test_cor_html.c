#include "cor_test.h"
#include "cor_str.h"
#include "../src/cor_html.c"

BEGIN_TEST(test_cor_html_tag_names)
{
    cor_html_t *html = cor_html_new(NULL);
    test_ptr_ne(html, NULL);
    for (int i = 1; i < COR_HTML_TAG_END; ++i) {
        test_int_eq(i, cor_html_get_tag_id(html, cor_html_tag_names[i], strlen(cor_html_tag_names[i])));
    }
    cor_html_delete(html);
}
END;

BEGIN_TEST(test_cor_str_utf8_to_lower)
{
    char src[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZабвгдеёжзийклмнопрстуфхцчшщъыьэюяАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ";
    char dst[] = "0123456789abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzабвгдеежзийклмнопрстуфхцчшщъыьэюяабвгдеежзийклмнопрстуфхцчшщъыьэюя";
    cor_str_utf8_to_lower(src, strlen(src));
    test_str_eq(src, dst);
}
END;

BEGIN_TEST(test_cor_html_parse)
{
    cor_html_t *html = cor_html_new(NULL);
    test_ptr_ne(html, NULL);
    char content[] = "<html><head><title>Title text</title></head><body>  <h1>Header</h1>  after header <p>content</p> after paragraph</body></html>";
    cor_html_doc_node_t *doc = cor_html_parse(html, content, strlen(content));
    test_ptr_ne(doc, NULL);
    cor_html_delete(html);
}
END;

BEGIN_TEST(test_cor_html_doc_to_plain)
{
    cor_html_t *html = cor_html_new(NULL);
    test_ptr_ne(html, NULL);
    char content[] = "<html><head><title>Title text</title></head><body>  <h1>Заголовок страницы  </h1>  after header <p>Какой-то текст параграфа </p> after paragraph</body></html>";
    char expected[] = "title text заголовок страницы after header какой-то текст параграфа after paragraph";
    cor_html_doc_node_t *doc = cor_html_parse(html, content, strlen(content));
    test_ptr_ne(doc, NULL);
    cor_str_t *str = cor_html_doc_to_plain(html, doc);
    test_ptr_ne(str, NULL);
    test_strn_eq(str->data, expected, (int) str->size);
    cor_html_delete(html);
}
END;

int
main(int argc, char **argv)
{
    RUN_TEST(test_cor_html_tag_names);
    RUN_TEST(test_cor_str_utf8_to_lower);
    RUN_TEST(test_cor_html_parse);
    RUN_TEST(test_cor_html_doc_to_plain);

    exit(0);
}