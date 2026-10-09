#pragma once
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>
#include "Vector3Batch.h"
#include "Mat4Batch.h"
#include "Quaternion.h"
#include "Vector2.h"
#include "Mat3.h"

/**
 * @file Data.h
 * @brief Reproducible input data for the benchmark.
 *
 * Same seed => same data on every run and every machine:
 *  - std::mt19937 is an algorithm fixed by the C++ standard (same sequence everywhere);
 *  - std::uniform_real_distribution is NOT fixed by the standard (depends on the STL), so the
 *    conversion to float is done by hand.
 */
namespace bench
{
    class Rng
    {
    public:
        explicit Rng(std::uint32_t seed) : m_gen(seed) {}

        /** Uniform float in [lo, hi): 24 random bits -> [0, 1) -> [lo, hi). */
        float Uniform(float lo, float hi)
        {
            const float t = static_cast<float>(m_gen() >> 8) * (1.0f / 16777216.0f);
            return lo + (hi - lo) * t;
        }

    private:
        std::mt19937 m_gen;
    };

    /** n vectors with components uniform in [-range, range). */
    inline std::vector<math::Vec3f> RandomVectors(std::size_t n, Rng& rng, float range = 100.0f)
    {
        std::vector<math::Vec3f> v(n);
        for (math::Vec3f& e : v)
        {
            const float x = rng.Uniform(-range, range);
            const float y = rng.Uniform(-range, range);
            const float z = rng.Uniform(-range, range);
            e = math::Vec3f(x, y, z);
        }
        return v;
    }

    /** Random affine matrix: uniform scale, rotation around a random axis, translation (TRS). */
    inline math::Mat4f RandomAffine(Rng& rng)
    {
        math::Vec3f axis(rng.Uniform(-1.0f, 1.0f), rng.Uniform(-1.0f, 1.0f), rng.Uniform(-1.0f, 1.0f));
        axis = axis.Normalized();
        if (axis.LengthSquared() == 0.0f)
            axis = math::Vec3f(0.0f, 0.0f, 1.0f);

        const float angle = rng.Uniform(-3.14159265f, 3.14159265f);
        const float scale = rng.Uniform(0.5f, 2.0f);
        const math::Vec3f translation(rng.Uniform(-50.0f, 50.0f), rng.Uniform(-50.0f, 50.0f), rng.Uniform(-50.0f, 50.0f));

        return math::Mat4f::TRS(translation, math::Quaternion::FromAxisAngle(axis, angle), math::Vec3f(scale, scale, scale));
    }

    /** n general 4x4 matrices (16 coefficients in [-2, 2)). */
    inline std::vector<math::Mat4f> RandomMatrices(std::size_t n, Rng& rng)
    {
        std::vector<math::Mat4f> v(n);
        for (math::Mat4f& mat : v)
            for (int r = 0; r < 4; ++r)
                for (int c = 0; c < 4; ++c)
                    mat.m[r][c] = rng.Uniform(-2.0f, 2.0f);
        return v;
    }

    /** n 2D vectors with components uniform in [-range, range). */
    inline std::vector<math::Vector2<float>> RandomVectors2(std::size_t n, Rng& rng, float range = 100.0f)
    {
        std::vector<math::Vector2<float>> v(n);
        for (math::Vector2<float>& e : v)
        {
            const float x = rng.Uniform(-range, range);
            const float y = rng.Uniform(-range, range);
            e = math::Vector2<float>(x, y);
        }
        return v;
    }

    /** n general 3x3 matrices (9 coefficients in [-2, 2)). */
    inline std::vector<math::Mat3<float>> RandomMatrices3(std::size_t n, Rng& rng)
    {
        std::vector<math::Mat3<float>> v(n);
        for (math::Mat3<float>& mat : v)
            for (int r = 0; r < 3; ++r)
                for (int c = 0; c < 3; ++c)
                    mat.m[r][c] = rng.Uniform(-2.0f, 2.0f);
        return v;
    }
}
