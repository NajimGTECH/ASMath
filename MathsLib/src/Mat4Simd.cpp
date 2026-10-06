#include "Mat4Simd.h"
#include "SimdTranspose.h"

namespace math
{
    // =====================================================================
    // Reference C++
    // =====================================================================
    namespace ref
    {
        void TransformPointsBatch(const Mat4f& m, const Vec3f* in, Vec3f* out, std::size_t n)
        {
            // Copie locale : out (float*) pourrait sinon pointer dans m, ce qui obligerait le
            // compilateur a relire les 12 coefficients a chaque iteration. La copie rend la
            // reference "optimisee" et la comparaison avec le SIMD plus honnete.
            const Mat4f local = m;
            for (std::size_t i = 0; i < n; ++i)
                out[i] = local.MultiplyPointAffine(in[i]);
        }

        void TransformPointsBatchSoA(const Mat4f& m, const Vec3SoA& in, Vec3SoA& out, std::size_t n)
        {
            const float m00 = m.m[0][0], m01 = m.m[0][1], m02 = m.m[0][2];
            const float m10 = m.m[1][0], m11 = m.m[1][1], m12 = m.m[1][2];
            const float m20 = m.m[2][0], m21 = m.m[2][1], m22 = m.m[2][2];
            const float m30 = m.m[3][0], m31 = m.m[3][1], m32 = m.m[3][2];

            const float* ix = in.x.data(); const float* iy = in.y.data(); const float* iz = in.z.data();
            float* ox = out.x.data(); float* oy = out.y.data(); float* oz = out.z.data();
            for (std::size_t i = 0; i < n; ++i)
            {
                const float x = ix[i], y = iy[i], z = iz[i];
                ox[i] = x * m00 + y * m10 + z * m20 + m30;
                oy[i] = x * m01 + y * m11 + z * m21 + m31;
                oz[i] = x * m02 + y * m12 + z * m22 + m32;
            }
        }

        void MultiplyBatch(const Mat4f* a, const Mat4f* b, Mat4f* out, std::size_t n)
        {
            for (std::size_t i = 0; i < n; ++i)
                out[i] = a[i] * b[i];
        }
    }

    // =====================================================================
    // SIMD explicite SSE/SSE2
    // =====================================================================
    namespace simd
    {
        static const float* AsFloats(const Vec3f* p) { return reinterpret_cast<const float*>(p); }
        static float* AsFloats(Vec3f* p) { return reinterpret_cast<float*>(p); }

