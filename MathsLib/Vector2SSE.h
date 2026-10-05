#pragma once

#include <immintrin.h>
#include "Vector2.h"

namespace math
{
    namespace Vector2SSE
    {
        inline float Dot(const Vector2<float>& a, const Vector2<float>& b)
        {
            const __m128 vecA = _mm_set_ps(0.0f, 0.0f, a.y, a.x);
            const __m128 vecB = _mm_set_ps(0.0f, 0.0f, b.y, b.x);

            const __m128 product = _mm_mul_ps(vecA, vecB);

            const __m128 shuffled = _mm_shuffle_ps(
                product,
                product,
                _MM_SHUFFLE(1, 1, 1, 1)
            );

            const __m128 result = _mm_add_ss(product, shuffled);

            return _mm_cvtss_f32(result);
        }

        inline Vector2<float> Scale(const Vector2<float>& a,const Vector2<float>& b)
        {
            const __m128 vecA = _mm_set_ps(0.0f, 0.0f, a.y, a.x);
            const __m128 vecB = _mm_set_ps(0.0f, 0.0f, b.y, b.x);

            const __m128 result = _mm_mul_ps(vecA, vecB);

            return Vector2<float>(
                _mm_cvtss_f32(result),
                _mm_cvtss_f32(_mm_shuffle_ps(
                    result,
                    result,
                    _MM_SHUFFLE(1, 1, 1, 1)
                ))
            );
        }

        inline Vector2<float> Lerp(const Vector2<float>& a,const Vector2<float>& b,float t)
        {
            const __m128 vecA = _mm_set_ps(0.0f, 0.0f, a.y, a.x);
            const __m128 vecB = _mm_set_ps(0.0f, 0.0f, b.y, b.x);

            const __m128 vecT = _mm_set1_ps(t);

            // b - a
            const __m128 difference = _mm_sub_ps(vecB, vecA);

            // (b - a) * t
            const __m128 scaledDifference = _mm_mul_ps(difference, vecT);

            // a + ((b - a) * t)
            const __m128 result = _mm_add_ps(vecA, scaledDifference);

            return Vector2<float>(
                _mm_cvtss_f32(result),
                _mm_cvtss_f32(
                    _mm_shuffle_ps(
                        result,
                        result,
                        _MM_SHUFFLE(1, 1, 1, 1)
                    )
                )
            );
        }
    }
}