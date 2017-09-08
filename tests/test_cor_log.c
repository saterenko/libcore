#include <fcntl.h>
#include <sys/stat.h>

#include "cor_test.h"
#include "cor_log.h"

char *
test_cor_log_get_content(const char *file, size_t *file_size)
{
    int fd = open(file, O_RDONLY);
    if (fd < 0) {
        return NULL;
    }
    struct stat sb;
    if (fstat(fd, &sb) < 0) {
        close(fd);
        return NULL;
    }
    if (!sb.st_size) {
        close(fd);
        return NULL;
    }
    size_t size = sb.st_size;
    if (file_size) {
        *file_size = sb.st_size;
    }
    char *buf = (char *) malloc(size);
    if (!buf) {
        close(fd);
        return NULL;
    }
    char *p = buf;
    while (1) {
        ssize_t rc = read(fd, (void *) p, size);
        if (rc == size) {
            break;
        }
        if (rc < 0) {
            free(buf);
            close(fd);
            return NULL;
        }
        p += rc;
        size -= rc;
    }
    close(fd);
    return buf;
}

BEGIN_TEST(test_cor_log_levels)
{
    cor_log_t *log = cor_log_new("test.log", cor_log_level_warn);
    test_ptr_ne(log, NULL);
    cor_log_error(log, "test error");
    cor_log_warn(log, "test warn");
    cor_log_info(log, "test info");
    cor_log_debug(log, "test debug");
    cor_log_delete(log);
    char *buf = test_cor_log_get_content("test.log", NULL);
    test_ptr_ne(buf, NULL);
    test_ptr_ne(strstr(buf, "test error\n"), NULL);
    test_ptr_ne(strstr(buf, "test warn\n"), NULL);
    test_ptr_eq(strstr(buf, "test info\n"), NULL);
    test_ptr_eq(strstr(buf, "test debug\n"), NULL);
    free(buf);
    unlink("test.log");
    /**/
    log = cor_log_new("test.log", cor_log_level_debug);
    test_ptr_ne(log, NULL);
    cor_log_error(log, "test error");
    cor_log_warn(log, "test warn");
    cor_log_info(log, "test info");
    cor_log_debug(log, "test debug");
    cor_log_delete(log);
    buf = test_cor_log_get_content("test.log", NULL);
    test_ptr_ne(buf, NULL);
    test_ptr_ne(strstr(buf, "test error\n"), NULL);
    test_ptr_ne(strstr(buf, "test warn\n"), NULL);
    test_ptr_ne(strstr(buf, "test info\n"), NULL);
    test_ptr_ne(strstr(buf, "test debug\n"), NULL);
    free(buf);
    unlink("test.log");
}
END;

BEGIN_TEST(test_cor_log_long_line)
{
    cor_log_t *log = cor_log_new("test.log", cor_log_level_debug);
    test_ptr_ne(log, NULL);
    char msg[3000];
    static char *chars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    size_t chars_len = strlen(chars);
    for (int i = 0; i < 2999; i++) {
        msg[i] = chars[rand() % chars_len];
    }
    msg[2999] = '\0';
    cor_log_info(log, msg);
    cor_log_delete(log);
    char *buf = test_cor_log_get_content("test.log", NULL);
    test_ptr_ne(buf, NULL);
    test_ptr_ne(strstr(buf, msg), NULL);
    free(buf);
    unlink("test.log");
}
END;

BEGIN_TEST(test_cor_log_max_size)
{
    cor_log_t *log = cor_log_new("test.log", cor_log_level_debug);
    test_ptr_ne(log, NULL);
    log->max_line_size = 2000;
    char msg[3000];
    static char *chars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    size_t chars_len = strlen(chars);
    for (int i = 0; i < 2999; i++) {
        msg[i] = chars[rand() % chars_len];
    }
    msg[2999] = '\0';
    cor_log_info(log, msg);
    cor_log_delete(log);
    size_t size;
    char *buf = test_cor_log_get_content("test.log", &size);
    test_ptr_ne(buf, NULL);
    test_int_eq(size, 2000);
    free(buf);
    unlink("test.log");
}
END;

int
main(int argc, char **argv)
{
    RUN_TEST(test_cor_log_levels);
    RUN_TEST(test_cor_log_long_line);
    RUN_TEST(test_cor_log_max_size);

    exit(0);
}
