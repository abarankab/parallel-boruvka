#ifndef __SEQUENTIAL_DSU_H
#define __SEQUENTIAL_DSU_H

#include "defs.h"

struct SequentialDSU {
    u32 size;
    u32* parent;
    u32* rank;

    SequentialDSU(u32 size) : size(size) {
        rank = new u32[size];
        parent = new u32[size];
        for (u32 i = 0; i < size; ++i) parent[i] = i;
    }

    ~SequentialDSU() {
        delete[] parent;
        delete[] rank;
    }

    u32 find_root(u32 id) {
        while (id != parent[id]) {
            id = parent[id];
        }

        return id;
    }

    bool same_set(u32 id1, u32 id2) {
        return find_root(id1) == find_root(id2);
    }

    void unite(u32 id1, u32 id2) {
        id1 = find_root(id1);
        id2 = find_root(id2);

        if (rank[id1] < rank[id2]) std::swap(id1, id2);

        parent[id2] = id1;
        if (rank[id1] == rank[id2]) ++rank[id1];
    }
};

#endif
