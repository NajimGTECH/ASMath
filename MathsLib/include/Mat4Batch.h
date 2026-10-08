#pragma once
#include <cstddef>
#include <type_traits>
#include "Mat4.h"
#include "Vector3Batch.h"

/**
 * @file Mat4Batch.h
 * @brief Batch processing with Mat4<float>: transformation of 3D points by the same affine matrix.
 *
 * Conventions (same as Mat4, see Mat4.h):
 *  - storage row-major `m[row][col]`, 16 contiguous floats. Row i is 4 contiguous floats, so it can
 *    be loaded with a single _mm_loadu_ps;
 *  - row vectors: p' = p * M, translation in row 3 (m[3][0..2]);
 *  - points use the homogeneous coordinate w = 1, **no perspective divide**. Column 3 is ignored,
 *    the matrix is assumed to be affine:
 *        x' = ((x*m[0][0] + y*m[1][0]) + z*m[2][0]) + m[3][0]   (same for y' with column 1, z' with column 2)
 *    This is exactly Mat4::MultiplyPointAffine, and the SIMD versions use the same order.
 *
 * Same batch rules as Vector3Batch.h: any n (0 included), scalar remainder, no alignment required,
 * input == output allowed, no partial overlap.
 */
namespace math
{
    /** @brief The float 4x4 matrix used by the batch functions. */
    using Mat4f = Mat4<float>;

    static_assert(sizeof(Mat4f) == 16 * sizeof(float), "Mat4<float> must be 64 bytes (16 contiguous floats)");
    static_assert(std::is_standard_layout_v<Mat4f>, "Mat4<float> must be standard-layout");

    namespace ref
    {
        /** @brief out[i] = m.MultiplyPointAffine(in[i]) for i in [0, n). */
        void TransformPointsBatch(const Mat4f& m, const Vec3f* in, Vec3f* out, std::size_t n);

        /** @brief SoA version. `out` must already have a size >= n. */
        void TransformPointsBatchSoA(const Mat4f& m, const Vec3SoA& in, Vec3SoA& out, std::size_t n);

        /** @brief (Optional extension) out[i] = a[i] * b[i] with the existing Mat4::operator*. */
        void MultiplyBatch(const Mat4f* a, const Mat4f* b, Mat4f* out, std::size_t n);
    }

    namespace simd
    {
        /**
         * @brief SSE version of ref::TransformPointsBatch, 4 points per iteration.
         *
         * The 4 points are transposed into X, Y, Z registers (lane k = point i+k). Each of the 12
         * useful coefficients of the matrix is broadcast in the 4 lanes of a register
         * (_mm_set1_ps), so x' = x*m00 + y*m10 + z*m20 + m30 is computed for 4 points at once.
         * The results are transposed back to AoS.
         */
        void TransformPointsBatch(const Mat4f& m, const Vec3f* in, Vec3f* out, std::size_t n);

        /**
         * @brief "Naive" SSE version: 1 point per iteration, no transposition.
         *
         * Lanes of the result = (x', y', z', w') of a single point:
         *     result = x*Row0 + y*Row1 + z*Row2 + Row3
         * where RowK is row K of the matrix loaded in one register. The w' lane is computed
         * for nothing (25 % of the work is wasted). Kept to compare with the 4-points version.
         */
        void TransformPointsBatchNaive(const Mat4f& m, const Vec3f* in, Vec3f* out, std::size_t n);

        /** @brief SSE version on SoA data: 4 points per iteration, direct loads, no transposition. */
        void TransformPointsBatchSoA(const Mat4f& m, const Vec3SoA& in, Vec3SoA& out, std::size_t n);

        /**
         * @brief (Optional extension) Product of two 4x4 matrices with SSE.
         *
         * Row i of the result = a[i][0]*B0 + a[i][1]*B1 + a[i][2]*B2 + a[i][3]*B3 (Bk = row k of b).
         * Same operation order as Mat4::operator*, so the result is bit-identical.
         */
        Mat4f Multiply(const Mat4f& a, const Mat4f& b);

        /** @brief (Optional extension) out[i] = simd::Multiply(a[i], b[i]). */
        void MultiplyBatch(const Mat4f* a, const Mat4f* b, Mat4f* out, std::size_t n);
    }
}
