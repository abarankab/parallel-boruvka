#ifndef __DSU_H
#define __DSU_H

#include <atomic>
#include <omp.h>
#include <stdexcept>

#include "defs.h"
#include "parallel_array.h"

/**
 * INTERFACE:
 * 
 * ParallelDSU(uint32_t N, uint32_t NUM_THREADS) - constructs a DSU of size N using NUM_THREADS
 * uint32_t find_root(uint32_t id) - finds root node of id
 * bool same_set(uint32_t id1, uint32_t id2) - checks if id1 and id2 are in the same set
 * void unite(uint32_t id1, uint32_t id2) - unites sets of id1 and id2
 * 
 * DETAILS:
 * 
 * Implementation was inspired by this repo
 * https://github.com/wjakob/dset/blob/master/dset.h
 * and this paper
 * http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.56.8354&rep=rep1&type=pdf
 * It uses both rank and path heuristics in parallel
 * which should allow for O(\alpha S) time on both find_root and unite operations
 * 
 * Data is stored in unsigned 64 bit integers
 * The first 32 bits encode node parent
 * The last 32 bits encode node rank
 * This allows for easier compare and swap and should work slightly faster
 * 
 * E.g. if X is the stored value:
 * X & 0x00000000FFFFFFFF <- parent
 * X & 0xFFFFFFFF00000000 <- rank
 * 
 * To decode these values from u64 one should use get_parent() and get_rank()
 * 
 * I also check if id is within range and throw an exception otherwise,
 * this slows the code down a little bit but should save you some time debugging
 */
struct ParallelDSU {
    const u32 NUM_THREADS;
    
    u32 dsu_size;
    atomic_u64* data;

    const u32 BINARY_BUCKET_SIZE = 32;
    const u64 RANK_MASK = 0xFFFFFFFF00000000ULL;

    ParallelDSU(u32 size, u32 NUM_THREADS = omp_get_max_threads()) : NUM_THREADS(NUM_THREADS), dsu_size(size) {
        if (size == 0) {
            throw std::invalid_argument("DSU size cannot be zero");
        }

        data = static_cast<atomic_u64*>(operator new[] (size * sizeof(atomic_u64)));;

        #pragma omp parallel for shared(data) num_threads(NUM_THREADS)
        for (u32 i = 0; i < size; ++i) data[i] = i;
    }

    u32 size() const {
        return dsu_size;
    }

    void check_out_of_range(u32 id) const {
        if (id >= size()) {
            throw std::out_of_range("Node id out of range");
        }
    }

    u64 encode_node(u32 parent, u32 rank) {
        return (static_cast<u64>(rank) << BINARY_BUCKET_SIZE) | parent;
    }

    u32 get_parent(u32 id) const {
        return static_cast<u32>(data[id]);
    }

    u32 get_rank(u32 id) const {
        return static_cast<u32>(data[id] >> BINARY_BUCKET_SIZE);
    }

    /**
     * On each step we try to apply path heuristic using CAS
     * and then move closer to the root and
     * 
     * The loop breaks when a node's parent is equal to itself
     * E.g. when we find the root
     */
    u32 find_root(u32 id) {
        check_out_of_range(id);

        while (id != get_parent(id)) {
            u64 value = data[id];
            u32 grandparent = get_parent(static_cast<u32>(value));
            u64 new_value = (value & RANK_MASK) | grandparent;

            /* Path heuristic */
            if (value != new_value) {
                data[id].compare_exchange_strong(value, new_value);
            }

            id = grandparent;
        }

        return id;
    }

    /**
     * We try to check if two nodes are in the same set
     * by checking if their roots are the same
     * 
     * Since it is a parallel structure, node roots may change during runtime
     * In order to account for this we do a while loop and repeat if
     * our current node is no longer the root of its set
     * 
     * In general, you should call this after synchronization,
     * It still works during parallel segments, but the results will make no sense
     */
    bool same_set(u32 id1, u32 id2) {
        check_out_of_range(id1);
        check_out_of_range(id2);

        while (true) {
            id1 = find_root(id1);
            id2 = find_root(id2);

            if (id1 == id2) {
                return true;
            } else if (get_parent(id1) == id1) {
                return false;
            }
        }
    }

    /**
     * We try to hang the smaller component onto the bigger one
     * 
     * Since it is a parallel structure, node roots may change during runtime
     * In order to account for this we do a while loop and repeat if
     * the smaller node was updated e.g. when CAS failed
     */
    void unite(u32 id1, u32 id2) {
        check_out_of_range(id1);
        check_out_of_range(id2);

        while (true) {
            id1 = find_root(id1);
            id2 = find_root(id2);

            /* Nodes are already in the same set */
            if (id1 == id2) return;

            u32 rank1 = get_rank(id1);
            u32 rank2 = get_rank(id2);

            /* Hanging the smaller set to the bigger one, rank heuristic */
            if (rank1 < rank2 || (rank1 == rank2 && id1 > id2)) {
                std::swap(rank1, rank2);
                std::swap(id1, id2);
            }

            u64 old_value = encode_node(id2, rank2);
            u64 new_value = encode_node(id1, rank2);

            /* If CAS fails we need to repeat the same step once again */
            if (!data[id2].compare_exchange_strong(old_value, new_value)) {
                continue;
            }

            /* Updating rank */
            if (rank1 == rank2) {
                old_value = encode_node(id1, rank1);
                new_value = encode_node(id1, rank1 + 1);

                data[id1].compare_exchange_strong(old_value, new_value);
            }

            break;
        }
    }
};

#endif
