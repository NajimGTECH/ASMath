#pragma once
#include <xmmintrin.h> // SSE  : __m128 and the _mm_*_ps intrinsics
#include <emmintrin.h> // SSE2

/**
 * @file SseHelpers.h
 * @brief Internal helpers shared by Vector3Batch.cpp and Mat4Batch.cpp (not part of the public API).
 *
 * The problem these helpers solve
 * -------------------------------
 * An array of Vector3<float> (AoS = Array of Structures) is stored like this in memory:
 *
 *     x0 y0 z0 x1 y1 z1 x2 y2 z2 x3 y3 z3 x4 ...
 *
 * An SSE register (__m128) holds 4 floats, called "lanes" (lane 0 to lane 3).
 * To compute 4 results at the same time, we want one vector per lane:
 *
 *     X = [x0 x1 x2 x3]    Y = [y0 y1 y2 y3]    Z = [z0 z1 z2 z3]
 *
 * Then a single _mm_mul_ps(X, X) computes x0*x0, x1*x1, x2*x2 and x3*x3 in one instruction.
 *
 * 4 vectors = 12 floats = 48 bytes = exactly 3 SSE registers. So we load 3 registers
 * (never more than the 4 vectors, so no out-of-bounds read) and reorganize them with shuffles.
 * This "transposition" is the price to pay for the AoS layout (the SoA layout does not need it).
 *
 * Reminder: _mm_shuffle_ps(a, b, _MM_SHUFFLE(i3, i2, i1, i0)) returns
 *     [ a[i0], a[i1], b[i2], b[i3] ]
 * The two low lanes come from the first operand, the two high lanes from the second one.
 * (_MM_SHUFFLE takes its indices from the highest lane to the lowest lane.)
 */
namespace math::simd::detail
{
    /**
     * @brief Loads 4 consecutive Vector3<float> (AoS) and transposes them into X, Y, Z registers.
     * @param p Address of x0. 12 floats must be readable from p. No alignment required.
     */
    inline void LoadTranspose4(const float* p, __m128& x, __m128& y, __m128& z)
    {
        const __m128 r0 = _mm_loadu_ps(p + 0); // [x0 y0 z0 x1]
        const __m128 r1 = _mm_loadu_ps(p + 4); // [y1 z1 x2 y2]
        const __m128 r2 = _mm_loadu_ps(p + 8); // [z2 x3 y3 z3]

        const __m128 xy23 = _mm_shuffle_ps(r1, r2, _MM_SHUFFLE(2, 1, 3, 2)); // [x2 y2 x3 y3]
        const __m128 yz01 = _mm_shuffle_ps(r0, r1, _MM_SHUFFLE(1, 0, 2, 1)); // [y0 z0 y1 z1]

        x = _mm_shuffle_ps(r0, xy23, _MM_SHUFFLE(2, 0, 3, 0));   // [x0 x1 x2 x3]
        y = _mm_shuffle_ps(yz01, xy23, _MM_SHUFFLE(3, 1, 2, 0)); // [y0 y1 y2 y3]
        z = _mm_shuffle_ps(yz01, r2, _MM_SHUFFLE(3, 0, 3, 1));   // [z0 z1 z2 z3]
    }

    /**
     * @brief Inverse operation: transposes X, Y, Z registers and stores 4 consecutive Vector3<float>.
     * @param p Address of x0. 12 floats must be writable from p. No alignment required.
     */
    inline void StoreTranspose4(float* p, __m128 x, __m128 y, __m128 z)
    {
        const __m128 xy01 = _mm_unpacklo_ps(x, y); // [x0 y0 x1 y1]
        const __m128 xy23 = _mm_unpackhi_ps(x, y); // [x2 y2 x3 y3]

        // r0 = [x0 y0 z0 x1]
        const __m128 z0x1 = _mm_shuffle_ps(z, x, _MM_SHUFFLE(1, 1, 0, 0));    // [z0 z0 x1 x1]
        const __m128 r0 = _mm_shuffle_ps(xy01, z0x1, _MM_SHUFFLE(2, 0, 1, 0)); // [x0 y0 z0 x1]

        // r1 = [y1 z1 x2 y2]
        const __m128 y1z1 = _mm_shuffle_ps(y, z, _MM_SHUFFLE(1, 1, 1, 1));    // [y1 y1 z1 z1]
        const __m128 r1 = _mm_shuffle_ps(y1z1, xy23, _MM_SHUFFLE(1, 0, 2, 0)); // [y1 z1 x2 y2]

        // r2 = [z2 x3 y3 z3]
        const __m128 x3z2 = _mm_shuffle_ps(xy23, z, _MM_SHUFFLE(3, 2, 3, 2));  // [x3 y3 z2 z3]
        const __m128 r2 = _mm_shuffle_ps(x3z2, x3z2, _MM_SHUFFLE(3, 1, 0, 2)); // [z2 x3 y3 z3]

        _mm_storeu_ps(p + 0, r0);
        _mm_storeu_ps(p + 4, r1);
        _mm_storeu_ps(p + 8, r2);
    }
}
