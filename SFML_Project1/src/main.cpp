#define ANKERL_NANOBENCH_IMPLEMENT

#include "nanobench.h"

#include "Vector2.h"
#include "Vector2SSE.h"

#include "Mat3.h"
#include "Mat3SSE.h"

#include <cstddef>
#include <vector>

int main()
{
    constexpr std::size_t N = 10000;
    constexpr float T = 0.5f;

    // =========================================================
    // VECTOR2 - AOS
    // =========================================================

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

    // =========================================================
    // VECTOR2 - SOA
    // =========================================================

    math::Vector2SSE::Vector2SoA vectorsASoA;
    math::Vector2SSE::Vector2SoA vectorsBSoA;

    vectorsASoA.Reserve(N);
    vectorsBSoA.Reserve(N);

    for (std::size_t i = 0; i < N; ++i)
    {
        vectorsASoA.Add(
            static_cast<float>(i),
            static_cast<float>(i + 1)
        );

        vectorsBSoA.Add(
            static_cast<float>(i + 2),
            static_cast<float>(i + 3)
        );
    }

    // =========================================================
    // MAT3 - AOS
    // =========================================================

    std::vector<math::Mat3<float>> matrices;
    std::vector<math::Mat3<float>> matricesB;

    matrices.reserve(N);
    matricesB.reserve(N);

    for (std::size_t i = 0; i < N; ++i)
    {
        const float value = static_cast<float>(i);

        matrices.emplace_back(
            value + 1.0f,
            value + 2.0f,
            value + 3.0f,

            value + 4.0f,
            value + 5.0f,
            value + 6.0f,

            value + 7.0f,
            value + 8.0f,
            value + 10.0f
        );

        matricesB.emplace_back(
            value + 11.0f,
            value + 12.0f,
            value + 13.0f,

            value + 14.0f,
            value + 15.0f,
            value + 16.0f,

            value + 17.0f,
            value + 18.0f,
            value + 20.0f
        );
    }

    // =========================================================
    // MAT3 - SOA
    // =========================================================

    math::Mat3SSE::Mat3SoA matricesSoA;
    math::Mat3SSE::Mat3SoA matricesBSoA;

    matricesSoA.Reserve(N);
    matricesBSoA.Reserve(N);

    for (std::size_t i = 0; i < N; ++i)
    {
        const float value = static_cast<float>(i);

        matricesSoA.Add(
            value + 1.0f,
            value + 2.0f,
            value + 3.0f,

            value + 4.0f,
            value + 5.0f,
            value + 6.0f,

            value + 7.0f,
            value + 8.0f,
            value + 10.0f
        );

        matricesBSoA.Add(
            value + 11.0f,
            value + 12.0f,
            value + 13.0f,

            value + 14.0f,
            value + 15.0f,
            value + 16.0f,

            value + 17.0f,
            value + 18.0f,
            value + 20.0f
        );
    }

    // =========================================================
    // OUTPUT BUFFERS
    // =========================================================

    math::Vector2SSE::Vector2SoA scaleResultSoA;
    math::Vector2SSE::Vector2SoA lerpResultSoA;

    scaleResultSoA.Reserve(N);
    lerpResultSoA.Reserve(N);

    // Mat3 transpose
    std::vector<math::Mat3<float>> transposeResultAoS;
    transposeResultAoS.resize(N);

    math::Mat3SSE::Mat3SoA transposeResultSoA;
    transposeResultSoA.Resize(N);

    // Mat3 multiplication
    std::vector<math::Mat3<float>> multiplyResultAoS;
    multiplyResultAoS.resize(N);

    math::Mat3SSE::Mat3SoA multiplyResultSoA;
    multiplyResultSoA.Resize(N);

    // =========================================================
    // BENCHMARK
    // =========================================================

    ankerl::nanobench::Bench bench;

    bench.title("Math Operations")
        .unit("element")
        .batch(N)
        .relative(true)
        .warmup(100);

    // =========================================================
    // VECTOR2 - DOT
    // =========================================================

    bench.run("Vector2 AoS Classic Dot", [&]
        {
            float result = 0.0f;

            for (std::size_t i = 0; i < N; ++i)
            {
                result += vectorsA[i].Dot(vectorsB[i]);
            }

            ankerl::nanobench::doNotOptimizeAway(result);
        });

    bench.run("Vector2 AoS SIMD Dot", [&]
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

    bench.run("Vector2 SoA Classic Dot", [&]
        {
            float result = 0.0f;

            for (std::size_t i = 0; i < N; ++i)
            {
                result +=
                    vectorsASoA.x[i] * vectorsBSoA.x[i] +
                    vectorsASoA.y[i] * vectorsBSoA.y[i];
            }

            ankerl::nanobench::doNotOptimizeAway(result);
        });

    bench.run("Vector2 SoA SIMD Dot", [&]
        {
            const float result =
                math::Vector2SSE::DotSoA(
                    vectorsASoA,
                    vectorsBSoA
                );

            ankerl::nanobench::doNotOptimizeAway(result);
        });

    // =========================================================
    // VECTOR2 - SCALE
    // =========================================================

    bench.run("Vector2 AoS Classic Scale", [&]
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

    bench.run("Vector2 AoS SIMD Scale", [&]
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

    bench.run("Vector2 SoA Classic Scale", [&]
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                scaleResultSoA.x[i] =
                    vectorsASoA.x[i] * vectorsBSoA.x[i];

                scaleResultSoA.y[i] =
                    vectorsASoA.y[i] * vectorsBSoA.y[i];
            }

            ankerl::nanobench::doNotOptimizeAway(
                scaleResultSoA.x.data()
            );

            ankerl::nanobench::doNotOptimizeAway(
                scaleResultSoA.y.data()
            );
        });

    bench.run("Vector2 SoA SIMD Scale", [&]
        {
            math::Vector2SSE::ScaleSoA(
                vectorsASoA,
                vectorsBSoA,
                scaleResultSoA
            );

            ankerl::nanobench::doNotOptimizeAway(
                scaleResultSoA.x.data()
            );

            ankerl::nanobench::doNotOptimizeAway(
                scaleResultSoA.y.data()
            );
        });

    // =========================================================
    // VECTOR2 - LERP
    // =========================================================

    bench.run("Vector2 AoS Classic Lerp", [&]
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

    bench.run("Vector2 AoS SIMD Lerp", [&]
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

    bench.run("Vector2 SoA Classic Lerp", [&]
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                lerpResultSoA.x[i] =
                    vectorsASoA.x[i] +
                    (vectorsBSoA.x[i] - vectorsASoA.x[i]) * T;

                lerpResultSoA.y[i] =
                    vectorsASoA.y[i] +
                    (vectorsBSoA.y[i] - vectorsASoA.y[i]) * T;
            }

            ankerl::nanobench::doNotOptimizeAway(
                lerpResultSoA.x.data()
            );

            ankerl::nanobench::doNotOptimizeAway(
                lerpResultSoA.y.data()
            );
        });

    bench.run("Vector2 SoA SIMD Lerp", [&]
        {
            math::Vector2SSE::LerpSoA(
                vectorsASoA,
                vectorsBSoA,
                T,
                lerpResultSoA
            );

            ankerl::nanobench::doNotOptimizeAway(
                lerpResultSoA.x.data()
            );

            ankerl::nanobench::doNotOptimizeAway(
                lerpResultSoA.y.data()
            );
        });

    // =========================================================
    // MAT3 - DETERMINANT
    // =========================================================

    bench.run("Mat3 AoS Classic Determinant", [&]
        {
            float result = 0.0f;

            for (std::size_t i = 0; i < N; ++i)
            {
                result += matrices[i].Determinant();
            }

            ankerl::nanobench::doNotOptimizeAway(result);
        });

    bench.run("Mat3 AoS SIMD Determinant", [&]
        {
            float result = 0.0f;

            for (std::size_t i = 0; i < N; ++i)
            {
                result += math::Mat3SSE::Determinant(
                    matrices[i]
                );
            }

            ankerl::nanobench::doNotOptimizeAway(result);
        });

    bench.run("Mat3 SoA Classic Determinant", [&]
        {
            const float result =
                math::Mat3SSE::DeterminantSoA(
                    matricesSoA
                );

            ankerl::nanobench::doNotOptimizeAway(result);
        });

    bench.run("Mat3 SoA SIMD Determinant", [&]
        {
            const float result =
                math::Mat3SSE::DeterminantSoASIMD(
                    matricesSoA
                );

            ankerl::nanobench::doNotOptimizeAway(result);
        });

    // =========================================================
    // MAT3 - TRANSPOSE
    // =========================================================

    bench.run("Mat3 AoS Classic Transpose", [&]
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                transposeResultAoS[i] =
                    math::Mat3SSE::Transpose(
                        matrices[i]
                    );
            }

            ankerl::nanobench::doNotOptimizeAway(
                transposeResultAoS.data()
            );
        });

    bench.run("Mat3 AoS SIMD Transpose", [&]
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                transposeResultAoS[i] =
                    math::Mat3SSE::TransposeSIMD(
                        matrices[i]
                    );
            }

            ankerl::nanobench::doNotOptimizeAway(
                transposeResultAoS.data()
            );
        });

    bench.run("Mat3 SoA Classic Transpose", [&]
        {
            math::Mat3SSE::TransposeSoA(
                matricesSoA,
                transposeResultSoA
            );

            ankerl::nanobench::doNotOptimizeAway(
                transposeResultSoA.m00.data()
            );
        });

    bench.run("Mat3 SoA SIMD Transpose", [&]
        {
            math::Mat3SSE::TransposeSoASIMD(
                matricesSoA,
                transposeResultSoA
            );

            ankerl::nanobench::doNotOptimizeAway(
                transposeResultSoA.m00.data()
            );
        });

    // =========================================================
    // MAT3 - MULTIPLICATION
    // =========================================================

    bench.run("Mat3 AoS Classic Multiply", [&]
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                multiplyResultAoS[i] =
                    math::Mat3SSE::Multiply(
                        matrices[i],
                        matricesB[i]
                    );
            }

            ankerl::nanobench::doNotOptimizeAway(
                multiplyResultAoS.data()
            );
        });

    bench.run("Mat3 AoS SIMD Multiply", [&]
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                multiplyResultAoS[i] =
                    math::Mat3SSE::MultiplySIMD(
                        matrices[i],
                        matricesB[i]
                    );
            }

            ankerl::nanobench::doNotOptimizeAway(
                multiplyResultAoS.data()
            );
        });

    bench.run("Mat3 SoA Classic Multiply", [&]
        {
            math::Mat3SSE::MultiplySoA(
                matricesSoA,
                matricesBSoA,
                multiplyResultSoA
            );

            ankerl::nanobench::doNotOptimizeAway(
                multiplyResultSoA.m00.data()
            );
        });

    bench.run("Mat3 SoA SIMD Multiply", [&]
        {
            math::Mat3SSE::MultiplySoASIMD(
                matricesSoA,
                matricesBSoA,
                multiplyResultSoA
            );

            ankerl::nanobench::doNotOptimizeAway(
                multiplyResultSoA.m00.data()
            );
        });

    return 0;
}