#pragma once
#include <cstddef>
#include "Vector3Batch.h"

/**
 * @file AsmFunctions.h
 * @brief Functions written by hand in x64 assembly (MASM), see src/asm/Dot3.asm.
 *
 * `extern "C"` tells the C++ compiler that these functions use C linkage: their symbol name is
 * exactly "Dot3Asm" / "DotBatchAsm" (no C++ name mangling), which is the name used in the .asm file.
 * The arguments follow the Microsoft x64 calling convention (RCX, RDX, R8, R9, result in XMM0).
 */
extern "C"
{
    /**
     * @brief Dot product of two Vector3<float> given by the address of their x component.
     * @param a Address of a.x (followed by a.y and a.z). Passed in RCX.
     * @param b Address of b.x (followed by b.y and b.z). Passed in RDX.
     * @return (a.x*b.x + a.y*b.y) + a.z*b.z, returned in XMM0. Bit-identical to Vector3::Dot.
     */
    float Dot3Asm(const float* a, const float* b);

    /**
     * @brief out[i] = Dot3(a[i], b[i]) for i in [0, n), scalar loop written in assembly.
     * @param a Array of n Vector3<float> seen as 3*n floats (RCX).
     * @param b Array of n Vector3<float> seen as 3*n floats (RDX).
     * @param out Array of n floats (R8).
     * @param n Number of vectors, can be 0 (R9).
     */
    void DotBatchAsm(const float* a, const float* b, float* out, std::size_t n);
}

namespace math::x64asm
{
    /** @brief Typed wrapper of Dot3Asm for Vector3<float>. */
    inline float Dot(const Vec3f& a, const Vec3f& b)
    {
        return Dot3Asm(&a.x, &b.x);
    }

    /** @brief Typed wrapper of DotBatchAsm, same signature as ref::DotBatch. */
    inline void DotBatch(const Vec3f* a, const Vec3f* b, float* out, std::size_t n)
    {
        DotBatchAsm(reinterpret_cast<const float*>(a), reinterpret_cast<const float*>(b), out, n);
    }
}
