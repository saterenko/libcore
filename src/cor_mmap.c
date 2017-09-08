#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "cor_mmap.h"

cor_mmap_t *
cor_mmap_open(const char *file, cor_log_t *log)
{
    cor_mmap_t *mp = (cor_mmap_t *) malloc(sizeof(cor_mmap_t));
    if (!mp) {
        cor_log_error(log, "can't malloc");
        return NULL;
    }
    memset(mp, 0, sizeof(cor_mmap_t));
    /**/
    int fd = open(file, O_RDONLY);
    if (fd < 0) {
        cor_log_error(log, "can't open %s", file);
        free(mp);
        return NULL;
    }
    struct stat sb;
    if (fstat(fd, &sb) < 0) {
        cor_log_error(log, "can't stat %s", file);
        close(fd);
        free(mp);
        return NULL;
    }
    if (!sb.st_size) {
        cor_log_error(log, "file %s is empty", file);
        close(fd);
        free(mp);
        return NULL;
    }
    mp->src = (const char *) mmap(0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (mp->src == MAP_FAILED) {
        cor_log_error(log, "can't mmap %s", file);
        close(fd);
        free(mp);
        return NULL;
    }
    mp->size = sb.st_size;
    close(fd);

    return mp;
}

void
cor_mmap_close(cor_mmap_t *mmap)
{
    if (mmap) {
        munmap((void *) mmap->src, mmap->size);
    }
}
