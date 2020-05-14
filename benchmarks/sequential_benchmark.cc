#include <omp.h>

#include "../benchmark.h"
#include "../sequential_boruvka.h"
#include "../graph.h"
#include "../timer.h"

const u32 NUM_ITER = 10;

int main(int argc, char* argv[]) {
    SequentialBoruvkaMST boruvka;
    if (argc < 2) {
        std::cerr << "Please specify path to graph\n";
        exit(-1);
    }

    Graph G = load_graph(argv[1]);

    u64 avg_seq_time = 0;

    for (u32 iter = 1; iter <= NUM_ITER; ++iter) {
        escape(&G);
        u64 start = currentSeconds();

        auto mst = boruvka.calculate_mst(G);

        u64 finish = currentSeconds();
        escape(&mst);

        avg_seq_time += finish - start;
    }

    avg_seq_time /= NUM_ITER;

    std::cout << avg_seq_time << "\n";

    return 0;
}
