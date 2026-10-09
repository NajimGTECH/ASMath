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
        // MAT3 - SOA
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

            void Resize(std::size_t size)
            {
                m00.resize(size);
                m01.resize(size);
                m02.resize(size);

                m10.resize(size);
                m11.resize(size);
                m12.resize(size);

                m20.resize(size);
                m21.resize(size);
                m22.resize(size);
            }

            void Add(
                float m00Value, float m01Value, float m02Value,
                float m10Value, float m11Value, float m12Value,
                float m20Value, float m21Value, float m22Value)
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
        // DETERMINANT - AOS
        // =========================================================

        inline float Determinant(const Mat3<float>& matrix)
        {
            return
                matrix.m[0][0] *
                (matrix.m[1][1] * matrix.m[2][2] -
                    matrix.m[1][2] * matrix.m[2][1])

                - matrix.m[0][1] *
                (matrix.m[1][0] * matrix.m[2][2] -
                    matrix.m[1][2] * matrix.m[2][0])

                + matrix.m[0][2] *
                (matrix.m[1][0] * matrix.m[2][1] -
                    matrix.m[1][1] * matrix.m[2][0]);
        }


        // =========================================================
        // DETERMINANT - SOA CLASSIC
        // =========================================================

        inline float DeterminantSoA(const Mat3SoA& matrices)
        {
            float result = 0.0f;

            const std::size_t size = matrices.Size();

            for (std::size_t i = 0; i < size; ++i)
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

                const float determinant =
                    matrices.m00[i] * minor0 -
                    matrices.m01[i] * minor1 +
                    matrices.m02[i] * minor2;

                result += determinant;
            }

            return result;
        }


        // =========================================================
        // DETERMINANT - SOA SIMD
        // =========================================================

        inline float DeterminantSoASIMD(const Mat3SoA& matrices)
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
                        _mm_mul_ps(m12, m21));

                const __m128 minor1 =
                    _mm_sub_ps(
                        _mm_mul_ps(m10, m22),
                        _mm_mul_ps(m12, m20));

                const __m128 minor2 =
                    _mm_sub_ps(
                        _mm_mul_ps(m10, m21),
                        _mm_mul_ps(m11, m20));

                const __m128 term0 =
                    _mm_mul_ps(m00, minor0);

                const __m128 term1 =
                    _mm_mul_ps(m01, minor1);

                const __m128 term2 =
                    _mm_mul_ps(m02, minor2);

                const __m128 determinant =
                    _mm_add_ps(
                        _mm_sub_ps(term0, term1),
                        term2);

                alignas(16) float values[4];

                _mm_store_ps(values, determinant);

                result +=
                    values[0] +
                    values[1] +
                    values[2] +
                    values[3];
            }

            // Tail
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


        // =========================================================
        // TRANSPOSE - AOS CLASSIC
        // =========================================================

        inline Mat3<float> Transpose(const Mat3<float>& matrix)
        {
            return Mat3<float>(
                matrix.m[0][0],
                matrix.m[1][0],
                matrix.m[2][0],

                matrix.m[0][1],
                matrix.m[1][1],
                matrix.m[2][1],

                matrix.m[0][2],
                matrix.m[1][2],
                matrix.m[2][2]
            );
        }


        // =========================================================
        // TRANSPOSE - AOS SIMD
        // =========================================================

        inline Mat3<float> TransposeSIMD(const Mat3<float>& matrix)
        {
            __m128 row0 = _mm_set_ps(
                0.0f,
                matrix.m[0][2],
                matrix.m[0][1],
                matrix.m[0][0]);

            __m128 row1 = _mm_set_ps(
                0.0f,
                matrix.m[1][2],
                matrix.m[1][1],
                matrix.m[1][0]);

            __m128 row2 = _mm_set_ps(
                0.0f,
                matrix.m[2][2],
                matrix.m[2][1],
                matrix.m[2][0]);

            __m128 row3 = _mm_setzero_ps();

            _MM_TRANSPOSE4_PS(
                row0,
                row1,
                row2,
                row3);

            alignas(16) float column0[4];
            alignas(16) float column1[4];
            alignas(16) float column2[4];

            _mm_store_ps(column0, row0);
            _mm_store_ps(column1, row1);
            _mm_store_ps(column2, row2);

            return Mat3<float>(
                column0[0],
                column0[1],
                column0[2],

                column1[0],
                column1[1],
                column1[2],

                column2[0],
                column2[1],
                column2[2]
            );
        }


        // =========================================================
        // TRANSPOSE - SOA CLASSIC
        // =========================================================

        inline void TransposeSoA(
            const Mat3SoA& input,
            Mat3SoA& output)
        {
            const std::size_t size = input.Size();

            for (std::size_t i = 0; i < size; ++i)
            {
                output.m00[i] = input.m00[i];
                output.m01[i] = input.m10[i];
                output.m02[i] = input.m20[i];

                output.m10[i] = input.m01[i];
                output.m11[i] = input.m11[i];
                output.m12[i] = input.m21[i];

                output.m20[i] = input.m02[i];
                output.m21[i] = input.m12[i];
                output.m22[i] = input.m22[i];
            }
        }


        // =========================================================
        // TRANSPOSE - SOA SIMD
        // =========================================================

        inline void TransposeSoASIMD(
            const Mat3SoA& input,
            Mat3SoA& output)
        {
            const std::size_t size = input.Size();

            std::size_t i = 0;

            for (; i + 4 <= size; i += 4)
            {
                const __m128 m00 =
                    _mm_loadu_ps(&input.m00[i]);

                const __m128 m01 =
                    _mm_loadu_ps(&input.m01[i]);

                const __m128 m02 =
                    _mm_loadu_ps(&input.m02[i]);

                const __m128 m10 =
                    _mm_loadu_ps(&input.m10[i]);

                const __m128 m11 =
                    _mm_loadu_ps(&input.m11[i]);

                const __m128 m12 =
                    _mm_loadu_ps(&input.m12[i]);

                const __m128 m20 =
                    _mm_loadu_ps(&input.m20[i]);

                const __m128 m21 =
                    _mm_loadu_ps(&input.m21[i]);

                const __m128 m22 =
                    _mm_loadu_ps(&input.m22[i]);

                _mm_storeu_ps(&output.m00[i], m00);
                _mm_storeu_ps(&output.m01[i], m10);
                _mm_storeu_ps(&output.m02[i], m20);

                _mm_storeu_ps(&output.m10[i], m01);
                _mm_storeu_ps(&output.m11[i], m11);
                _mm_storeu_ps(&output.m12[i], m21);

                _mm_storeu_ps(&output.m20[i], m02);
                _mm_storeu_ps(&output.m21[i], m12);
                _mm_storeu_ps(&output.m22[i], m22);
            }

            // Tail
            for (; i < size; ++i)
            {
                output.m00[i] = input.m00[i];
                output.m01[i] = input.m10[i];
                output.m02[i] = input.m20[i];

                output.m10[i] = input.m01[i];
                output.m11[i] = input.m11[i];
                output.m12[i] = input.m21[i];

                output.m20[i] = input.m02[i];
                output.m21[i] = input.m12[i];
                output.m22[i] = input.m22[i];
            }
        }


        // =========================================================
        // MATRIX MULTIPLICATION - AOS CLASSIC
        // =========================================================

        inline Mat3<float> Multiply(
            const Mat3<float>& a,
            const Mat3<float>& b)
        {
            return Mat3<float>(
                a.m[0][0] * b.m[0][0] +
                a.m[0][1] * b.m[1][0] +
                a.m[0][2] * b.m[2][0],

                a.m[0][0] * b.m[0][1] +
                a.m[0][1] * b.m[1][1] +
                a.m[0][2] * b.m[2][1],

                a.m[0][0] * b.m[0][2] +
                a.m[0][1] * b.m[1][2] +
                a.m[0][2] * b.m[2][2],

                a.m[1][0] * b.m[0][0] +
                a.m[1][1] * b.m[1][0] +
                a.m[1][2] * b.m[2][0],

                a.m[1][0] * b.m[0][1] +
                a.m[1][1] * b.m[1][1] +
                a.m[1][2] * b.m[2][1],

                a.m[1][0] * b.m[0][2] +
                a.m[1][1] * b.m[1][2] +
                a.m[1][2] * b.m[2][2],

                a.m[2][0] * b.m[0][0] +
                a.m[2][1] * b.m[1][0] +
                a.m[2][2] * b.m[2][0],

                a.m[2][0] * b.m[0][1] +
                a.m[2][1] * b.m[1][1] +
                a.m[2][2] * b.m[2][1],

                a.m[2][0] * b.m[0][2] +
                a.m[2][1] * b.m[1][2] +
                a.m[2][2] * b.m[2][2]
            );
        }


        // =========================================================
        // MATRIX MULTIPLICATION - AOS SIMD
        // =========================================================

        inline Mat3<float> MultiplySIMD(
            const Mat3<float>& a,
            const Mat3<float>& b)
        {
            const __m128 bColumn0 = _mm_set_ps(
                0.0f,
                b.m[2][0],
                b.m[1][0],
                b.m[0][0]);

            const __m128 bColumn1 = _mm_set_ps(
                0.0f,
                b.m[2][1],
                b.m[1][1],
                b.m[0][1]);

            const __m128 bColumn2 = _mm_set_ps(
                0.0f,
                b.m[2][2],
                b.m[1][2],
                b.m[0][2]);

            const __m128 aRow0 = _mm_set_ps(
                0.0f,
                a.m[0][2],
                a.m[0][1],
                a.m[0][0]);

            const __m128 aRow1 = _mm_set_ps(
                0.0f,
                a.m[1][2],
                a.m[1][1],
                a.m[1][0]);

            const __m128 aRow2 = _mm_set_ps(
                0.0f,
                a.m[2][2],
                a.m[2][1],
                a.m[2][0]);

            auto Dot3 = [](__m128 left, __m128 right) -> float
                {
                    const __m128 product =
                        _mm_mul_ps(left, right);

                    alignas(16) float values[4];

                    _mm_store_ps(values, product);

                    return
                        values[0] +
                        values[1] +
                        values[2];
                };

            return Mat3<float>(
                Dot3(aRow0, bColumn0),
                Dot3(aRow0, bColumn1),
                Dot3(aRow0, bColumn2),

                Dot3(aRow1, bColumn0),
                Dot3(aRow1, bColumn1),
                Dot3(aRow1, bColumn2),

                Dot3(aRow2, bColumn0),
                Dot3(aRow2, bColumn1),
                Dot3(aRow2, bColumn2)
            );
        }


        // =========================================================
        // MATRIX MULTIPLICATION - SOA CLASSIC
        // =========================================================

        inline void MultiplySoA(
            const Mat3SoA& a,
            const Mat3SoA& b,
            Mat3SoA& result)
        {
            const std::size_t size = a.Size();

            for (std::size_t i = 0; i < size; ++i)
            {
                result.m00[i] =
                    a.m00[i] * b.m00[i] +
                    a.m01[i] * b.m10[i] +
                    a.m02[i] * b.m20[i];

                result.m01[i] =
                    a.m00[i] * b.m01[i] +
                    a.m01[i] * b.m11[i] +
                    a.m02[i] * b.m21[i];

                result.m02[i] =
                    a.m00[i] * b.m02[i] +
                    a.m01[i] * b.m12[i] +
                    a.m02[i] * b.m22[i];

                result.m10[i] =
                    a.m10[i] * b.m00[i] +
                    a.m11[i] * b.m10[i] +
                    a.m12[i] * b.m20[i];

                result.m11[i] =
                    a.m10[i] * b.m01[i] +
                    a.m11[i] * b.m11[i] +
                    a.m12[i] * b.m21[i];

                result.m12[i] =
                    a.m10[i] * b.m02[i] +
                    a.m11[i] * b.m12[i] +
                    a.m12[i] * b.m22[i];

                result.m20[i] =
                    a.m20[i] * b.m00[i] +
                    a.m21[i] * b.m10[i] +
                    a.m22[i] * b.m20[i];

                result.m21[i] =
                    a.m20[i] * b.m01[i] +
                    a.m21[i] * b.m11[i] +
                    a.m22[i] * b.m21[i];

                result.m22[i] =
                    a.m20[i] * b.m02[i] +
                    a.m21[i] * b.m12[i] +
                    a.m22[i] * b.m22[i];
            }
        }


        // =========================================================
        // MATRIX MULTIPLICATION - SOA SIMD
        // =========================================================

        inline void MultiplySoASIMD(
            const Mat3SoA& a,
            const Mat3SoA& b,
            Mat3SoA& result)
        {
            const std::size_t size = a.Size();

            std::size_t i = 0;

            for (; i + 4 <= size; i += 4)
            {
                const __m128 a00 = _mm_loadu_ps(&a.m00[i]);
                const __m128 a01 = _mm_loadu_ps(&a.m01[i]);
                const __m128 a02 = _mm_loadu_ps(&a.m02[i]);

                const __m128 a10 = _mm_loadu_ps(&a.m10[i]);
                const __m128 a11 = _mm_loadu_ps(&a.m11[i]);
                const __m128 a12 = _mm_loadu_ps(&a.m12[i]);

                const __m128 a20 = _mm_loadu_ps(&a.m20[i]);
                const __m128 a21 = _mm_loadu_ps(&a.m21[i]);
                const __m128 a22 = _mm_loadu_ps(&a.m22[i]);

                const __m128 b00 = _mm_loadu_ps(&b.m00[i]);
                const __m128 b01 = _mm_loadu_ps(&b.m01[i]);
                const __m128 b02 = _mm_loadu_ps(&b.m02[i]);

                const __m128 b10 = _mm_loadu_ps(&b.m10[i]);
                const __m128 b11 = _mm_loadu_ps(&b.m11[i]);
                const __m128 b12 = _mm_loadu_ps(&b.m12[i]);

                const __m128 b20 = _mm_loadu_ps(&b.m20[i]);
                const __m128 b21 = _mm_loadu_ps(&b.m21[i]);
                const __m128 b22 = _mm_loadu_ps(&b.m22[i]);

                const __m128 c00 =
                    _mm_add_ps(
                        _mm_add_ps(
                            _mm_mul_ps(a00, b00),
                            _mm_mul_ps(a01, b10)),
                        _mm_mul_ps(a02, b20));

                const __m128 c01 =
                    _mm_add_ps(
                        _mm_add_ps(
                            _mm_mul_ps(a00, b01),
                            _mm_mul_ps(a01, b11)),
                        _mm_mul_ps(a02, b21));

                const __m128 c02 =
                    _mm_add_ps(
                        _mm_add_ps(
                            _mm_mul_ps(a00, b02),
                            _mm_mul_ps(a01, b12)),
                        _mm_mul_ps(a02, b22));

                const __m128 c10 =
                    _mm_add_ps(
                        _mm_add_ps(
                            _mm_mul_ps(a10, b00),
                            _mm_mul_ps(a11, b10)),
                        _mm_mul_ps(a12, b20));

                const __m128 c11 =
                    _mm_add_ps(
                        _mm_add_ps(
                            _mm_mul_ps(a10, b01),
                            _mm_mul_ps(a11, b11)),
                        _mm_mul_ps(a12, b21));

                const __m128 c12 =
                    _mm_add_ps(
                        _mm_add_ps(
                            _mm_mul_ps(a10, b02),
                            _mm_mul_ps(a11, b12)),
                        _mm_mul_ps(a12, b22));

                const __m128 c20 =
                    _mm_add_ps(
                        _mm_add_ps(
                            _mm_mul_ps(a20, b00),
                            _mm_mul_ps(a21, b10)),
                        _mm_mul_ps(a22, b20));

                const __m128 c21 =
                    _mm_add_ps(
                        _mm_add_ps(
                            _mm_mul_ps(a20, b01),
                            _mm_mul_ps(a21, b11)),
                        _mm_mul_ps(a22, b21));

                const __m128 c22 =
                    _mm_add_ps(
                        _mm_add_ps(
                            _mm_mul_ps(a20, b02),
                            _mm_mul_ps(a21, b12)),
                        _mm_mul_ps(a22, b22));

                _mm_storeu_ps(&result.m00[i], c00);
                _mm_storeu_ps(&result.m01[i], c01);
                _mm_storeu_ps(&result.m02[i], c02);

                _mm_storeu_ps(&result.m10[i], c10);
                _mm_storeu_ps(&result.m11[i], c11);
                _mm_storeu_ps(&result.m12[i], c12);

                _mm_storeu_ps(&result.m20[i], c20);
                _mm_storeu_ps(&result.m21[i], c21);
                _mm_storeu_ps(&result.m22[i], c22);
            }

            // Scalar tail
            for (; i < size; ++i)
            {
                result.m00[i] =
                    a.m00[i] * b.m00[i] +
                    a.m01[i] * b.m10[i] +
                    a.m02[i] * b.m20[i];

                result.m01[i] =
                    a.m00[i] * b.m01[i] +
                    a.m01[i] * b.m11[i] +
                    a.m02[i] * b.m21[i];

                result.m02[i] =
                    a.m00[i] * b.m02[i] +
                    a.m01[i] * b.m12[i] +
                    a.m02[i] * b.m22[i];

                result.m10[i] =
                    a.m10[i] * b.m00[i] +
                    a.m11[i] * b.m10[i] +
                    a.m12[i] * b.m20[i];

                result.m11[i] =
                    a.m10[i] * b.m01[i] +
                    a.m11[i] * b.m11[i] +
                    a.m12[i] * b.m21[i];

                result.m12[i] =
                    a.m10[i] * b.m02[i] +
                    a.m11[i] * b.m12[i] +
                    a.m12[i] * b.m22[i];

                result.m20[i] =
                    a.m20[i] * b.m00[i] +
                    a.m21[i] * b.m10[i] +
                    a.m22[i] * b.m20[i];

                result.m21[i] =
                    a.m20[i] * b.m01[i] +
                    a.m21[i] * b.m11[i] +
                    a.m22[i] * b.m21[i];

                result.m22[i] =
                    a.m20[i] * b.m02[i] +
                    a.m21[i] * b.m12[i] +
                    a.m22[i] * b.m22[i];
            }
        }
    }
}