#ifndef COR_TRACE_H
#define COR_TRACE_H

#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#define COR_TRACE_TIME_STR_SIZE sizeof("[YYYY-MM-DD HH:MM:SS.") - 1

typedef struct
{
    size_t size;
    size_t alloc_size;
    char *trace;
    time_t ts;
    char ts_str[COR_TRACE_TIME_STR_SIZE + 1];
    unsigned trace_time:1;
    unsigned trace_file:1;
} cor_trace_t;

#define cor_trace(msg, args...) cor_trace_impl(msg, __FILE__, __LINE__, args)

cor_trace_t *cor_trace_new(size_t size);
void cor_trace_delete(cor_trace_t *t);
void cor_trace_impl(cor_trace_t *t, const char *file, int line, const char *format, ...);
const char *cor_trace_get(cor_trace_t *t);

#endif
