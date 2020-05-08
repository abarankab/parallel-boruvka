#ifndef __BORUVKA_H
#define __BORUVKA_H

#include <algorithm>
#include <limits>
#include <omp.h>
#include <parallel/algorithm>
#include <parallel/numeric>
#include <unordered_map>

#include "parallel_dsu.h"
#include "graph.h"
#include "parallel_array.h"

struct ParallelBoruvkaMST {
    /**
     * I use the same atomic pair that I use in dsu.h
     */
    const u32 EDGE_BINARY_BUCKET_SIZE = 32;
    const u64 EDGE_WEIGHT_MASK = 0xFFFFFFFF00000000ULL;

    u64 encode_edge(u32 id, u32 weight) {
        return (static_cast<u64>(weight) << EDGE_BINARY_BUCKET_SIZE) | id;
    }

    u32 get_id(u64 encoded_edge) {
        return static_cast<u32>(encoded_edge);
    }

    u32 get_weight(u64 encoded_edge) {
        return static_cast<u32>(encoded_edge >> EDGE_BINARY_BUCKET_SIZE);
    }

    /**
     * Calculates MST of given graph and returns a ParallelArray<Edge> object
     * Graph edges must be sorted beforehand
     */
    ParallelArray<Edge> calculate_mst(Graph graph, u32 NUM_THREADS = omp_get_max_threads()) {
        ParallelDSU node_sets(graph.num_nodes(), NUM_THREADS);
        ParallelArray<Edge> mst(graph.num_nodes() - 1, NUM_THREADS);
        u32 current_mst_size = 0;
        u32 initial_num_nodes = graph.num_nodes();

        while (graph.num_nodes() != 1) {
            ParallelArray<atomic_u64> shortest_edges(initial_num_nodes, NUM_THREADS);

            /* Calculating shortest edges from each node */
            #pragma omp parallel num_threads(NUM_THREADS)
            {
                ParallelArray<std::pair<u32, u32>> local_shortest_edges(initial_num_nodes);
                ParallelArray<u32> local_nodes(graph.num_nodes());
                u32 local_size = 0;
                u32 last_node = initial_num_nodes + 1;  /* Assuming there is no node bigger than N in G */

                for (u32 i = 0; i < graph.num_nodes(); ++i) {
                    shortest_edges[graph.nodes[i]] = encode_edge(0, std::numeric_limits<u32>::max());
                }

                #pragma omp for
                for (u32 i = 0; i < graph.num_edges(); ++i) {
                    const Edge& e = graph.edges[i];

                    if (e.from != last_node || local_shortest_edges[e.from].first > e.weight) {
                        local_shortest_edges[e.from] = { e.weight, i };
                        if (e.from != last_node) {
                            local_nodes[local_size++] = e.from;
                            last_node = e.from;
                        }
                    }
                }

                for (u32 i = 0; i < local_size; ++i) { /* O(M / p) operations in each thread */
                    u32 node = local_nodes[i];
                    u64 old = shortest_edges[node];
                    auto shortest_edge = local_shortest_edges[node];

                    /* p.second = { weight, id } */
                    u64 encoded_edge = encode_edge(shortest_edge.second, shortest_edge.first);

                    while (true) { /* This loop is wait-free */
                        if (get_weight(old) < shortest_edge.first ||
                            shortest_edges[node].compare_exchange_strong(old, encoded_edge)) {
                            break;
                        }
                    }
                }
            }

            /* Calculating selected edges */
            ParallelArray<u32> edge_selected(graph.num_edges(), NUM_THREADS);

            #pragma omp parallel for num_threads(NUM_THREADS)
            for (u32 i = 0; i < graph.num_edges(); ++i) {
                edge_selected[i] = 0;
            }

            #pragma omp parallel for num_threads(NUM_THREADS)
            for (u32 i = 0; i < graph.num_nodes(); ++i) {
                u32 u = graph.nodes[i];
                u32 v = graph.edges[get_id(shortest_edges[u])].to;
                
                /* If smallest edge from v goes to u or u < v */
                if (graph.edges[get_id(shortest_edges[v])].to != u || u < v) { 
                    node_sets.unite(u, v);
                    edge_selected[get_id(shortest_edges[u])] = true;
                }
            }

            /* Adding edges to MST */
            ParallelArray<u32> edge_selected_prefix(graph.num_edges(), NUM_THREADS);
            __gnu_parallel::partial_sum(edge_selected.begin(), edge_selected.end(), edge_selected_prefix.begin());

            #pragma omp parallel for num_threads(NUM_THREADS)
            for (u32 i = 0; i < graph.num_edges(); ++i) {
                if (edge_selected[i]) {
                    mst[current_mst_size + edge_selected_prefix[i] - 1] = graph.edges[i];
                }
            }
            current_mst_size += edge_selected_prefix[graph.num_edges() - 1];

            /* Calculating remaining edges */
            ParallelArray<u32> edge_remains(graph.num_edges(), NUM_THREADS);
            #pragma omp parallel for
            for (u32 i = 0; i < graph.num_edges(); ++i) {
                edge_remains[i] = !node_sets.same_set(graph.edges[i].from, graph.edges[i].to);
            }

            ParallelArray<u32> edge_remains_prefix(graph.num_edges(), NUM_THREADS);
            __gnu_parallel::partial_sum(edge_remains.begin(), edge_remains.end(), edge_remains_prefix.begin());
            ParallelArray<Edge> new_edges(edge_remains_prefix[graph.num_edges() - 1], NUM_THREADS);
                
            #pragma omp parallel for num_threads(NUM_THREADS)
            for (u32 i = 0; i < graph.num_edges(); ++i) {
                if (edge_remains[i]) {
                    const Edge& old_edge = graph.edges[i];
                    new_edges[edge_remains_prefix[i] - 1] = Edge(node_sets.find_root(old_edge.from),
                                                                    node_sets.find_root(old_edge.to),
                                                                    old_edge.weight);
                }
            }
                
            /* Calculating remaining nodes */
            ParallelArray<u32> node_remains(graph.num_nodes(), NUM_THREADS);
            #pragma omp parallel for num_threads(NUM_THREADS)
            for (u32 i = 0; i < graph.num_nodes(); ++i) {
                node_remains[i] = (
                    node_sets.find_root(graph.nodes[i]) == graph.nodes[i]
                );
            }

            ParallelArray<u32> node_remains_prefix(graph.num_nodes(), NUM_THREADS);
            __gnu_parallel::partial_sum(node_remains.begin(), node_remains.end(), node_remains_prefix.begin());
            ParallelArray<u32> new_nodes(node_remains_prefix[graph.num_nodes() - 1]);

            #pragma omp parallel for num_threads(NUM_THREADS)
            for (u32 i = 0; i < graph.num_nodes(); ++i) {
                if (node_remains[i]) {
                    new_nodes[node_remains_prefix[i] - 1] = graph.nodes[i];
                }
            }

            /* Swapping old graph for new graph */
            graph.nodes.swap(new_nodes);
            graph.edges.swap(new_edges);
            graph.sort_edges();
        }

        return mst;
    }
};

#endif
