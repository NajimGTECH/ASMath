#pragma once

#include <immintrin.h>
#include <cstddef>

#include "Vector2.h"

namespace math
{
    namespace Vector2SSE
    {
        // =========================================================
        // SOA DATA
        // =========================================================

        struct Vector2SoA
        {
            std::vector<float> x;
            std::vector<float> y;

            void Reserve(std::size_t size)
            {
                x.reserve(size);
                y.reserve(size);
            }

            void Add(float xValue, float yValue)
            {
                x.push_back(xValue);
                y.push_back(yValue);
            }

            std::size_t Size() const
            {
                return x.size();
            }
        };

        // =========================================================
        // AOS - DOT
        // =========================================================

        inline float Dot(
            const Vector2<float>& a,
            const Vector2<float>& b)
        {
            const __m128 vecA = _mm_set_ps(
                0.0f,
                0.0f,
                a.y,
                a.x
            );

            const __m128 vecB = _mm_set_ps(
                0.0f,
                0.0f,
                b.y,
                b.x
            );

            const __m128 product = _mm_mul_ps(
                vecA,
                vecB
            );

            const __m128 shuffled = _mm_shuffle_ps(
                product,
                product,
                _MM_SHUFFLE(1, 1, 1, 1)
            );

            const __m128 result = _mm_add_ss(
                product,
                shuffled
            );

            return _mm_cvtss_f32(result);
        }

        // =========================================================
        // AOS - SCALE
        // =========================================================

        inline Vector2<float> Scale(
            const Vector2<float>& a,
            const Vector2<float>& b)
        {
            const __m128 vecA = _mm_set_ps(
                0.0f,
                0.0f,
                a.y,
                a.x
            );

            const __m128 vecB = _mm_set_ps(
                0.0f,
                0.0f,
                b.y,
                b.x
            );

            const __m128 result = _mm_mul_ps(
                vecA,
                vecB
            );

            const float x = _mm_cvtss_f32(result);

            const __m128 yVector = _mm_shuffle_ps(
                result,
                result,
                _MM_SHUFFLE(1, 1, 1, 1)
            );

            const float y = _mm_cvtss_f32(yVector);

            return Vector2<float>(x, y);
        }

        // =========================================================
        // AOS - LERP
        // =========================================================

        inline Vector2<float> Lerp(
            const Vector2<float>& a,
            const Vector2<float>& b,
            float t)
        {
            const __m128 vecA = _mm_set_ps(
                0.0f,
                0.0f,
                a.y,
                a.x
            );

            const __m128 vecB = _mm_set_ps(
                0.0f,
                0.0f,
                b.y,
                b.x
            );

            const __m128 vecT = _mm_set1_ps(t);

            const __m128 difference = _mm_sub_ps(
                vecB,
                vecA
            );

            const __m128 scaledDifference = _mm_mul_ps(
                difference,
                vecT
            );

            const __m128 result = _mm_add_ps(
                vecA,
                scaledDifference
            );

            const float x = _mm_cvtss_f32(result);

            const __m128 yVector = _mm_shuffle_ps(
                result,
                result,
                _MM_SHUFFLE(1, 1, 1, 1)
            );

            const float y = _mm_cvtss_f32(yVector);

            return Vector2<float>(x, y);
        }

        // =========================================================
        // SOA - DOT
        // =========================================================

        inline float DotSoA(
            const Vector2SoA& a,
            const Vector2SoA& b)
        {
            float result = 0.0f;

            const std::size_t size = a.Size();

            std::size_t i = 0;

            for (; i + 4 <= size; i += 4)
            {
                const __m128 ax = _mm_loadu_ps(&a.x[i]);
                const __m128 ay = _mm_loadu_ps(&a.y[i]);

                const __m128 bx = _mm_loadu_ps(&b.x[i]);
                const __m128 by = _mm_loadu_ps(&b.y[i]);

                const __m128 productX = _mm_mul_ps(ax, bx);
                const __m128 productY = _mm_mul_ps(ay, by);

                const __m128 products = _mm_add_ps(
                    productX,
                    productY
                );

                alignas(16) float values[4];

                _mm_store_ps(values, products);

                result += values[0];
                result += values[1];
                result += values[2];
                result += values[3];
            }

            for (; i < size; ++i)
            {
                result +=
                    a.x[i] * b.x[i] +
                    a.y[i] * b.y[i];
            }

            return result;
        }

        // =========================================================
        // SOA - SCALE
        // =========================================================

        inline void ScaleSoA(
            const Vector2SoA& a,
            const Vector2SoA& b,
            Vector2SoA& result)
        {
            const std::size_t size = a.Size();

            result.x.resize(size);
            result.y.resize(size);

            std::size_t i = 0;

            for (; i + 4 <= size; i += 4)
            {
                const __m128 ax = _mm_loadu_ps(&a.x[i]);
                const __m128 ay = _mm_loadu_ps(&a.y[i]);

                const __m128 bx = _mm_loadu_ps(&b.x[i]);
                const __m128 by = _mm_loadu_ps(&b.y[i]);

                const __m128 resultX = _mm_mul_ps(ax, bx);
                const __m128 resultY = _mm_mul_ps(ay, by);

                _mm_storeu_ps(&result.x[i], resultX);
                _mm_storeu_ps(&result.y[i], resultY);
            }

            for (; i < size; ++i)
            {
                result.x[i] = a.x[i] * b.x[i];
                result.y[i] = a.y[i] * b.y[i];
            }
        }

        // =========================================================
        // SOA - LERP
        // =========================================================

        inline void LerpSoA(
            const Vector2SoA& a,
            const Vector2SoA& b,
            float t,
            Vector2SoA& result)
        {
            const std::size_t size = a.Size();

            result.x.resize(size);
            result.y.resize(size);

            const __m128 vecT = _mm_set1_ps(t);

            std::size_t i = 0;

            for (; i + 4 <= size; i += 4)
            {
                const __m128 ax = _mm_loadu_ps(&a.x[i]);
                const __m128 ay = _mm_loadu_ps(&a.y[i]);

                const __m128 bx = _mm_loadu_ps(&b.x[i]);
                const __m128 by = _mm_loadu_ps(&b.y[i]);

                const __m128 differenceX = _mm_sub_ps(bx, ax);
                const __m128 differenceY = _mm_sub_ps(by, ay);

                const __m128 resultX = _mm_add_ps(
                    ax,
                    _mm_mul_ps(differenceX, vecT)
                );

                const __m128 resultY = _mm_add_ps(
                    ay,
                    _mm_mul_ps(differenceY, vecT)
                );

                _mm_storeu_ps(&result.x[i], resultX);
                _mm_storeu_ps(&result.y[i], resultY);
            }

            for (; i < size; ++i)
            {
                result.x[i] =
                    a.x[i] +
                    (b.x[i] - a.x[i]) * t;

                result.y[i] =
                    a.y[i] +
                    (b.y[i] - a.y[i]) * t;
            }
        }
    }
}