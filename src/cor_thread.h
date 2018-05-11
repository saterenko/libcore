#ifndef COR_THREAD_H
#define COR_THREAD_H

#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif
#include <pthread.h>
#include <sched.h>
#include <sys/types.h>

#include <ev.h>

#include "cor_core.h"

typedef struct cor_thread_s cor_thread_t;
typedef void (cor_thread_cb_t) (cor_thread_t *t, void *arg);

struct cor_thread_s
{
    pid_t tid;
    cor_thread_cb_t *on_init;
    cor_thread_cb_t *on_shutdown;
    void *arg;
    pthread_t thread;
    ev_async ev_shutdown;
    struct ev_loop *loop;
    unsigned shutdown:1;
};

cor_thread_t *cor_thread_new(cor_thread_cb_t *on_init, cor_thread_cb_t *on_shutdown, void *arg);
void cor_thread_run(cor_thread_t *t);
void cor_thread_delete(cor_thread_t *t);
int cor_thread_set_affinity(cor_thread_t *t, int n);

static inline pid_t
cor_thread_tid(cor_thread_t *t)
{
    return t->tid;
}

static inline struct ev_loop *
cor_thread_loop(cor_thread_t *t)
{
    return t->loop;
}

#endif
