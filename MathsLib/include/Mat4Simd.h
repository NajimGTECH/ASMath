#pragma once
#include <cstddef>
#include <type_traits>
#include "Mat4.h"
#include "Vector3Simd.h"

/**
 * @file Mat4Simd.h
 * @brief Traitements Mat4<float> : reference C++ (math::ref) et SIMD explicite SSE/SSE2 (math::simd).
 *
 * Conventions de la bibliotheque (a respecter par les deux versions) :
 *  - Stockage : row-major, m[ligne][colonne], 16 floats contigus. La ligne i (m[i][0..3]) est
 *    contigue et se charge en un seul _mm_loadu_ps.
 *  - Vecteurs lignes : p' = p * M. La translation est dans la ligne 3 (m[3][0..2]).
 *  - Composition : A * B applique A puis B (p * A * B).
 *  - Points : coordonnee homogene w = 1 implicite, pas de division perspective
 *    (voir Mat4::MultiplyPointAffine). La colonne 3 est ignoree : la matrice est supposee affine.
 *
 * Memes regles que Vector3Simd.h : n quelconque (0 inclus), reste scalaire, pas d'alignement
 * requis, entree == sortie autorise, pas de chevauchement partiel.
 */
namespace math
{
    using Mat4f = Mat4<float>;

    static_assert(sizeof(Mat4f) == 16 * sizeof(float), "Mat4<float> doit faire 64 octets (16 floats contigus)");
    static_assert(std::is_standard_layout_v<Mat4f>, "Mat4<float> doit etre standard-layout");

    namespace ref
    {
        /** @brief out[i] = m.MultiplyPointAffine(in[i]) pour i dans [0, n). */
        void TransformPointsBatch(const Mat4f& m, const Vec3f* in, Vec3f* out, std::size_t n);

        /** @brief Version SoA. out doit deja avoir une taille >= n. */
        void TransformPointsBatchSoA(const Mat4f& m, const Vec3SoA& in, Vec3SoA& out, std::size_t n);

        /** @brief out[i] = a[i] * b[i] (operator* existant de Mat4). */
        void MultiplyBatch(const Mat4f* a, const Mat4f* b, Mat4f* out, std::size_t n);
    }

    namespace simd
    {
        /**
         * @brief SSE, 4 points par iteration.
         * Les points AoS sont transposes en X/Y/Z (une lane = un point), la matrice est diffusee
         * (12 registres constants m00..m32), puis le resultat est re-transpose en AoS.
         */
        void TransformPointsBatch(const Mat4f& m, const Vec3f* in, Vec3f* out, std::size_t n);

        /**
         * @brief SSE, 1 point par iteration, sans transposition.
         * Les lanes contiennent (x', y', z', w') d'un seul point : p' = x*L0 + y*L1 + z*L2 + L3
         * avec Li la ligne i de la matrice. La lane w' est calculee pour rien (25 % de lanes perdues).
         */
        void TransformPointsBatchPerPoint(const Mat4f& m, const Vec3f* in, Vec3f* out, std::size_t n);

        /** @brief SSE sur donnees SoA, 4 points par iteration, aucune transposition. */
        void TransformPointsBatchSoA(const Mat4f& m, const Vec3SoA& in, Vec3SoA& out, std::size_t n);

        /**
         * @brief Produit de deux matrices 4x4 en SSE.
         * Ligne i du resultat = a[i][0]*B0 + a[i][1]*B1 + a[i][2]*B2 + a[i][3]*B3 (Bk = ligne k de b).
         * Le resultat peut etre l'un des operandes (a = a * b fonctionne).
         */
        Mat4f Multiply(const Mat4f& a, const Mat4f& b);

        /** @brief out[i] = simd::Multiply(a[i], b[i]). */
        void MultiplyBatch(const Mat4f* a, const Mat4f* b, Mat4f* out, std::size_t n);
    }
}
