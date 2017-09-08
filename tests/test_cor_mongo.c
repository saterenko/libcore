#include "cor_test.h"
#include "cor_mongo.h"

static volatile int mongo_set_cb_status = 0;
static volatile int mongo_get_cb_status = 0;

void mongo_set_cb(int status, const bson_t *docs, int docs_count, void *arg)
{
    if (status != cor_ok) {
        LOG_FAIL("status != cor_ok");
        return;
    }
    if (docs != NULL) {
        LOG_FAIL("docs != NULL");
        return;
    }
    if (docs_count != 0) {
        LOG_FAIL("docs_count != 0");
        return;
    }
    mongo_set_cb_status = 1;
}

void mongo_get_cb(int status, const bson_t *docs, int docs_count, void *arg)
{
    cor_mongo_t *mongo = (cor_mongo_t *) arg;
    if (status != cor_ok) {
        LOG_FAIL("status != cor_ok");
        ev_break(mongo->loop, EVBREAK_ALL);
        return;
    }
    if (docs == NULL) {
        LOG_FAIL("docs == NULL");
        ev_break(mongo->loop, EVBREAK_ALL);
        return;
    }
    if (docs_count != 1) {
        LOG_FAIL("docs_count != 1");
        ev_break(mongo->loop, EVBREAK_ALL);
        return;
    }
    int is_id = 0;
    int is_value = 0;
    bson_iter_t it;
    bson_iter_init(&it, &docs[0]);
    while (bson_iter_next(&it)) {
        bson_type_t type = bson_iter_type(&it);
        const char *key = bson_iter_key(&it);
        if (type != BSON_TYPE_UTF8) {
            LOG_FAIL("type != BSON_TYPE_UTF8");
            ev_break(mongo->loop, EVBREAK_ALL);
            return;
        }
        if (strncmp(key, "_id", 3) == 0) {
            const char *str = bson_iter_utf8(&it, NULL);
            if (strncmp(str, "1234567890", 10) != 0) {
                LOG_FAIL("value for key _id != \"1234567890\"");
                ev_break(mongo->loop, EVBREAK_ALL);
                return;
            }
            is_id = 1;
        } else if (strncmp(key, "value", 5) == 0) {
            const char *str = bson_iter_utf8(&it, NULL);
            if (strncmp(str, "value text", 10) != 0) {
                LOG_FAIL("value for key _id != \"value text\"");
                ev_break(mongo->loop, EVBREAK_ALL);
                return;
            }
            is_value = 1;
        } else {
            LOG_PFAIL("bad key \"%s\"", key);
            ev_break(mongo->loop, EVBREAK_ALL);
            return;
        }
    }
    if (!is_id) {
        LOG_FAIL("key _id not found");
        ev_break(mongo->loop, EVBREAK_ALL);
        return;
    }
    if (!is_value) {
        LOG_FAIL("key value not found");
        ev_break(mongo->loop, EVBREAK_ALL);
        return;
    }
    mongo_get_cb_status = 1;
    ev_break(mongo->loop, EVBREAK_ALL);
}

BEGIN_TESTA(test_cor_mongo(char *uri))
{
    struct ev_loop *loop = EV_DEFAULT;
    cor_log_t *log = cor_log_new("tests.log", cor_log_level_debug);
    test_ptr_ne(log, NULL);
    cor_mongo_t *mongo = cor_mongo_new(uri, 4, loop, NULL, log);
    test_ptr_ne(mongo, NULL);
    bson_t data;
    bson_init(&data);
    bson_append_utf8(&data, "_id", 3, "1234567890", 10);
    bson_append_utf8(&data, "value", 5, "value text", 10);
    int rc = cor_mongo_set(mongo, "test", "test", &data, mongo_set_cb, mongo);
    test_int_eq(rc, cor_ok);
    bson_destroy(&data);
    usleep(200);
    bson_init(&data);
    bson_append_utf8(&data, "_id", 3, "1234567890", 10);
    rc = cor_mongo_gets(mongo, "test", "test", &data, NULL, 0, 0, mongo_get_cb, mongo);
    test_int_eq(rc, cor_ok);
    bson_destroy(&data);
    usleep(200);
    ev_run(loop, 0);
    test_int_eq(mongo_set_cb_status, 1);
    test_int_eq(mongo_get_cb_status, 1);
    cor_mongo_delete(mongo);
    cor_log_delete(log);
}
END;

int
main(int argc, char **argv)
{
    if (argc == 2) {
        RUN_TESTA(test_cor_mongo(argv[1]));
    }

    exit(0);
}
