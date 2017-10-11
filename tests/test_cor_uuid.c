#include "cor_test.h"
#include "cor_uuid.h"

BEGIN_TEST(test_cor_uuid)
{
    cor_uuid_seed_t seed;
    int rc = cor_uuid_init(&seed);
    test_int_eq(rc, cor_ok);
    cor_uuid_t uuid;
    cor_uuid_generate(&seed, &uuid);
    char buf[37];
    cor_uuid_unparse(&uuid, buf);
    buf[36] = '\0';
    cor_uuid_t uuid2;
    cor_uuid_parse(buf, &uuid2);
    test_int_eq(uuid.uuid[0], uuid2.uuid[0]);
    test_int_eq(uuid.uuid[1], uuid2.uuid[1]);
}
END;

int
main(int argc, char **argv)
{
    RUN_TEST(test_cor_uuid);

    exit(0);
}
