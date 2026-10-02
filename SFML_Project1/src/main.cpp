#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"

int main()
{
    constexpr std::size_t N = 1000;

    ankerl::nanobench::Bench bench;
    bench.title("Test nanobench")
        .unit("element")   // les résultats sont donnés par élément...
        .batch(N)          // ...car chaque run traite N éléments
        .relative(true)    // le 1er benchmark sert de référence (100%)
        .warmup(100);

    bench.run("push_back sans reserve", [&] {
        std::vector<int> v;
        for (std::size_t i = 0; i < N; ++i) {
            v.push_back(static_cast<int>(i));
        }
        ankerl::nanobench::doNotOptimizeAway(v.data());
        });

    bench.run("push_back avec reserve", [&] {
        std::vector<int> v;
        v.reserve(N);
        for (std::size_t i = 0; i < N; ++i) {
            v.push_back(static_cast<int>(i));
        }
        ankerl::nanobench::doNotOptimizeAway(v.data());
        });

    return 0;
}