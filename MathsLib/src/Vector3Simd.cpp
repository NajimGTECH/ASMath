#include "Vector3Simd.h"
#include "SimdTranspose.h"

namespace math
{
    // =====================================================================
    // Reference C++ : on reutilise l'API existante de Vector3 (Dot, Normalized).
    // Compile en /O2 : le compilateur a le droit d'auto-vectoriser ces boucles.
    // =====================================================================
    namespace ref
    {
        void DotBatch(const Vec3f* a, const Vec3f* b, float* out, std::size_t n)
        {
            for (std::size_t i = 0; i < n; ++i)
                out[i] = a[i].Dot(b[i]);
        }

        void NormalizeBatch(const Vec3f* in, Vec3f* out, std::size_t n)
        {
            for (std::size_t i = 0; i < n; ++i)
                out[i] = in[i].Normalized();
        }

        void DotBatchSoA(const Vec3SoA& a, const Vec3SoA& b, float* out, std::size_t n)
        {
            const float* ax = a.x.data(); const float* ay = a.y.data(); const float* az = a.z.data();
            const float* bx = b.x.data(); const float* by = b.y.data(); const float* bz = b.z.data();
            for (std::size_t i = 0; i < n; ++i)
                out[i] = ax[i] * bx[i] + ay[i] * by[i] + az[i] * bz[i];
        }

        void NormalizeBatchSoA(const Vec3SoA& in, Vec3SoA& out, std::size_t n)
        {
            const float* ix = in.x.data(); const float* iy = in.y.data(); const float* iz = in.z.data();
            float* ox = out.x.data(); float* oy = out.y.data(); float* oz = out.z.data();
            for (std::size_t i = 0; i < n; ++i)
            {
                const float x = ix[i], y = iy[i], z = iz[i];
                const float len = std::sqrt(x * x + y * y + z * z);
                if (len > 0.0f)
                {
                    ox[i] = x / len; oy[i] = y / len; oz[i] = z / len;
                }
                else
                {
                    ox[i] = 0.0f; oy[i] = 0.0f; oz[i] = 0.0f;
                }
            }
        }

        void AoSToSoA(const Vec3f* in, Vec3SoA& out, std::size_t n)
        {
            float* ox = out.x.data(); float* oy = out.y.data(); float* oz = out.z.data();
            for (std::size_t i = 0; i < n; ++i)
            {
                ox[i] = in[i].x; oy[i] = in[i].y; oz[i] = in[i].z;
            }
        }

        void SoAToAoS(const Vec3SoA& in, Vec3f* out, std::size_t n)
        {
            const float* ix = in.x.data(); const float* iy = in.y.data(); const float* iz = in.z.data();
            for (std::size_t i = 0; i < n; ++i)
                out[i] = Vec3f(ix[i], iy[i], iz[i]);
        }
    }

    // =====================================================================
    // SIMD explicite SSE/SSE2.
    // Schema commun : boucle principale par blocs de 4 (n4 = n arrondi au multiple de 4 inferieur),
    // puis boucle de reste scalaire identique a la reference pour les n % 4 derniers elements.
    // =====================================================================
    namespace simd
    {
        // Un tableau de Vector3<float> est vu comme un tableau de 3*n floats (static_assert dans le .h).
        static const float* AsFloats(const Vec3f* p) { return reinterpret_cast<const float*>(p); }
        static float* AsFloats(Vec3f* p) { return reinterpret_cast<float*>(p); }

        // (x*x + y*y) + z*z : meme ordre d'evaluation que Vector3::Dot / Length.
        static __m128 Dot3(__m128 ax, __m128 ay, __m128 az, __m128 bx, __m128 by, __m128 bz)
        {
            return _mm_add_ps(_mm_add_ps(_mm_mul_ps(ax, bx), _mm_mul_ps(ay, by)), _mm_mul_ps(az, bz));
        }

        // Normalise 4 vecteurs (une lane par vecteur). Sans branche :
        //   len  = sqrt(x*x + y*y + z*z)
        //   mask = (len > 0) -> 0xFFFFFFFF ou 0 par lane (faux aussi pour NaN)
        //   x    = mask & (x / len)  -> 0 si vecteur nul (0/0 = NaN est efface par le masque)
        static void Normalize4(__m128& x, __m128& y, __m128& z)
        {
            const __m128 len  = _mm_sqrt_ps(Dot3(x, y, z, x, y, z));
            const __m128 mask = _mm_cmpgt_ps(len, _mm_setzero_ps());
            x = _mm_and_ps(mask, _mm_div_ps(x, len));
            y = _mm_and_ps(mask, _mm_div_ps(y, len));
            z = _mm_and_ps(mask, _mm_div_ps(z, len));
        }

