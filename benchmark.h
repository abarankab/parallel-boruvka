#ifndef __BENCHMARK_H
#define __BENCHMARK_H

void escape(void* p) {
    asm volatile("" : : "g"(p) : "memory");
}

#endif
