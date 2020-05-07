#!/bin/bash

g++ tests/$1.cc -Wall -Wextra -Werror -fopenmp -std=c++17 -mtune=native -mavx2 -Ofast -o exec/$1.o

