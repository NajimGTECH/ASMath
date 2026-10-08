#pragma once
#include <cstddef>
#include <type_traits>
#include <vector>
#include "Vector3.h"

/**
 * @file Vector3Batch.h
 * @brief Batch processing on arrays of Vector3<float>: dot products and normalization.
 *
 * Every processing exists in two versions with exactly the same signature:
 *  - `math::ref`  : the C++ reference. Plain loops that reuse the Vector3 API (Dot, Normalized).
 *                   Compiled with /O2, the compiler is allowed to auto-vectorize them.
 *  - `math::simd` : explicit SIMD version written with SSE/SSE2 intrinsics (4 floats per register).
 *
 * Both versions also exist for two memory layouts:
 *  - AoS (Array of Structures): `const Vec3f*`, memory = x0 y0 z0 x1 y1 z1 ...
 *  - SoA (Structure of Arrays): `Vec3SoA`, memory = x0 x1 x2 ... / y0 y1 y2 ... / z0 z1 z2 ...
 *
 * Rules shared by all the batch functions of the library
 * ------------------------------------------------------
 *  - **Batch size**: n can be 0 (nothing is read or written), small, or not a multiple of 4.
 *    The SIMD versions process blocks of 4 elements while `i + 4 <= n`, then finish the last
 *    `n % 4` elements with the scalar code of the reference. Nothing is read or written outside
 *    [0, n).
 *  - **Alignment**: no alignment is required. The SIMD code only uses unaligned loads/stores
 *    (_mm_loadu_ps / _mm_storeu_ps). An array of Vector3<float> is only 4-byte aligned anyway
 *    (sizeof = 12), so aligned loads (_mm_load_ps, 16 bytes) could not be used on it.
 *  - **Aliasing**: input and output may be the same array (in-place processing), but must not
 *    partially overlap.
 *  - **Same arithmetic as the reference**: the SIMD code does the operations in the same order
 *    ((x*x' + y*y') + z*z', IEEE sqrt and division). With /fp:precise and no FMA (default x64
 *    target, /arch:SSE2), ref and simd give bit-identical results. The approximate
 *    normalization (NormalizeBatchApprox) is the only exception.
 */
namespace math
{
    /** @brief The float 3D vector used by all the batch functions. */
    using Vec3f = Vector3<float>;

    // The AoS SIMD code reads an array of Vec3f as an array of 3*n floats.
    // This is only valid if Vector3<float> is exactly x, y, z with no padding.
    static_assert(sizeof(Vec3f) == 3 * sizeof(float), "Vector3<float> must be 12 bytes (x, y, z contiguous)");
    static_assert(std::is_standard_layout_v<Vec3f>, "Vector3<float> must be standard-layout");

    /**
     * @brief Structure of Arrays (SoA): one array per component.
     *
     * Vector i is (x[i], y[i], z[i]). With this layout a single _mm_loadu_ps(&x[i]) loads the x
     * of 4 consecutive vectors, so no transposition is needed.
     *
     * Alignment: the default allocator of MSVC x64 returns 16-byte aligned memory, but the SIMD
     * functions do not rely on it (unaligned loads only).
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

    // ================================================================================
    // Layout conversions (plain C++). Measured separately in the benchmark.
    // ================================================================================

    /** @brief AoS -> SoA. `out` must already have a size >= n (no allocation is done here). */
    void AoSToSoA(const Vec3f* in, Vec3SoA& out, std::size_t n);

    /** @brief SoA -> AoS. `in` must have a size >= n. */
    void SoAToAoS(const Vec3SoA& in, Vec3f* out, std::size_t n);

    // ================================================================================
    // C++ reference
    // ================================================================================
    namespace ref
    {
        /** @brief out[i] = a[i].Dot(b[i]) for i in [0, n). */
        void DotBatch(const Vec3f* a, const Vec3f* b, float* out, std::size_t n);

        /** @brief SoA version of DotBatch. a and b must have a size >= n. */
        void DotBatchSoA(const Vec3SoA& a, const Vec3SoA& b, float* out, std::size_t n);

        /**
         * @brief out[i] = in[i].Normalized() for i in [0, n).
         *
         * Zero vector rule (same as Vector3::Normalized): if the length is not > 0 (zero, NaN,
         * or squared length that underflows to 0), the result is (0, 0, 0).
         */
        void NormalizeBatch(const Vec3f* in, Vec3f* out, std::size_t n);

        /** @brief SoA version of NormalizeBatch. `out` must already have a size >= n. */
        void NormalizeBatchSoA(const Vec3SoA& in, Vec3SoA& out, std::size_t n);
    }

    // ================================================================================
    // Explicit SIMD (SSE/SSE2)
    // ================================================================================
    namespace simd
    {
        /**
         * @brief SSE version of ref::DotBatch. 4 dot products per iteration.
         *
         * Each iteration loads 4 vectors of a and 4 of b (3 registers each), transposes them so
         * that lane k holds vector i+k, then computes the 4 results and stores them with one
         * 16-byte store.
         */
        void DotBatch(const Vec3f* a, const Vec3f* b, float* out, std::size_t n);

        /** @brief SSE version of ref::DotBatchSoA. Direct loads, no transposition. */
        void DotBatchSoA(const Vec3SoA& a, const Vec3SoA& b, float* out, std::size_t n);

        /**
         * @brief SSE version of ref::NormalizeBatch. 4 normalizations per iteration.
         *
         * No branch in the SIMD loop: a mask (length > 0) is computed for the 4 lanes and
         * used to force the result to 0 for zero vectors (same rule as the reference).
         */
        void NormalizeBatch(const Vec3f* in, Vec3f* out, std::size_t n);

        /** @brief SSE version of ref::NormalizeBatchSoA. */
        void NormalizeBatchSoA(const Vec3SoA& in, Vec3SoA& out, std::size_t n);

        /**
         * @brief (Optional extension) Approximate normalization with _mm_rsqrt_ps.
         *
         * Uses the approximate reciprocal square root of the CPU instead of sqrt + division.
         * Intel guarantees a relative error <= 1.5 * 2^-12 (about 3.7e-4) on 1/sqrt, so each
         * component has a relative error of about the same size. NOT bit-identical to the
         * reference. Zero rule: vectors with a squared length below FLT_MIN (~1.2e-38, i.e. a
         * length below ~1.1e-19) give (0, 0, 0), because rsqrt treats denormals as zero.
         */
        void NormalizeBatchApprox(const Vec3f* in, Vec3f* out, std::size_t n);
    }
}
