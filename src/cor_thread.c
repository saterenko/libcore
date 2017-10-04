#include "cor_thread.h"

#include <stdlib.h>
#include <stdio.h>
#ifndef gettid
    #include <sys/syscall.h>
    #include <unistd.h>
    #define gettid() syscall(SYS_gettid)
#endif


static void *cor_thread_main(void *arg);
static void cor_thread_on_shutdown(struct ev_loop *loop, ev_async *w, int revents);

cor_thread_t *
cor_thread_new(cor_thread_cb_t *on_init, cor_thread_cb_t *on_shutdown, void *arg)
{
    cor_thread_t *t = (cor_thread_t *) malloc(sizeof(cor_thread_t));
    if (!t) {
        return NULL;
    }
    memset(t, 0, sizeof(cor_thread_t));
    /**/
    t->loop = ev_loop_new(EVFLAG_AUTO);
    t->on_init = on_init;
    t->on_shutdown = on_shutdown;
    t->arg = arg;
    /*  init shutdown handler  */
    ev_async_init(&t->ev_shutdown, cor_thread_on_shutdown);
    t->ev_shutdown.data = t;
    ev_async_start(t->loop, &t->ev_shutdown);
    /*  start thread  */
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&t->thread, &attr, cor_thread_main, t);
    pthread_attr_destroy(&attr);
    return t;
}

void
cor_thread_delete(cor_thread_t *t)
{
    if (t) {
        if (!t->shutdown) {
            t->shutdown = 1;
            if (t->tid == gettid()) {
                /*  this is a spawned thread  */
                ev_break(t->loop, EVBREAK_ALL);
            } else {
                /*  this is a parent thread  */
                ev_async_send(t->loop, &t->ev_shutdown);
                pthread_join(t->thread, NULL);
            }
        }
        free(t);
    }
}

static void *
cor_thread_main(void *arg)
{
    cor_thread_t *t = (cor_thread_t *) arg;
    t->tid = gettid();
    t->on_init(t, arg);
    ev_run(t->loop, 0);
    ev_loop_destroy(t->loop);
    return NULL;
}

static void
cor_thread_on_shutdown(struct ev_loop *loop, ev_async *w, int revents)
{
    cor_thread_t *t = (cor_thread_t *) w->data;
    t->on_shutdown(t, t->arg);
    ev_break(t->loop, EVBREAK_ALL);
}

