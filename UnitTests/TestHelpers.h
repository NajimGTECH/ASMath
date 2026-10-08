#pragma once
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "Vector3Batch.h"
#include "Mat4Batch.h"

/**
 * @file TestHelpers.h
 * @brief Tolerances, random data and out-of-bounds detection shared by the batch tests.
 */
namespace test
{
    // ====================================================================================
    // Floating-point tolerances
    //
    // u = 2^-24 is the "unit roundoff" of float: with round-to-nearest, the result of ONE float
    // operation (+, -, *, /, sqrt) is the exact result times (1 + d) with |d| <= u.
    //
    // A sum of k rounded products has an error bounded by gamma(k) * (sum of |terms|), with
    // gamma(k) = k*u / (1 - k*u) (N. Higham, "Accuracy and Stability of Numerical Algorithms",
    // chapter 3). We compare the float results to the same formula computed in double: the double
    // error (~1e-16) is negligible compared to u (~6e-8), so double is our "exact" value.
    //
    // This gives tolerances that are justified (not a magic 1e-5) and that scale with the size of
    // the values: absolute when the result is near 0, relative to the terms otherwise.
    // ====================================================================================
    constexpr double kU = FLT_EPSILON / 2.0; // 2^-24 ~ 5.96e-8

    constexpr double Gamma(int k) { return k * kU / (1.0 - k * kU); }

    /**
     * @brief |a - b| <= max(absTol, relTol * max(|a|, |b|)).
     *
     * The absolute part is needed near 0 (a relative tolerance means nothing when the expected
     * value is 0), the relative part for large values (float spacing grows with the value).
     */
    inline bool NearlyEqual(double a, double b, double absTol, double relTol)
    {
        const double diff = std::fabs(a - b);
        const double largest = std::fmax(std::fabs(a), std::fabs(b));
        return diff <= std::fmax(absTol, relTol * largest);
    }

    /**
     * Tolerance for matrix tests that use sin/cos (rotations): absolute 1e-6 and relative 1e-6.
     * Why: float(pi/2) is off by ~4.4e-8, so cos(float(pi/2)) = -4.37e-8 instead of 0, and the
     * quaternion -> matrix -> point chain does about 10 float operations on values of order 1
     * (10 * u ~ 6e-7). 1e-6 covers this with a small margin and still detects any real error.
     */
    constexpr double kAngleAbsTol = 1e-6;
    constexpr double kAngleRelTol = 1e-6;

    // ====================================================================================
    // Exact results computed in double
    // ====================================================================================

    /** Dot product in double + its error bound gamma(3) * sum |a_i * b_i| (3 products, 2 additions). */
    inline double DotDouble(const math::Vec3f& a, const math::Vec3f& b, double& bound)
    {
        const double px = double(a.x) * b.x, py = double(a.y) * b.y, pz = double(a.z) * b.z;
        bound = Gamma(3) * (std::fabs(px) + std::fabs(py) + std::fabs(pz));
        return px + py + pz;
    }

    /**
     * Normalized vector in double.
     * Error bound for each float component (first order): squared length gamma(3), sqrt halves it
     * and adds u, the division adds u -> about 3.5u relative to the component. Components of a unit
     * vector are <= 1, so an absolute tolerance of 4u is enough.
     */
    constexpr double kNormalizeAbsTol = 4.0 * kU;

    inline void NormalizeDouble(const math::Vec3f& v, double out[3])
    {
        const double len = std::sqrt(double(v.x) * v.x + double(v.y) * v.y + double(v.z) * v.z);
        out[0] = len > 0.0 ? v.x / len : 0.0;
        out[1] = len > 0.0 ? v.y / len : 0.0;
        out[2] = len > 0.0 ? v.z / len : 0.0;
    }

    /**
     * Affine transformation in double (row vector, w = 1) + an error bound per component:
     * x' = x*m00 + y*m10 + z*m20 + m30 = 4 terms, 3 products and 3 additions -> gamma(4) * sum |terms|.
     */
    inline void TransformDouble(const math::Mat4f& m, const math::Vec3f& p, double out[3], double bound[3])
    {
        for (int c = 0; c < 3; ++c)
        {
            const double t0 = double(p.x) * m.m[0][c];
            const double t1 = double(p.y) * m.m[1][c];
            const double t2 = double(p.z) * m.m[2][c];
            const double t3 = m.m[3][c];
            out[c] = t0 + t1 + t2 + t3;
            bound[c] = Gamma(4) * (std::fabs(t0) + std::fabs(t1) + std::fabs(t2) + std::fabs(t3));
        }
    }

