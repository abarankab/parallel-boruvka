#ifndef __SEQUENTIAL_MST_H
#define __SEQUENTIAL_MST_H

#include <algorithm>
#include <vector>

#include "graph.h"
#include "parallel_array.h"
#include "sequential_dsu.h"

struct SequentialBoruvkaMST {
    ParallelArray<Edge> calculate_mst(Graph graph) {
        SequentialDSU node_sets(graph.num_nodes());
        ParallelArray<Edge> mst(graph.num_nodes() - 1);
        u32 current_mst_size = 0;
        u32 initial_num_nodes = graph.num_nodes();

        while (graph.num_nodes() != 1) {
            std::vector<std::pair<u32, u32>> shortest_edges(initial_num_nodes, 
                                                           { 0, std::numeric_limits<u32>::max() });

            for (u32 i = 0; i < graph.num_edges(); ++i) {
                const Edge& e = graph.edges[i];

                if (shortest_edges[e.from].second > e.weight) {
                    shortest_edges[e.from] = { i, e.weight };
                }
            }

            for (u32 i = 0; i < graph.num_nodes(); ++i) {
                u32 u = graph.nodes[i];
                const Edge& min_edge_u = graph.edges[shortest_edges[u].first];

                u32 v = min_edge_u.to;
                const Edge& min_edge_v = graph.edges[shortest_edges[v].first];
                
                if (min_edge_v.to != u || (min_edge_v.to == u && u < v)) {
                    node_sets.unite(u, v);
                    mst[current_mst_size++] = min_edge_u;
                }
            }

            std::vector<Edge> new_edges;
            for (u32 i = 0; i < graph.num_edges(); ++i) {
                if (!node_sets.same_set(graph.edges[i].from, graph.edges[i].to)) {
                    Edge e = graph.edges[i];
                    e.from = node_sets.find_root(e.from);
                    e.to = node_sets.find_root(e.to);
                    new_edges.push_back(e);
                }
            }

            std::vector<u32> new_nodes;
            for (u32 i = 0; i < graph.num_nodes(); ++i) {
                if (node_sets.find_root(graph.nodes[i]) == graph.nodes[i]) {
                    new_nodes.push_back(graph.nodes[i]);
                }
            }

            graph.nodes = ParallelArray<u32>(new_nodes.size());
            graph.edges = ParallelArray<Edge>(new_edges.size());

            for (u32 i = 0; i < new_nodes.size(); ++i) {
                graph.nodes[i] = new_nodes[i];
            }

            for (u32 i = 0; i < new_edges.size(); ++i) {
                graph.edges[i] = new_edges[i];
            }
        }

        return mst;
    }
};

#endif
