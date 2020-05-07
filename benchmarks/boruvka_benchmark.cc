#include <omp.h>

#include "../benchmark.h"
#include "../parallel_boruvka.h"
#include "../graph.h"
#include "../sequential_boruvka.h"
#include "../timer.h"

const u32 NUM_ITER = 50;
const u32 MAX_N = 1'000'000;
const u32 STEP = 200'000;

int main() {
    ParallelBoruvkaMST boruvka;
    SequentialBoruvkaMST sequential_mst;

    std::cout << omp_get_max_threads() << "\n";

    for (u32 n = STEP; n <= MAX_N; n += STEP) {
            u32 avg_par_time = 0;
            u32 avg_seq_time = 0;

            u32 m = n * 20;
            Graph G = generate_graph(n, m);

            for (u32 iter = 1; iter <= NUM_ITER; ++iter) {
                // std::cout << "Iter no. " << iter << "\n";
                
                {
                    escape(&G);
                    u64 start = currentSeconds();

                    auto mst = boruvka.calculate_mst(G);

                    u64 finish = currentSeconds();
                    escape(&mst);

                    avg_par_time += finish - start;
                }
                // std::cout << "Parallel mst finished\n";
                
                {
                    escape(&G);
                    u64 start = currentSeconds();

                    auto mst = sequential_mst.calculate_mst(G);

                    u64 finish = currentSeconds();
                    escape(&mst);

                    avg_seq_time += finish - start;
                }
                // std::cout << "Sequential mst finished\n";
            }

            avg_par_time /= NUM_ITER;
            avg_seq_time /= NUM_ITER;

            std::cout << n << " " << avg_par_time << " " << avg_seq_time << " " << static_cast<double>(avg_seq_time) / avg_par_time << "\n";
    }
}
