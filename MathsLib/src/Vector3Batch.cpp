#include "Vector3Batch.h"
#include "SseHelpers.h"

#include <cassert>
#include <cfloat>

namespace math
{
    // ================================================================================
    // Layout conversions
    // ================================================================================

    void AoSToSoA(const Vec3f* in, Vec3SoA& out, std::size_t n)
    {
        assert(out.Size() >= n);
        float* ox = out.x.data();
        float* oy = out.y.data();
        float* oz = out.z.data();
        for (std::size_t i = 0; i < n; ++i)
        {
            ox[i] = in[i].x;
            oy[i] = in[i].y;
            oz[i] = in[i].z;
        }
    }

    void SoAToAoS(const Vec3SoA& in, Vec3f* out, std::size_t n)
    {
        assert(in.Size() >= n);
        const float* ix = in.x.data();
        const float* iy = in.y.data();
        const float* iz = in.z.data();
        for (std::size_t i = 0; i < n; ++i)
            out[i] = Vec3f(ix[i], iy[i], iz[i]);
    }

    // ================================================================================
    // C++ reference: reuses the existing Vector3 API.
    // ================================================================================
    namespace ref
    {
        void DotBatch(const Vec3f* a, const Vec3f* b, float* out, std::size_t n)
        {
            for (std::size_t i = 0; i < n; ++i)
                out[i] = a[i].Dot(b[i]);
        }

        void DotBatchSoA(const Vec3SoA& a, const Vec3SoA& b, float* out, std::size_t n)
        {
            assert(a.Size() >= n && b.Size() >= n);
            // Raw pointers: simpler loop for the compiler (no std::vector bounds/size reloads).
            const float* ax = a.x.data();
            const float* ay = a.y.data();
            const float* az = a.z.data();
            const float* bx = b.x.data();
            const float* by = b.y.data();
            const float* bz = b.z.data();
            for (std::size_t i = 0; i < n; ++i)
                out[i] = ax[i] * bx[i] + ay[i] * by[i] + az[i] * bz[i]; // same order as Vector3::Dot
        }

        void NormalizeBatch(const Vec3f* in, Vec3f* out, std::size_t n)
        {
            for (std::size_t i = 0; i < n; ++i)
                out[i] = in[i].Normalized();
        }

        void NormalizeBatchSoA(const Vec3SoA& in, Vec3SoA& out, std::size_t n)
        {
            assert(in.Size() >= n && out.Size() >= n);
            const float* ix = in.x.data();
            const float* iy = in.y.data();
            const float* iz = in.z.data();
            float* ox = out.x.data();
            float* oy = out.y.data();
            float* oz = out.z.data();
            for (std::size_t i = 0; i < n; ++i)
            {
                const Vec3f v = Vec3f(ix[i], iy[i], iz[i]).Normalized();
                ox[i] = v.x;
                oy[i] = v.y;
                oz[i] = v.z;
            }
        }
    }

    // ================================================================================
    // Explicit SIMD (SSE/SSE2)
    //
    // Every function follows the same pattern:
    //   1. main loop: 4 elements per iteration, while i + 4 <= n (so we never go past the end);
    //   2. remainder loop: the last n % 4 elements (0 to 3) with the scalar reference code.
    // ================================================================================
    namespace simd
    {
        namespace
        {
            // An array of n Vec3f is seen as an array of 3*n floats (checked by static_assert in the .h).
            const float* AsFloats(const Vec3f* p) { return reinterpret_cast<const float*>(p); }
            float* AsFloats(Vec3f* p) { return reinterpret_cast<float*>(p); }

            /**
             * 4 dot products at once. Lane k: (ax[k]*bx[k] + ay[k]*by[k]) + az[k]*bz[k].
             * Same evaluation order as Vector3::Dot -> same rounding -> same result.
             */
            __m128 Dot4(__m128 ax, __m128 ay, __m128 az, __m128 bx, __m128 by, __m128 bz)
            {
                const __m128 xx = _mm_mul_ps(ax, bx);
                const __m128 yy = _mm_mul_ps(ay, by);
                const __m128 zz = _mm_mul_ps(az, bz);
                return _mm_add_ps(_mm_add_ps(xx, yy), zz);
            }

            /**
             * Normalizes 4 vectors at once (lane k = vector k), without any branch.
             *
             *   len  = sqrt(x*x + y*y + z*z)          (4 lengths)
             *   mask = (len > 0) ? 0xFFFFFFFF : 0     (per lane; false for NaN too)
             *   x    = mask AND (x / len)             (0/0 = NaN in a zero lane is erased by the mask)
             */
            void Normalize4(__m128& x, __m128& y, __m128& z)
            {
                const __m128 len = _mm_sqrt_ps(Dot4(x, y, z, x, y, z));
                const __m128 mask = _mm_cmpgt_ps(len, _mm_setzero_ps());
                x = _mm_and_ps(mask, _mm_div_ps(x, len));
                y = _mm_and_ps(mask, _mm_div_ps(y, len));
                z = _mm_and_ps(mask, _mm_div_ps(z, len));
            }
        }

