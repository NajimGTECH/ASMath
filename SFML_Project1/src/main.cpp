#define ANKERL_NANOBENCH_IMPLEMENT

#include "nanobench.h"
#include "Vector2.h"
#include "Vector2SSE.h"

#include <iostream>
#include <vector>

int main()
{
    constexpr std::size_t N = 10000;
    constexpr float T = 0.5f;

    std::vector<math::Vector2<float>> vectorsA;
    std::vector<math::Vector2<float>> vectorsB;

    vectorsA.reserve(N);
    vectorsB.reserve(N);

    for (std::size_t i = 0; i < N; ++i)
    {
        vectorsA.emplace_back(
            static_cast<float>(i),
            static_cast<float>(i + 1)
        );

        vectorsB.emplace_back(
            static_cast<float>(i + 2),
            static_cast<float>(i + 3)
        );
    }

    ankerl::nanobench::Bench bench;

    bench.title("Vector2 Operations")
        .unit("element")
        .batch(N)
        .relative(true)
        .warmup(100);

    // =========================================================
    // DOT - Classic
    // =========================================================

    bench.run("Classic Dot", [&]
        {
            float result = 0.0f;

            for (std::size_t i = 0; i < N; ++i)
            {
                result += vectorsA[i].Dot(vectorsB[i]);
            }

            ankerl::nanobench::doNotOptimizeAway(result);
        });

    // =========================================================
    // DOT - SIMD
    // =========================================================

    bench.run("SIMD Dot", [&]
        {
            float result = 0.0f;

            for (std::size_t i = 0; i < N; ++i)
            {
                result += math::Vector2SSE::Dot(
                    vectorsA[i],
                    vectorsB[i]
                );
            }

            ankerl::nanobench::doNotOptimizeAway(result);
        });

    // =========================================================
    // SCALE - Classic
    // =========================================================

    bench.run("Classic Scale", [&]
        {
            math::Vector2<float> result;

            for (std::size_t i = 0; i < N; ++i)
            {
                result = math::Vector2<float>::Scale(
                    vectorsA[i],
                    vectorsB[i]
                );
            }

            ankerl::nanobench::doNotOptimizeAway(result);
        });

    // =========================================================
    // SCALE - SIMD
    // =========================================================

    bench.run("SIMD Scale", [&]
        {
            math::Vector2<float> result;

            for (std::size_t i = 0; i < N; ++i)
            {
                result = math::Vector2SSE::Scale(
                    vectorsA[i],
                    vectorsB[i]
                );
            }

            ankerl::nanobench::doNotOptimizeAway(result);
        });

    // =========================================================
    // LERP - Classic
    // =========================================================

    bench.run("Classic Lerp", [&]
        {
            math::Vector2<float> result;

            for (std::size_t i = 0; i < N; ++i)
            {
                result = math::Vector2<float>::Lerp(
                    vectorsA[i],
                    vectorsB[i],
                    T
                );
            }

            ankerl::nanobench::doNotOptimizeAway(result);
        });

    // =========================================================
    // LERP - SIMD
    // =========================================================

    bench.run("SIMD Lerp", [&]
        {
            math::Vector2<float> result;

            for (std::size_t i = 0; i < N; ++i)
            {
                result = math::Vector2SSE::Lerp(
                    vectorsA[i],
                    vectorsB[i],
                    T
                );
            }

            ankerl::nanobench::doNotOptimizeAway(result);
        });

    return 0;
}