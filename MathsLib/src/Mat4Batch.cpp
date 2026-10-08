#include "Mat4Batch.h"
#include "SseHelpers.h"

#include <cassert>

namespace math
{
    // ================================================================================
    // C++ reference
    // ================================================================================
    namespace ref
    {
        void TransformPointsBatch(const Mat4f& m, const Vec3f* in, Vec3f* out, std::size_t n)
        {
            // Local copy of the matrix: `out` points to floats, and so does `m`. Without the copy the
            // compiler must assume that writing out[i] may modify m (aliasing), and it would reload
            // the 12 coefficients from memory at every iteration. The copy makes the reference a
            // fair "optimized C++" baseline.
            const Mat4f local = m;
            for (std::size_t i = 0; i < n; ++i)
                out[i] = local.MultiplyPointAffine(in[i]);
        }

        void TransformPointsBatchSoA(const Mat4f& m, const Vec3SoA& in, Vec3SoA& out, std::size_t n)
        {
            assert(in.Size() >= n && out.Size() >= n);
            const Mat4f local = m;
            const float* ix = in.x.data();
            const float* iy = in.y.data();
            const float* iz = in.z.data();
            float* ox = out.x.data();
            float* oy = out.y.data();
            float* oz = out.z.data();
            for (std::size_t i = 0; i < n; ++i)
            {
                const Vec3f p = local.MultiplyPointAffine(Vec3f(ix[i], iy[i], iz[i]));
                ox[i] = p.x;
                oy[i] = p.y;
                oz[i] = p.z;
            }
        }

        void MultiplyBatch(const Mat4f* a, const Mat4f* b, Mat4f* out, std::size_t n)
        {
            for (std::size_t i = 0; i < n; ++i)
                out[i] = a[i] * b[i];
        }
    }

    // ================================================================================
    // Explicit SIMD (SSE/SSE2)
    // ================================================================================
    namespace simd
    {
        namespace
        {
            const float* AsFloats(const Vec3f* p) { return reinterpret_cast<const float*>(p); }
            float* AsFloats(Vec3f* p) { return reinterpret_cast<float*>(p); }

            /** ((x*c0 + y*c1) + z*c2) + c3 for the 4 lanes: same order as Mat4::MultiplyPointAffine. */
            __m128 Affine4(__m128 x, __m128 y, __m128 z, __m128 c0, __m128 c1, __m128 c2, __m128 c3)
            {
                const __m128 sum = _mm_add_ps(_mm_mul_ps(x, c0), _mm_mul_ps(y, c1));
                return _mm_add_ps(_mm_add_ps(sum, _mm_mul_ps(z, c2)), c3);
            }

            /**
             * The 12 useful coefficients of an affine matrix, each one broadcast in the 4 lanes.
             * Example: m10 = [m[1][0] m[1][0] m[1][0] m[1][0]].
             * Used by the "one lane = one point" versions (AoS 4 points and SoA).
             */
            struct BroadcastMatrix
            {
                __m128 m00, m01, m02;
                __m128 m10, m11, m12;
                __m128 m20, m21, m22;
                __m128 m30, m31, m32;

                explicit BroadcastMatrix(const Mat4f& m)
                {
                    m00 = _mm_set1_ps(m.m[0][0]); m01 = _mm_set1_ps(m.m[0][1]); m02 = _mm_set1_ps(m.m[0][2]);
                    m10 = _mm_set1_ps(m.m[1][0]); m11 = _mm_set1_ps(m.m[1][1]); m12 = _mm_set1_ps(m.m[1][2]);
                    m20 = _mm_set1_ps(m.m[2][0]); m21 = _mm_set1_ps(m.m[2][1]); m22 = _mm_set1_ps(m.m[2][2]);
                    m30 = _mm_set1_ps(m.m[3][0]); m31 = _mm_set1_ps(m.m[3][1]); m32 = _mm_set1_ps(m.m[3][2]);
                }

                /** Transforms 4 points (lane k = point k) in place. */
                void Transform4(__m128& x, __m128& y, __m128& z) const
                {
                    const __m128 newX = Affine4(x, y, z, m00, m10, m20, m30); // column 0
                    const __m128 newY = Affine4(x, y, z, m01, m11, m21, m31); // column 1
                    const __m128 newZ = Affine4(x, y, z, m02, m12, m22, m32); // column 2
                    x = newX;
                    y = newY;
                    z = newZ;
                }
            };
        }

