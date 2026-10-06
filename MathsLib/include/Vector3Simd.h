#pragma once
#include <cstddef>
#include <type_traits>
#include <vector>
#include "Vector3.h"

/**
 * @file Vector3Simd.h
 * @brief Traitements par lots sur des Vector3<float> : version de reference C++ (math::ref)
 *        et version SIMD explicite SSE/SSE2 (math::simd).
 *
 * Les deux namespaces exposent exactement les memes signatures pour pouvoir etre compares.
 *
 * Conventions communes a toutes les fonctions :
 *  - n peut valoir 0 (aucun acces memoire), etre petit, ou ne pas etre multiple de 4.
 *    Les versions SIMD traitent les blocs de 4 elements puis finissent les elements restants
 *    (n % 4) avec le meme calcul scalaire que la reference : aucune lecture ni ecriture hors limites.
 *  - Aucun alignement n'est exige : les versions SIMD utilisent des chargements/ecritures non
 *    alignes (_mm_loadu_ps / _mm_storeu_ps). Un tableau de Vector3<float> n'est de toute facon
 *    aligne que sur 4 octets (sizeof = 12).
 *  - Entree et sortie peuvent etre le meme tableau (traitement en place) mais ne doivent pas se
 *    chevaucher partiellement.
 *  - Les operations sont faites dans le meme ordre que la reference ((x*x + y*y) + z*z, divisions
 *    IEEE et sqrt IEEE), donc avec /fp:precise et sans FMA les resultats sont identiques bit a bit.
 */
namespace math
{
    using Vec3f = Vector3<float>;

    // Le SIMD AoS suppose que Vector3<float> est exactement 3 floats contigus, sans padding.
    static_assert(sizeof(Vec3f) == 3 * sizeof(float), "Vector3<float> doit faire 12 octets (x, y, z contigus)");
    static_assert(std::is_standard_layout_v<Vec3f>, "Vector3<float> doit etre standard-layout");

    /**
     * @brief Structure de tableaux (SoA) : un tableau par composante.
     *
     * x[i], y[i], z[i] forment le vecteur i. Un registre SSE charge directement 4 x consecutifs,
     * sans reorganisation. std::vector<float> est aligne sur 16 octets par l'allocateur MSVC x64,
     * mais les fonctions n'en dependent pas (chargements non alignes).
     */
    struct Vec3SoA
    {
        std::vector<float> x;
        std::vector<float> y;
        std::vector<float> z;

        Vec3SoA() = default;
        explicit Vec3SoA(std::size_t n) : x(n), y(n), z(n) {}

        void Resize(std::size_t n) { x.resize(n); y.resize(n); z.resize(n); }
        std::size_t Size() const { return x.size(); }
    };

    namespace ref
    {
        /** @brief out[i] = a[i].Dot(b[i]) pour i dans [0, n). */
        void DotBatch(const Vec3f* a, const Vec3f* b, float* out, std::size_t n);

        /**
         * @brief out[i] = in[i].Normalized() pour i dans [0, n).
         * Vecteur nul (ou de longueur NaN) : le resultat est (0, 0, 0), comme Vector3::Normalized().
         */
        void NormalizeBatch(const Vec3f* in, Vec3f* out, std::size_t n);

        /** @brief Version SoA de DotBatch. a, b doivent contenir au moins n elements. */
        void DotBatchSoA(const Vec3SoA& a, const Vec3SoA& b, float* out, std::size_t n);

        /** @brief Version SoA de NormalizeBatch. out doit deja avoir une taille >= n (aucune allocation). */
        void NormalizeBatchSoA(const Vec3SoA& in, Vec3SoA& out, std::size_t n);

        /** @brief Conversion AoS -> SoA. out doit deja avoir une taille >= n. */
        void AoSToSoA(const Vec3f* in, Vec3SoA& out, std::size_t n);

        /** @brief Conversion SoA -> AoS. */
        void SoAToAoS(const Vec3SoA& in, Vec3f* out, std::size_t n);
    }

    namespace simd
    {
        /** @brief SSE : 4 produits scalaires par iteration (transposition AoS -> SoA dans les registres). */
        void DotBatch(const Vec3f* a, const Vec3f* b, float* out, std::size_t n);

        /** @brief SSE : 4 normalisations par iteration, sans branche (masque pour le vecteur nul). */
        void NormalizeBatch(const Vec3f* in, Vec3f* out, std::size_t n);

        /** @brief SSE sur donnees SoA : chargements directs, aucune transposition. */
        void DotBatchSoA(const Vec3SoA& a, const Vec3SoA& b, float* out, std::size_t n);

        /** @brief SSE sur donnees SoA. */
        void NormalizeBatchSoA(const Vec3SoA& in, Vec3SoA& out, std::size_t n);

        /** @brief Conversion AoS -> SoA avec transposition SSE (4 vecteurs par iteration). */
        void AoSToSoA(const Vec3f* in, Vec3SoA& out, std::size_t n);

        /** @brief Conversion SoA -> AoS avec transposition SSE (4 vecteurs par iteration). */
        void SoAToAoS(const Vec3SoA& in, Vec3f* out, std::size_t n);
    }
}
