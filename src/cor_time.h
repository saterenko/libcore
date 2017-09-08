#ifndef COR_TIME_H
#define COR_TIME_H

#include <sys/time.h>
#include "cor_core.h"

double cor_time_diff(struct timeval *x, struct timeval *y);

#endif