        void DotBatch(const Vec3f* a, const Vec3f* b, float* out, std::size_t n)
        {
            const float* pa = AsFloats(a);
            const float* pb = AsFloats(b);
            const std::size_t n4 = n & ~static_cast<std::size_t>(3);

            std::size_t i = 0;
            for (; i < n4; i += 4)
            {
                __m128 ax, ay, az, bx, by, bz;
                detail::LoadTranspose4(pa + 3 * i, ax, ay, az);
                detail::LoadTranspose4(pb + 3 * i, bx, by, bz);
                _mm_storeu_ps(out + i, Dot3(ax, ay, az, bx, by, bz)); // 4 resultats contigus
            }
            for (; i < n; ++i)
                out[i] = a[i].Dot(b[i]);
        }

        void NormalizeBatch(const Vec3f* in, Vec3f* out, std::size_t n)
        {
            const float* pin = AsFloats(in);
            float* pout = AsFloats(out);
            const std::size_t n4 = n & ~static_cast<std::size_t>(3);

            std::size_t i = 0;
            for (; i < n4; i += 4)
            {
                __m128 x, y, z;
                detail::LoadTranspose4(pin + 3 * i, x, y, z); // tout est lu avant d'ecrire : en place OK
                Normalize4(x, y, z);
                detail::TransposeStore4(pout + 3 * i, x, y, z);
            }
            for (; i < n; ++i)
                out[i] = in[i].Normalized();
        }

        void DotBatchSoA(const Vec3SoA& a, const Vec3SoA& b, float* out, std::size_t n)
        {
            const float* ax = a.x.data(); const float* ay = a.y.data(); const float* az = a.z.data();
            const float* bx = b.x.data(); const float* by = b.y.data(); const float* bz = b.z.data();
            const std::size_t n4 = n & ~static_cast<std::size_t>(3);

            std::size_t i = 0;
            for (; i < n4; i += 4)
            {
                const __m128 d = Dot3(_mm_loadu_ps(ax + i), _mm_loadu_ps(ay + i), _mm_loadu_ps(az + i),
                                      _mm_loadu_ps(bx + i), _mm_loadu_ps(by + i), _mm_loadu_ps(bz + i));
                _mm_storeu_ps(out + i, d);
            }
            for (; i < n; ++i)
                out[i] = ax[i] * bx[i] + ay[i] * by[i] + az[i] * bz[i];
        }

        void NormalizeBatchSoA(const Vec3SoA& in, Vec3SoA& out, std::size_t n)
        {
            const float* ix = in.x.data(); const float* iy = in.y.data(); const float* iz = in.z.data();
            float* ox = out.x.data(); float* oy = out.y.data(); float* oz = out.z.data();
            const std::size_t n4 = n & ~static_cast<std::size_t>(3);

            std::size_t i = 0;
            for (; i < n4; i += 4)
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
                ox[i] = v.x; oy[i] = v.y; oz[i] = v.z;
            }
        }

        void AoSToSoA(const Vec3f* in, Vec3SoA& out, std::size_t n)
        {
            const float* pin = AsFloats(in);
            float* ox = out.x.data(); float* oy = out.y.data(); float* oz = out.z.data();
            const std::size_t n4 = n & ~static_cast<std::size_t>(3);

            std::size_t i = 0;
            for (; i < n4; i += 4)
            {
                __m128 x, y, z;
                detail::LoadTranspose4(pin + 3 * i, x, y, z);
                _mm_storeu_ps(ox + i, x);
                _mm_storeu_ps(oy + i, y);
                _mm_storeu_ps(oz + i, z);
            }
            for (; i < n; ++i)
            {
                ox[i] = in[i].x; oy[i] = in[i].y; oz[i] = in[i].z;
            }
        }

        void SoAToAoS(const Vec3SoA& in, Vec3f* out, std::size_t n)
        {
            const float* ix = in.x.data(); const float* iy = in.y.data(); const float* iz = in.z.data();
            float* pout = AsFloats(out);
            const std::size_t n4 = n & ~static_cast<std::size_t>(3);

            std::size_t i = 0;
            for (; i < n4; i += 4)
                detail::TransposeStore4(pout + 3 * i, _mm_loadu_ps(ix + i), _mm_loadu_ps(iy + i), _mm_loadu_ps(iz + i));
            for (; i < n; ++i)
                out[i] = Vec3f(ix[i], iy[i], iz[i]);
        }
    }
}