        void DotBatch(const Vec3f* a, const Vec3f* b, float* out, std::size_t n)
        {
            const float* pa = AsFloats(a);
            const float* pb = AsFloats(b);

            std::size_t i = 0;
            for (; i + 4 <= n; i += 4)
            {
                __m128 ax, ay, az, bx, by, bz;
                detail::LoadTranspose4(pa + 3 * i, ax, ay, az); // vectors a[i..i+3]
                detail::LoadTranspose4(pb + 3 * i, bx, by, bz); // vectors b[i..i+3]
                _mm_storeu_ps(out + i, Dot4(ax, ay, az, bx, by, bz)); // out[i..i+3]
            }

            for (; i < n; ++i) // remainder: 0 to 3 vectors
                out[i] = a[i].Dot(b[i]);
        }

        void DotBatchSoA(const Vec3SoA& a, const Vec3SoA& b, float* out, std::size_t n)
        {
            assert(a.Size() >= n && b.Size() >= n);
            const float* ax = a.x.data();
            const float* ay = a.y.data();
            const float* az = a.z.data();
            const float* bx = b.x.data();
            const float* by = b.y.data();
            const float* bz = b.z.data();

            std::size_t i = 0;
            for (; i + 4 <= n; i += 4)
            {
                // SoA: the x of 4 consecutive vectors are already contiguous -> one load each.
                const __m128 d = Dot4(_mm_loadu_ps(ax + i), _mm_loadu_ps(ay + i), _mm_loadu_ps(az + i),
                                      _mm_loadu_ps(bx + i), _mm_loadu_ps(by + i), _mm_loadu_ps(bz + i));
                _mm_storeu_ps(out + i, d);
            }

            for (; i < n; ++i)
                out[i] = ax[i] * bx[i] + ay[i] * by[i] + az[i] * bz[i];
        }

        void NormalizeBatch(const Vec3f* in, Vec3f* out, std::size_t n)
        {
            const float* pin = AsFloats(in);
            float* pout = AsFloats(out);

            std::size_t i = 0;
            for (; i + 4 <= n; i += 4)
            {
                __m128 x, y, z;
                // The 12 floats are fully read before anything is written: in == out is OK.
                detail::LoadTranspose4(pin + 3 * i, x, y, z);
                Normalize4(x, y, z);
                detail::StoreTranspose4(pout + 3 * i, x, y, z);
            }

            for (; i < n; ++i)
                out[i] = in[i].Normalized();
        }

        void NormalizeBatchSoA(const Vec3SoA& in, Vec3SoA& out, std::size_t n)
        {
            assert(in.Size() >= n && out.Size() >= n);
            const float* ix = in.x.data();
            const float* iy = in.y.data();
            const float* iz = in.z.data();
            float* ox = out.x.data();
            float* oy = out.y.data();
            float* oz = out.z.data();

            std::size_t i = 0;
            for (; i + 4 <= n; i += 4)
            {
                __m128 x = _mm_loadu_ps(ix + i);
                __m128 y = _mm_loadu_ps(iy + i);
                __m128 z = _mm_loadu_ps(iz + i);
                Normalize4(x, y, z);
                _mm_storeu_ps(ox + i, x);
                _mm_storeu_ps(oy + i, y);
                _mm_storeu_ps(oz + i, z);
            }

            for (; i < n; ++i)
            {
                const Vec3f v = Vec3f(ix[i], iy[i], iz[i]).Normalized();
                ox[i] = v.x;
                oy[i] = v.y;
                oz[i] = v.z;
            }
        }

        void NormalizeBatchApprox(const Vec3f* in, Vec3f* out, std::size_t n)
        {
            const float* pin = AsFloats(in);
            float* pout = AsFloats(out);
            const __m128 minLenSq = _mm_set1_ps(FLT_MIN);

            std::size_t i = 0;
            for (; i + 4 <= n; i += 4)
            {
                __m128 x, y, z;
                detail::LoadTranspose4(pin + 3 * i, x, y, z);

                const __m128 lenSq = Dot4(x, y, z, x, y, z);
                const __m128 invLen = _mm_rsqrt_ps(lenSq);              // ~1/sqrt(lenSq), 12 bits
                const __m128 mask = _mm_cmpge_ps(lenSq, minLenSq);      // false for 0, denormals and NaN
                x = _mm_and_ps(mask, _mm_mul_ps(x, invLen));
                y = _mm_and_ps(mask, _mm_mul_ps(y, invLen));
                z = _mm_and_ps(mask, _mm_mul_ps(z, invLen));

                detail::StoreTranspose4(pout + 3 * i, x, y, z);
            }

            // Remainder: same approximation, one vector at a time (_ss = "scalar single", lane 0 only).
            for (; i < n; ++i)
            {
                const Vec3f v = in[i];
                const float lenSq = v.LengthSquared();
                if (lenSq >= FLT_MIN)
                {
                    const float invLen = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(lenSq)));
                    out[i] = v * invLen;
                }
                else
                {
                    out[i] = Vec3f(0.0f, 0.0f, 0.0f);
                }
            }
        }
    }
}