        // ((x*c0 + y*c1) + z*c2) + c3 : meme ordre que Mat4::MultiplyPointAffine.
        static __m128 Affine4(__m128 x, __m128 y, __m128 z, __m128 c0, __m128 c1, __m128 c2, __m128 c3)
        {
            return _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(x, c0), _mm_mul_ps(y, c1)), _mm_mul_ps(z, c2)), c3);
        }

        // Coefficients de la matrice diffuses dans les 4 lanes (_mm_set1_ps) pour le mode "une lane = un point".
        struct BroadcastAffine
        {
            __m128 m00, m01, m02, m10, m11, m12, m20, m21, m22, m30, m31, m32;

            explicit BroadcastAffine(const Mat4f& m)
                : m00(_mm_set1_ps(m.m[0][0])), m01(_mm_set1_ps(m.m[0][1])), m02(_mm_set1_ps(m.m[0][2]))
                , m10(_mm_set1_ps(m.m[1][0])), m11(_mm_set1_ps(m.m[1][1])), m12(_mm_set1_ps(m.m[1][2]))
                , m20(_mm_set1_ps(m.m[2][0])), m21(_mm_set1_ps(m.m[2][1])), m22(_mm_set1_ps(m.m[2][2]))
                , m30(_mm_set1_ps(m.m[3][0])), m31(_mm_set1_ps(m.m[3][1])), m32(_mm_set1_ps(m.m[3][2]))
            {
            }

            void Apply(__m128& x, __m128& y, __m128& z) const
            {
                const __m128 nx = Affine4(x, y, z, m00, m10, m20, m30);
                const __m128 ny = Affine4(x, y, z, m01, m11, m21, m31);
                const __m128 nz = Affine4(x, y, z, m02, m12, m22, m32);
                x = nx; y = ny; z = nz;
            }
        };

        void TransformPointsBatch(const Mat4f& m, const Vec3f* in, Vec3f* out, std::size_t n)
        {
            const Mat4f local = m; // meme raison que dans la reference (pas d'alias avec out)
            const BroadcastAffine bm(local);
            const float* pin = AsFloats(in);
            float* pout = AsFloats(out);
            const std::size_t n4 = n & ~static_cast<std::size_t>(3);

            std::size_t i = 0;
            for (; i < n4; i += 4)
            {
                __m128 x, y, z;
                detail::LoadTranspose4(pin + 3 * i, x, y, z);
                bm.Apply(x, y, z);
                detail::TransposeStore4(pout + 3 * i, x, y, z);
            }
            for (; i < n; ++i)
                out[i] = local.MultiplyPointAffine(in[i]);
        }

        void TransformPointsBatchPerPoint(const Mat4f& m, const Vec3f* in, Vec3f* out, std::size_t n)
        {
            // Une ligne de la matrice = un registre : L0 = (m00 m01 m02 m03), ..., L3 = (m30 m31 m32 m33).
            const __m128 l0 = _mm_loadu_ps(m.m[0]);
            const __m128 l1 = _mm_loadu_ps(m.m[1]);
            const __m128 l2 = _mm_loadu_ps(m.m[2]);
            const __m128 l3 = _mm_loadu_ps(m.m[3]);

            for (std::size_t i = 0; i < n; ++i)
            {
                // Lecture scalaire de x, y, z (12 octets) puis diffusion : pas de lecture de 16 octets
                // qui deborderait apres le dernier point.
                const __m128 x = _mm_set1_ps(in[i].x);
                const __m128 y = _mm_set1_ps(in[i].y);
                const __m128 z = _mm_set1_ps(in[i].z);
                const __m128 r = Affine4(x, y, z, l0, l1, l2, l3); // x' y' z' w'

                // Ecriture de 12 octets seulement : x' y' (64 bits bas) puis z'. La lane w' est jetee.
                _mm_storel_epi64(reinterpret_cast<__m128i*>(&out[i].x), _mm_castps_si128(r));
                _mm_store_ss(&out[i].z, _mm_movehl_ps(r, r));
            }
        }

        void TransformPointsBatchSoA(const Mat4f& m, const Vec3SoA& in, Vec3SoA& out, std::size_t n)
        {
            const BroadcastAffine bm(m);
            const float* ix = in.x.data(); const float* iy = in.y.data(); const float* iz = in.z.data();
            float* ox = out.x.data(); float* oy = out.y.data(); float* oz = out.z.data();
            const std::size_t n4 = n & ~static_cast<std::size_t>(3);

            std::size_t i = 0;
            for (; i < n4; i += 4)
            {
                __m128 x = _mm_loadu_ps(ix + i);
                __m128 y = _mm_loadu_ps(iy + i);
                __m128 z = _mm_loadu_ps(iz + i);
                bm.Apply(x, y, z);
                _mm_storeu_ps(ox + i, x);
                _mm_storeu_ps(oy + i, y);
                _mm_storeu_ps(oz + i, z);
            }
            for (; i < n; ++i)
            {
                const Vec3f p = m.MultiplyPointAffine(Vec3f(ix[i], iy[i], iz[i]));
                ox[i] = p.x; oy[i] = p.y; oz[i] = p.z;
            }
        }

        // r = a * b sur 16 floats row-major. r peut etre egal a a ou b.
        static void MultiplyRaw(const float* a, const float* b, float* r)
        {
            // Toutes les lignes de b sont chargees avant la moindre ecriture (r == b autorise).
            const __m128 b0 = _mm_loadu_ps(b + 0);
            const __m128 b1 = _mm_loadu_ps(b + 4);
            const __m128 b2 = _mm_loadu_ps(b + 8);
            const __m128 b3 = _mm_loadu_ps(b + 12);

            for (int i = 0; i < 4; ++i)
            {
                // La ligne i de a est lue avant l'ecriture de la ligne i de r (r == a autorise).
                const __m128 ai = _mm_loadu_ps(a + 4 * i);
                __m128 row = _mm_mul_ps(_mm_shuffle_ps(ai, ai, _MM_SHUFFLE(0, 0, 0, 0)), b0);
                row = _mm_add_ps(row, _mm_mul_ps(_mm_shuffle_ps(ai, ai, _MM_SHUFFLE(1, 1, 1, 1)), b1));
                row = _mm_add_ps(row, _mm_mul_ps(_mm_shuffle_ps(ai, ai, _MM_SHUFFLE(2, 2, 2, 2)), b2));
                row = _mm_add_ps(row, _mm_mul_ps(_mm_shuffle_ps(ai, ai, _MM_SHUFFLE(3, 3, 3, 3)), b3));
                _mm_storeu_ps(r + 4 * i, row);
            }
        }

        Mat4f Multiply(const Mat4f& a, const Mat4f& b)
        {
            Mat4f result;
            MultiplyRaw(&a.m[0][0], &b.m[0][0], &result.m[0][0]);
            return result;
        }

        void MultiplyBatch(const Mat4f* a, const Mat4f* b, Mat4f* out, std::size_t n)
        {
            for (std::size_t i = 0; i < n; ++i)
                MultiplyRaw(&a[i].m[0][0], &b[i].m[0][0], &out[i].m[0][0]);
        }
    }
}
