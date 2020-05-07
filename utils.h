#ifndef __UTILS_H
#define __UTILS_H

#include <random>

#include "defs.h"

std::mt19937 gen(std::random_device{}());

u32 randint(u32 l, u32 r) {
    return std::uniform_int_distribution<u32>(l, r)(gen);
}

#endif