        void TransformPointsBatch(const Mat4f& m, const Vec3f* in, Vec3f* out, std::size_t n)
        {
            const Mat4f local = m; // same aliasing reason as in the reference
            const BroadcastMatrix bm(local);
            const float* pin = AsFloats(in);
            float* pout = AsFloats(out);

            std::size_t i = 0;
            for (; i + 4 <= n; i += 4)
            {
                __m128 x, y, z;
                detail::LoadTranspose4(pin + 3 * i, x, y, z); // read everything before writing: in == out OK
                bm.Transform4(x, y, z);
                detail::StoreTranspose4(pout + 3 * i, x, y, z);
            }

            for (; i < n; ++i)
                out[i] = local.MultiplyPointAffine(in[i]);
        }

        void TransformPointsBatchNaive(const Mat4f& m, const Vec3f* in, Vec3f* out, std::size_t n)
        {
            // One row of the matrix = one register: row0 = [m00 m01 m02 m03], ..., row3 = [m30 m31 m32 m33].
            const __m128 row0 = _mm_loadu_ps(m.m[0]);
            const __m128 row1 = _mm_loadu_ps(m.m[1]);
            const __m128 row2 = _mm_loadu_ps(m.m[2]);
            const __m128 row3 = _mm_loadu_ps(m.m[3]);

            for (std::size_t i = 0; i < n; ++i)
            {
                // x, y, z are read one by one (12 bytes): a 16-byte load on the last point would
                // read 4 bytes past the end of the array.
                const Vec3f p = in[i];
                const __m128 x = _mm_set1_ps(p.x);
                const __m128 y = _mm_set1_ps(p.y);
                const __m128 z = _mm_set1_ps(p.z);
                const __m128 result = Affine4(x, y, z, row0, row1, row2, row3); // [x' y' z' w']

                // Store the 4 lanes in a small local array, then copy only x', y', z' (w' is dropped).
                float tmp[4];
                _mm_storeu_ps(tmp, result);
                out[i] = Vec3f(tmp[0], tmp[1], tmp[2]);
            }
        }

        void TransformPointsBatchSoA(const Mat4f& m, const Vec3SoA& in, Vec3SoA& out, std::size_t n)
        {
            assert(in.Size() >= n && out.Size() >= n);
            const Mat4f local = m;
            const BroadcastMatrix bm(local);
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
                bm.Transform4(x, y, z);
                _mm_storeu_ps(ox + i, x);
                _mm_storeu_ps(oy + i, y);
                _mm_storeu_ps(oz + i, z);
            }

            for (; i < n; ++i)
            {
                const Vec3f p = local.MultiplyPointAffine(Vec3f(ix[i], iy[i], iz[i]));
                ox[i] = p.x;
                oy[i] = p.y;
                oz[i] = p.z;
            }
        }

        Mat4f Multiply(const Mat4f& a, const Mat4f& b)
        {
            // The 4 rows of b, one register each.
            const __m128 b0 = _mm_loadu_ps(b.m[0]);
            const __m128 b1 = _mm_loadu_ps(b.m[1]);
            const __m128 b2 = _mm_loadu_ps(b.m[2]);
            const __m128 b3 = _mm_loadu_ps(b.m[3]);

            Mat4f result; // local result: out[i] = Multiply(a[i], b[i]) also works when out == a or out == b
            for (int i = 0; i < 4; ++i)
            {
                // Row i of the result = a[i][0]*b0 + a[i][1]*b1 + a[i][2]*b2 + a[i][3]*b3
                __m128 row = _mm_mul_ps(_mm_set1_ps(a.m[i][0]), b0);
                row = _mm_add_ps(row, _mm_mul_ps(_mm_set1_ps(a.m[i][1]), b1));
                row = _mm_add_ps(row, _mm_mul_ps(_mm_set1_ps(a.m[i][2]), b2));
                row = _mm_add_ps(row, _mm_mul_ps(_mm_set1_ps(a.m[i][3]), b3));
                _mm_storeu_ps(result.m[i], row);
            }
            return result;
        }

        void MultiplyBatch(const Mat4f* a, const Mat4f* b, Mat4f* out, std::size_t n)
        {
            for (std::size_t i = 0; i < n; ++i)
                out[i] = Multiply(a[i], b[i]);
        }
    }
}
