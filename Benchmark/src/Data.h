#pragma once
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>
#include "Vector3Simd.h"
#include "Mat4Simd.h"
#include "Quaternion.h"

namespace bench
{
    /**
     * @brief Generateur pseudo-aleatoire reproductible.
     *
     * std::mt19937 produit la meme suite sur toutes les plateformes (algorithme fixe par la norme),
     * alors que std::uniform_real_distribution depend de l'implementation de la STL.
     * On fait donc la conversion en float nous-memes : meme graine => memes donnees partout.
     */
    class Rng
    {
    public:
        explicit Rng(std::uint32_t seed) : m_gen(seed) {}

        /** @brief Float uniforme dans [lo, hi). */
        float Uniform(float lo, float hi)
        {
            const float u = static_cast<float>(m_gen() >> 8) * (1.0f / 16777216.0f); // 24 bits -> [0, 1)
            return lo + (hi - lo) * u;
        }

    private:
        std::mt19937 m_gen;
    };

    /** @brief n vecteurs aux composantes uniformes dans [-range, range). */
    inline std::vector<math::Vec3f> RandomVectors(std::size_t n, Rng& rng, float range = 100.0f)
    {
        std::vector<math::Vec3f> v(n);
        for (auto& e : v)
            e = math::Vec3f(rng.Uniform(-range, range), rng.Uniform(-range, range), rng.Uniform(-range, range));
        return v;
    }

    /** @brief Matrice affine aleatoire : TRS (rotation autour d'un axe aleatoire, echelle, translation). */
    inline math::Mat4f RandomAffine(Rng& rng)
    {
        math::Vec3f axis(rng.Uniform(-1.0f, 1.0f), rng.Uniform(-1.0f, 1.0f), rng.Uniform(-1.0f, 1.0f));
        axis = axis.Normalized();
        if (axis.LengthSquared() == 0.0f)
            axis = math::Vec3f(0.0f, 0.0f, 1.0f);

        const float angle = rng.Uniform(-3.14159265f, 3.14159265f);
        const math::Vec3f translation(rng.Uniform(-50.0f, 50.0f), rng.Uniform(-50.0f, 50.0f), rng.Uniform(-50.0f, 50.0f));
        const float s = rng.Uniform(0.5f, 2.0f);

        return math::Mat4f::TRS(translation, math::Quaternion::FromAxisAngle(axis, angle), math::Vec3f(s, s, s));
    }

    /** @brief n matrices 4x4 quelconques (16 coefficients dans [-2, 2)). */
    inline std::vector<math::Mat4f> RandomMatrices(std::size_t n, Rng& rng)
    {
        std::vector<math::Mat4f> v(n);
        for (auto& mat : v)
            for (int r = 0; r < 4; ++r)
                for (int c = 0; c < 4; ++c)
                    mat.m[r][c] = rng.Uniform(-2.0f, 2.0f);
        return v;
    }
}
