#include "cor_trace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

cor_trace_t *
cor_trace_new(size_t size)
{
    cor_trace_t *t = (cor_trace_t *) malloc(sizeof(cor_trace_t));
    if (!t) {
        return NULL;
    }
    memset(t, 0, sizeof(cor_trace_t));
    t->alloc_size = size ? size : 256;
    t->trace_time = 1;
    t->trace_file = 1;
    t->trace = (char *) malloc(t->alloc_size);
    if (!t->trace) {
        free(t);
        return NULL;
    }
    t->trace[0] = '\0';
    return t;
}

void
cor_trace_delete(cor_trace_t *t)
{
    if (t) {
        if (t->trace) {
            free(t->trace);
        }
        free(t);
    }
}

void
cor_trace_impl(cor_trace_t *t, const char *file, int line, const char *format, ...)
{
    char *p = t->trace;
    char *end = p + t->alloc_size;
    /*  write time if need  */
    if (t->trace_time) {
        /*  cache time if changed  */
        struct timeval tmv;
        gettimeofday(&tmv, NULL);
        if (tmv.tv_sec != t->ts) {
            struct tm tm;
            localtime_r(&tmv.tv_sec, &tm);
            snprintf(t->ts_str, COR_TRACE_TIME_STR_SIZE + 1, "[%04d-%02d-%02d %02d:%02d:%02d.", 
                (tm.tm_year + 1900), (tm.tm_mon + 1), tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
            t->ts = tmv.tv_sec;
        }
        /* write  */
        int rc = snprintf(p, end - p, "%s%lld] ", t->ts_str, (long long) tmv.tv_usec);
        if (rc < 0 || rc > (end - p)) {
            return;
        }
        p += rc;
    }
    /*  write file if need  */
    if (t->trace_file) {
        const char *k = strrchr(file, '/');
        k = k ? k + 1 : file;
        int rc = snprintf(p, end - p, "%s:%d ", k, line);
        if (rc < 0 || rc > (end - p)) {
            return;
        }
        p += rc;
    }
    /*  write message  */
    va_list args;
    while (1) {
        va_start(args, format);
        int rc = vsnprintf(p, end - p, format, args);
        va_end(args);
        if (rc < 0) {
            return;
        }
        if (rc >= end - p) {
            size_t need = t->alloc_size + (rc - (end - p)) + 1;
            size_t size = t->alloc_size;
            while (size < need) {
                size = (size << 1) - (size >> 1);
            }
            /*  realloc  */
            char *nb = (char *) malloc(size);
            if (!nb) {
                return;
            }
            t->alloc_size = size;
            memcpy(nb, t->trace, p - t->trace);
            p = nb + (p - t->trace);
            end = nb + size;
            free(t->trace);
            t->trace = nb;
            continue;
        }
        p += rc;
        break;
    }
    *p = '\0';
    t->size += p - t->trace;
}

const char *
cor_trace_get(cor_trace_t *t)
{
    return (const char *) t->trace;
}
