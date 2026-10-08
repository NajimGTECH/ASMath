#pragma once
#include <xmmintrin.h> // SSE  : __m128, _mm_*_ps
#include <emmintrin.h> // SSE2

/*
 * Helpers internes (non exposes dans include/) partages par Vector3Simd.cpp et Mat4Simd.cpp.
 *
 * Probleme : un tableau de Vector3<float> (AoS) est stocke en memoire comme
 *     x0 y0 z0 x1 y1 z1 x2 y2 z2 x3 y3 z3 ...
 * Un registre SSE contient 4 floats (4 "lanes"). Pour calculer 4 resultats a la fois,
 * on veut une lane par vecteur :
 *     X = x0 x1 x2 x3   Y = y0 y1 y2 y3   Z = z0 z1 z2 z3
 *
 * 4 Vector3 = 12 floats = 48 octets = exactement 3 registres de 16 octets.
 * On les charge avec 3 _mm_loadu_ps (aucune lecture au-dela des 4 vecteurs),
 * puis on les reorganise avec des _mm_shuffle_ps ("transposition" dans les registres).
 *
 * Rappel _mm_shuffle_ps(a, b, _MM_SHUFFLE(d, c, b_, a_)) :
 *     resultat = { a[a_], a[b_], b[c], b[d] }
 *   les 2 lanes basses viennent du 1er operande, les 2 hautes du 2nd.
 */
namespace math::simd::detail
{
    /**
     * @brief Charge 4 Vector3<float> consecutifs (AoS) et les transpose en SoA.
     * @param p Pointeur sur le x du premier vecteur. 12 floats lisibles. Aucun alignement requis.
     */
    inline void LoadTranspose4(const float* p, __m128& x, __m128& y, __m128& z)
    {
        const __m128 m0 = _mm_loadu_ps(p + 0); // x0 y0 z0 x1
        const __m128 m1 = _mm_loadu_ps(p + 4); // y1 z1 x2 y2
        const __m128 m2 = _mm_loadu_ps(p + 8); // z2 x3 y3 z3

        const __m128 t0 = _mm_shuffle_ps(m1, m2, _MM_SHUFFLE(2, 1, 3, 2)); // x2 y2 x3 y3
        const __m128 t1 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(1, 0, 2, 1)); // y0 z0 y1 z1

        x = _mm_shuffle_ps(m0, t0, _MM_SHUFFLE(2, 0, 3, 0)); // x0 x1 x2 x3
        y = _mm_shuffle_ps(t1, t0, _MM_SHUFFLE(3, 1, 2, 0)); // y0 y1 y2 y3
        z = _mm_shuffle_ps(t1, m2, _MM_SHUFFLE(3, 0, 3, 1)); // z0 z1 z2 z3
    }

    /**
     * @brief Operation inverse : transpose 3 registres SoA et ecrit 4 Vector3<float> consecutifs (AoS).
     * @param p Pointeur sur le x du premier vecteur. 12 floats inscriptibles. Aucun alignement requis.
     */
    inline void TransposeStore4(float* p, __m128 x, __m128 y, __m128 z)
    {
        const __m128 xy01 = _mm_unpacklo_ps(x, y);                            // x0 y0 x1 y1
        const __m128 xy23 = _mm_unpackhi_ps(x, y);                            // x2 y2 x3 y3

        const __m128 a  = _mm_shuffle_ps(z, xy01, _MM_SHUFFLE(2, 2, 0, 0));   // z0 z0 x1 x1
        const __m128 m0 = _mm_shuffle_ps(xy01, a, _MM_SHUFFLE(2, 0, 1, 0));   // x0 y0 z0 x1

        const __m128 c  = _mm_shuffle_ps(xy01, z, _MM_SHUFFLE(1, 1, 3, 3));   // y1 y1 z1 z1
        const __m128 m1 = _mm_shuffle_ps(c, xy23, _MM_SHUFFLE(1, 0, 2, 0));   // y1 z1 x2 y2

        const __m128 b  = _mm_shuffle_ps(z, xy23, _MM_SHUFFLE(3, 2, 3, 2));   // z2 z3 x3 y3
        const __m128 m2 = _mm_shuffle_ps(b, b, _MM_SHUFFLE(1, 3, 2, 0));      // z2 x3 y3 z3

        _mm_storeu_ps(p + 0, m0);
        _mm_storeu_ps(p + 4, m1);
        _mm_storeu_ps(p + 8, m2);
    }
}
