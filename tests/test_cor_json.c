#include "cor_test.h"
#include "cor_str.h"
#include "../src/cor_json.c"

BEGIN_TEST(test_cor_json_parse)
{
    // const char *str = "{"
    //     "\"id\": \"4eb9c03e-8a9d-4978-89ff-d85120330782-9229-7b871\","
    //     "\"imp\": [{"
    //         "\"id\": \"1\","
    //         "\"video\": {"
    //             "\"mimes\": [\"video/mp4\"],"
    //             "\"w\": 1920,"
    //             "\"h\": 1080,"
    //             "\"ext\": {"
    //                 "\"rewarded\": 1"
    //             "}"
    //         "}"
    //     "}],"
    //     "\"app\": {"
    //         "\"id\": \"123\","
    //         "\"name\": \"Pull the Pin\","
    //         "\"bundle\": \"com.maroieqrwlk.unpin\""
    //     "},"
    //     "\"device\": {"
    //         "\"ua\": \"Mozilla/5.0 (Linux; Android 10; SM-S102DL Build/QP1A.190711.020; wv) AppleWebKit/537.36 (KHTML, like Gecko) Version/4.0 Chrome/93.0.4577.62 Mobile Safari/537.36\","
    //         "\"geo\": {"
    //             "\"lat\": 44.0051,"
    //             "\"lon\": -103.3663,"
    //             "\"country\": \"RU\","
    //             "\"region\": \"MO\","
    //             "\"city\": \"Moscow\""
    //         "},"
    //         "\"ip\": \"209.159.234.89\","
    //         "\"devicetype\": 4,"
    //         "\"make\": \"samsung\","
    //         "\"model\": \"SM-S102DL\","
    //         "\"os\": \"android\","
    //         "\"osv\": \"10.0\","
    //         "\"w\": 1080,"
    //         "\"h\": 1920,"
    //         "\"carrier\": \"Mobile\","
    //         "\"connectiontype\": 2,"
    //         "\"ifa\": \"83362a46-a2b3-424f-9705-0dc3bd0abb83\""
    //     "},"
    //     "\"user\": {"
    //         "\"id\": \"0be085d7-b22c-4464-99c5-719d076f96e5\""
    //     "}"
    // "}";
    const char *str = "{"
        "\"id\": \"4eb9c03e-8a9d-4978-89ff-d85120330782-9229-7b871\","
        "\"app\": {"
            "\"id\": \"123\","
            "\"name\": \"Pull the Pin\","
            "\"bundle\": \"com.maroieqrwlk.unpin\""
        "},"
        "\"device\": {"
            "\"ua\": \"Mozilla/5.0 (Linux; Android 10; SM-S102DL Build/QP1A.190711.020; wv) AppleWebKit/537.36 (KHTML, like Gecko) Version/4.0 Chrome/93.0.4577.62 Mobile Safari/537.36\","
            "\"geo\": {"
                "\"lat\": 44.0051,"
                "\"lon\": -103.3663,"
                "\"country\": \"RU\","
                "\"region\": \"MO\","
                "\"city\": \"Moscow\""
            "},"
            "\"ip\": \"209.159.234.89\","
            "\"devicetype\": 4,"
            "\"make\": \"samsung\","
            "\"model\": \"SM-S102DL\","
            "\"os\": \"android\","
            "\"osv\": \"10.0\","
            "\"w\": 1080,"
            "\"h\": 1920,"
            "\"carrier\": \"Mobile\","
            "\"connectiontype\": 2,"
            "\"ifa\": \"83362a46-a2b3-424f-9705-0dc3bd0abb83\""
        "},"
        "\"user\": {"
            "\"id\": \"0be085d7-b22c-4464-99c5-719d076f96e5\""
        "}"
    "}";
    /**/
    cor_json_t *json = cor_json_new();
    test_ptr_ne(json, NULL);
    int rc = cor_json_parse(json, str, strlen(str));
    LOG_PINFO("error: %s\n", json->error);
    test_int_eq(rc, cor_ok);
    test_int_eq(json->root.type, COR_JSON_NODE_OBJECT);
    cor_json_node_t *n = json->root.first_child;
    test_ptr_ne(n, NULL);
    test_int_eq(n->type, COR_JSON_NODE_STRING);
    test_strn_eq(n->name.data, "id", 2);
    LOG_PINFO("%.*s\n", (int) n->value.size, n->value.data);
    test_strn_eq(n->value.data, "4eb9c03e-8a9d-4978-89ff-d85120330782-9229-7b871", 47);
    cor_str_t *s = cor_json_stringify(json);
    test_ptr_ne(s, NULL);
    printf("%.*s\n", (int) s->size, s->data);
}
END;

int
main(int argc, char **argv)
{
    RUN_TEST(test_cor_json_parse);

    exit(0);
}