    // ====================================================================================
    // Reproducible random data
    // ====================================================================================

    /** Small wrapper around std::mt19937 (same sequence on every machine for the same seed). */
    class Rng
    {
    public:
        explicit Rng(std::uint32_t seed) : m_gen(seed) {}

        /** Uniform float in [lo, hi). 24 random bits -> [0, 1) -> [lo, hi). */
        float Uniform(float lo, float hi)
        {
            const float t = static_cast<float>(m_gen() >> 8) * (1.0f / 16777216.0f);
            return lo + (hi - lo) * t;
        }

        math::Vec3f Vector(float range)
        {
            const float x = Uniform(-range, range);
            const float y = Uniform(-range, range);
            const float z = Uniform(-range, range);
            return math::Vec3f(x, y, z);
        }

    private:
        std::mt19937 m_gen;
    };

    inline std::vector<math::Vec3f> RandomVectors(std::size_t n, Rng& rng, float range = 100.0f)
    {
        std::vector<math::Vec3f> v(n);
        for (math::Vec3f& e : v)
            e = rng.Vector(range);
        return v;
    }

    /** Batch sizes used by the "all sizes" tests: 0..40 (every remainder 0..3, several blocks of 4) + bigger ones. */
    inline std::vector<std::size_t> TestSizes()
    {
        std::vector<std::size_t> sizes;
        for (std::size_t n = 0; n <= 40; ++n)
            sizes.push_back(n);
        sizes.push_back(1000);
        sizes.push_back(1001);
        sizes.push_back(1002);
        sizes.push_back(1003);
        return sizes;
    }

    // ====================================================================================
    // Out-of-bounds detection
    // ====================================================================================

    /**
     * @brief Array whose LAST element is placed just before a memory page with no access rights.
     *
     * Reading or writing even one byte after data[n-1] immediately crashes the test with an
     * access violation (instead of silently reading garbage). This is exactly the bug a SIMD
     * loop would have if it did a 16-byte load on the last 12-byte Vector3.
     *
     * How: VirtualAlloc reserves whole pages. We allocate the pages needed for the data plus one
     * extra page, mark the extra page PAGE_NOACCESS with VirtualProtect, and place the array so
     * that it ends exactly at the start of that page.
     */
    template <typename T>
    class GuardedArray
    {
    public:
        explicit GuardedArray(std::size_t n) : m_size(n)
        {
            SYSTEM_INFO info;
            GetSystemInfo(&info);
            const std::size_t pageSize = info.dwPageSize;
            const std::size_t bytes = n * sizeof(T);
            const std::size_t dataPages = (bytes + pageSize - 1) / pageSize;

            m_base = static_cast<char*>(VirtualAlloc(nullptr, (dataPages + 1) * pageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
            char* guardPage = m_base + dataPages * pageSize;
            DWORD oldProtect = 0;
            VirtualProtect(guardPage, pageSize, PAGE_NOACCESS, &oldProtect);

            m_data = reinterpret_cast<T*>(guardPage - bytes);
            for (std::size_t i = 0; i < n; ++i)
                m_data[i] = T{};
        }

        explicit GuardedArray(const std::vector<T>& values) : GuardedArray(values.size())
        {
            for (std::size_t i = 0; i < values.size(); ++i)
                m_data[i] = values[i];
        }

        ~GuardedArray() { VirtualFree(m_base, 0, MEM_RELEASE); }

        GuardedArray(const GuardedArray&) = delete;
        GuardedArray& operator=(const GuardedArray&) = delete;

        T* Data() { return m_data; }
        T& operator[](std::size_t i) { return m_data[i]; }
        std::size_t Size() const { return m_size; }

    private:
        char* m_base = nullptr;
        T* m_data = nullptr;
        std::size_t m_size = 0;
    };

    /** Value written after the end of SoA outputs: it must still be there after the call. */
    constexpr float kSentinel = -12345.0f;

    inline std::wstring Label(const std::wstring& version, std::size_t n, std::size_t i)
    {
        return version + L" n=" + std::to_wstring(n) + L" i=" + std::to_wstring(i);
    }
}
