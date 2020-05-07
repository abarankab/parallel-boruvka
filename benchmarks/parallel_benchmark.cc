#include <omp.h>
#include <stdlib.h>

#include "../benchmark.h"
#include "../parallel_boruvka.h"
#include "../graph.h"
#include "../timer.h"

const u32 NUM_ITER = 10;

int main(int argc, char* argv[]) {
    ParallelBoruvkaMST boruvka;
    if (argc < 2) {
        std::cerr << "Please specify path to graph\n";
        exit(-1);
    }
    if (argc < 3) {
        std::cerr << "Please specify number of threads\n";
        exit(-1);
    }

    Graph G = load_graph(argv[1]);
    u32 num_threads = atoi(argv[2]);

    u64 avg_par_time = 0;

    for (u32 iter = 1; iter <= NUM_ITER; ++iter) {
        escape(&G);
        u64 start = currentSeconds();

        auto mst = boruvka.calculate_mst(G, num_threads);

        u64 finish = currentSeconds();
        escape(&mst);

        avg_par_time += finish - start;
    }

    avg_par_time /= NUM_ITER;

    std::cout << num_threads << " "
              << avg_par_time << "\n";

    return 0;
}
