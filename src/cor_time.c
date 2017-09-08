#include "cor_time.h"

double
cor_time_diff(struct timeval *x, struct timeval *y)
{
    struct timeval  diff;

    diff.tv_sec = y->tv_sec - x->tv_sec;
    diff.tv_usec = y->tv_usec - x->tv_usec;
    while (diff.tv_usec < 0) {
        diff.tv_usec += 1000000;
        diff.tv_sec -= 1;
    }
    return (double) diff.tv_sec + (double) diff.tv_usec / 1000000.0;
}
