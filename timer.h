#ifndef __TIMER_H
#define __TIMER_H

#include <sys/time.h>
#include <chrono>

#include "defs.h"

u64 currentSeconds() {
    // timespec spec;
    // clock_gettime(CLOCK_THREAD_CPUTIME_ID, &spec);
    // return spec.tv_sec * 1e9 + spec.tv_nsec;

    auto now = std::chrono::steady_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::nanoseconds>(now);
    auto epoch = now_ms.time_since_epoch();
    auto value = std::chrono::duration_cast<std::chrono::nanoseconds>(epoch);
    return value.count();
}

#endif
