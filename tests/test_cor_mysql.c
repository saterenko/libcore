#include "cor_test.h"
#include "../src/cor_mysql.c"

BEGIN_TESTA(test_cor_mysql_query(const char *host, int port, const char *db, const char *usr, const char *pwd))
{
    /*  init  */
    cor_mysql_t *m = cor_mysql_new(host, port, db, usr, pwd);
    test_ptr_ne(m, NULL);
    if (host) {
        test_str_eq(m->host, host);
    }
    test_int_eq(m->port, port);
    if (db) {
        test_str_eq(m->db, db);
    }
    if (usr) {
        test_str_eq(m->usr, usr);
    }
    if (pwd) {
        test_str_eq(m->pwd, pwd);
    }
    /*  create test table  */
    int rc = cor_mysql_update(m, "drop table if exists test_cor_mysql_query");
    test_int_eq(rc, 0);
    rc = cor_mysql_update(m, "create table test_cor_mysql_query(id int, name varchar(256))");
    test_int_eq(rc, 0);
    rc = cor_mysql_update(m, "insert into test_cor_mysql_query values(1, 'name1'), (2, 'name2')");
    test_int_eq(rc, 0);
    /*  select  */
    m->query_size = 16;
    MYSQL_RES *r = cor_mysql_query(m, "select * from test_cor_mysql_query where id=%d", 1);
    test_ptr_ne(r, NULL);
    test_int_eq(m->query_size, 54);
    test_str_eq(m->query, "select * from test_cor_mysql_query where id=1");
    char **row = cor_mysql_row(r);
    test_ptr_ne(row, NULL);
    test_ptr_ne(row[0], NULL);
    test_ptr_ne(row[1], NULL);
    test_str_eq(row[0], "1");
    test_str_eq(row[1], "name1");
    row = cor_mysql_row(r);
    test_ptr_eq(row, NULL);
    /*  select 2  */
    r = cor_mysql_query(m, "select * from test_cor_mysql_query where name in ('%s', '%s')", "name1", "name2");
    test_ptr_ne(r, NULL);
    test_str_eq(m->query, "select * from test_cor_mysql_query where name in ('name1', 'name2')");
    row = cor_mysql_row(r);
    test_ptr_ne(row, NULL);
    test_ptr_ne(row[0], NULL);
    test_ptr_ne(row[1], NULL);
    test_str_eq(row[0], "1");
    test_str_eq(row[1], "name1");
    row = cor_mysql_row(r);
    test_ptr_ne(row, NULL);
    test_ptr_ne(row[0], NULL);
    test_ptr_ne(row[1], NULL);
    test_str_eq(row[0], "2");
    test_str_eq(row[1], "name2");
    row = cor_mysql_row(r);
    test_ptr_eq(row, NULL);
    cor_mysql_delete(m);
}
END;

int
main(int argc, char **argv)
{
    if (argc == 5) {
        RUN_TESTA(test_cor_mysql_query(argv[1], atoi(argv[2]), argv[3], argv[4], argv[5]));
    }

    exit(0);
}
