#ifndef __PARALLEL_ARRAY_H
#define __PARALLEL_ARRAY_H

#include <omp.h>
#include <utility>

#include "defs.h"

template<typename T>
struct ParallelArray {
    const u32 NUM_THREADS;

    u32 arr_size;
    T* data;

    ParallelArray(u32 arr_size, u32 NUM_THREADS = omp_get_max_threads()) : NUM_THREADS(NUM_THREADS),
                                                                           arr_size(arr_size) {
        data = static_cast<T*>(operator new[] (arr_size * sizeof(T)));
    }

    ParallelArray(ParallelArray<T>& other) : NUM_THREADS(other.NUM_THREADS),
                                                arr_size(other.arr_size) {
        data = static_cast<T*>(operator new[] (arr_size * sizeof(T)));

        #pragma omp parallel for num_threads(NUM_THREADS)
        for (u32 i = 0; i < arr_size; ++i) {
            data[i] = other.data[i];
        }
    }

    ParallelArray(ParallelArray<T>&& other) : NUM_THREADS(other.NUM_THREADS) {
        std::swap(arr_size, other.arr_size);
        std::swap(data, other.data);
    }

    ParallelArray<T>& operator=(const ParallelArray<T>& other) {
        delete[] data;
        arr_size = other.arr_size;
        data = static_cast<T*>(operator new[] (arr_size * sizeof(T)));

        #pragma omp parallel for num_threads(NUM_THREADS)
        for (u32 i = 0; i < arr_size; ++i) {
            data[i] = other.data[i];
        }

        return *this;
    }

    u32 size() const {
        return arr_size;
    }

    const T& operator[](u32 id) const {
        if (id >= arr_size) {
            throw std::out_of_range("Parallel array id out of range");
        }
        return data[id];
    }

    T& operator[](u32 id) {
        if (id >= arr_size) {
            throw std::out_of_range("Parallel array id out of range");
        }
        return data[id];
    }

    void swap(ParallelArray<T> other) {
        if (this == &other) {
            throw std::invalid_argument("Swapping with the same ParallelArray");
        }
        std::swap(arr_size, other.arr_size);
        std::swap(data, other.data);
    }

    const T* begin() const {
        return data;
    }

    T* begin() {
        return data;
    }

    const T* end() const {
        return data + arr_size;
    }

    T* end() {
        return data + arr_size;
    }

    ~ParallelArray() {
        delete[] data;
    }
};

#endif
