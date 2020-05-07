#ifndef __TIMER_H
#define __TIMER_H

#include <sys/time.h>
#include <chrono>

#include "defs.h"

u64 currentSeconds() {
    timespec spec;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &spec);
    return spec.tv_sec * 1e9 + spec.tv_nsec;
}

#endif
