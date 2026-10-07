#pragma once

#include <immintrin.h>
#include <cstddef>
#include <vector>

#include "Mat3.h"

namespace math
{
    namespace Mat3SSE
    {
        // =========================================================
        // SOA DATA
        // =========================================================

        struct Mat3SoA
        {
            std::vector<float> m00;
            std::vector<float> m01;
            std::vector<float> m02;

            std::vector<float> m10;
            std::vector<float> m11;
            std::vector<float> m12;

            std::vector<float> m20;
            std::vector<float> m21;
            std::vector<float> m22;

            void Reserve(std::size_t size)
            {
                m00.reserve(size);
                m01.reserve(size);
                m02.reserve(size);

                m10.reserve(size);
                m11.reserve(size);
                m12.reserve(size);

                m20.reserve(size);
                m21.reserve(size);
                m22.reserve(size);
            }

            void Add(
                float m00Value,
                float m01Value,
                float m02Value,
                float m10Value,
                float m11Value,
                float m12Value,
                float m20Value,
                float m21Value,
                float m22Value)
            {
                m00.push_back(m00Value);
                m01.push_back(m01Value);
                m02.push_back(m02Value);

                m10.push_back(m10Value);
                m11.push_back(m11Value);
                m12.push_back(m12Value);

                m20.push_back(m20Value);
                m21.push_back(m21Value);
                m22.push_back(m22Value);
            }

            std::size_t Size() const
            {
                return m00.size();
            }
        };

        // =========================================================
        // AOS - DETERMINANT
        // =========================================================

        inline float Determinant(
            const Mat3<float>& matrix)
        {
            return
                matrix.m[0][0] *
                (
                    matrix.m[1][1] * matrix.m[2][2] -
                    matrix.m[1][2] * matrix.m[2][1]
                    )
                -
                matrix.m[0][1] *
                (
                    matrix.m[1][0] * matrix.m[2][2] -
                    matrix.m[1][2] * matrix.m[2][0]
                    )
                +
                matrix.m[0][2] *
                (
                    matrix.m[1][0] * matrix.m[2][1] -
                    matrix.m[1][1] * matrix.m[2][0]
                    );
        }

        // =========================================================
        // SOA - CLASSIC DETERMINANT
        // =========================================================

        inline float DeterminantSoA(
            const Mat3SoA& matrices)
        {
            float result = 0.0f;

            for (std::size_t i = 0; i < matrices.Size(); ++i)
            {
                const float minor0 =
                    matrices.m11[i] * matrices.m22[i] -
                    matrices.m12[i] * matrices.m21[i];

                const float minor1 =
                    matrices.m10[i] * matrices.m22[i] -
                    matrices.m12[i] * matrices.m20[i];

                const float minor2 =
                    matrices.m10[i] * matrices.m21[i] -
                    matrices.m11[i] * matrices.m20[i];

                result +=
                    matrices.m00[i] * minor0 -
                    matrices.m01[i] * minor1 +
                    matrices.m02[i] * minor2;
            }

            return result;
        }

        // =========================================================
        // SOA - SIMD DETERMINANT
        // =========================================================

        inline float DeterminantSoASIMD(
            const Mat3SoA& matrices)
        {
            float result = 0.0f;

            const std::size_t size = matrices.Size();

            std::size_t i = 0;

            for (; i + 4 <= size; i += 4)
            {
                const __m128 m00 = _mm_loadu_ps(&matrices.m00[i]);
                const __m128 m01 = _mm_loadu_ps(&matrices.m01[i]);
                const __m128 m02 = _mm_loadu_ps(&matrices.m02[i]);

                const __m128 m10 = _mm_loadu_ps(&matrices.m10[i]);
                const __m128 m11 = _mm_loadu_ps(&matrices.m11[i]);
                const __m128 m12 = _mm_loadu_ps(&matrices.m12[i]);

                const __m128 m20 = _mm_loadu_ps(&matrices.m20[i]);
                const __m128 m21 = _mm_loadu_ps(&matrices.m21[i]);
                const __m128 m22 = _mm_loadu_ps(&matrices.m22[i]);

                const __m128 minor0 =
                    _mm_sub_ps(
                        _mm_mul_ps(m11, m22),
                        _mm_mul_ps(m12, m21)
                    );

                const __m128 minor1 =
                    _mm_sub_ps(
                        _mm_mul_ps(m10, m22),
                        _mm_mul_ps(m12, m20)
                    );

                const __m128 minor2 =
                    _mm_sub_ps(
                        _mm_mul_ps(m10, m21),
                        _mm_mul_ps(m11, m20)
                    );

                const __m128 term0 =
                    _mm_mul_ps(m00, minor0);

                const __m128 term1 =
                    _mm_mul_ps(m01, minor1);

                const __m128 term2 =
                    _mm_mul_ps(m02, minor2);

                const __m128 determinant =
                    _mm_add_ps(
                        _mm_sub_ps(term0, term1),
                        term2
                    );

                alignas(16) float values[4];

                _mm_store_ps(values, determinant);

                result += values[0];
                result += values[1];
                result += values[2];
                result += values[3];
            }

            // Remaining elements
            for (; i < size; ++i)
            {
                const float minor0 =
                    matrices.m11[i] * matrices.m22[i] -
                    matrices.m12[i] * matrices.m21[i];

                const float minor1 =
                    matrices.m10[i] * matrices.m22[i] -
                    matrices.m12[i] * matrices.m20[i];

                const float minor2 =
                    matrices.m10[i] * matrices.m21[i] -
                    matrices.m11[i] * matrices.m20[i];

                result +=
                    matrices.m00[i] * minor0 -
                    matrices.m01[i] * minor1 +
                    matrices.m02[i] * minor2;
            }

            return result;
        }
    }
}