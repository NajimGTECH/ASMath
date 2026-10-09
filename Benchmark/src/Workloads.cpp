#include "Workloads.h"
#include "Data.h"
#include "AsmFunctions.h"

using namespace math;

namespace bench
{
    namespace
    {
        // ================================================================================
        // One small function per measured variant. They all have the same signature so that the
        // benchmark can call them through a function pointer (see Measure.cpp).
        // ================================================================================

        // ---- dot product ----
        void DotRefAoS(Workspace& w) { ref::DotBatch(w.a.data(), w.b.data(), w.outFloats.data(), w.n); }
        void DotSimdAoS(Workspace& w) { simd::DotBatch(w.a.data(), w.b.data(), w.outFloats.data(), w.n); }
        void DotAsmAoS(Workspace& w) { x64asm::DotBatch(w.a.data(), w.b.data(), w.outFloats.data(), w.n); }
        void DotRefSoA(Workspace& w) { ref::DotBatchSoA(w.soaA, w.soaB, w.outFloats.data(), w.n); }
        void DotSimdSoA(Workspace& w) { simd::DotBatchSoA(w.soaA, w.soaB, w.outFloats.data(), w.n); }
        void DotSimdSoAWithConversion(Workspace& w)
        {
            // Total cost when the data lives in AoS: convert both inputs, then compute in SoA.
            AoSToSoA(w.a.data(), w.tmpA, w.n);
            AoSToSoA(w.b.data(), w.tmpB, w.n);
            simd::DotBatchSoA(w.tmpA, w.tmpB, w.outFloats.data(), w.n);
        }

        // ---- normalization ----
        void NormRefAoS(Workspace& w) { ref::NormalizeBatch(w.a.data(), w.outVectors.data(), w.n); }
        void NormSimdAoS(Workspace& w) { simd::NormalizeBatch(w.a.data(), w.outVectors.data(), w.n); }
        void NormRefSoA(Workspace& w) { ref::NormalizeBatchSoA(w.soaA, w.outSoA, w.n); }
        void NormSimdSoA(Workspace& w) { simd::NormalizeBatchSoA(w.soaA, w.outSoA, w.n); }
        void NormSimdSoAWithConversion(Workspace& w)
        {
            // AoS -> SoA, compute, SoA -> AoS (the result is wanted in AoS like the input).
            AoSToSoA(w.a.data(), w.tmpA, w.n);
            simd::NormalizeBatchSoA(w.tmpA, w.tmpB, w.n);
            SoAToAoS(w.tmpB, w.outVectors.data(), w.n);
        }
        void NormSimdAoSApprox(Workspace& w) { simd::NormalizeBatchApprox(w.a.data(), w.outVectors.data(), w.n); }

        // ---- transformation of points ----
        void TransRefAoS(Workspace& w) { ref::TransformPointsBatch(w.matrix, w.a.data(), w.outVectors.data(), w.n); }
        void TransSimdAoS(Workspace& w) { simd::TransformPointsBatch(w.matrix, w.a.data(), w.outVectors.data(), w.n); }
        void TransSimdAoSNaive(Workspace& w) { simd::TransformPointsBatchNaive(w.matrix, w.a.data(), w.outVectors.data(), w.n); }
        void TransRefSoA(Workspace& w) { ref::TransformPointsBatchSoA(w.matrix, w.soaA, w.outSoA, w.n); }
        void TransSimdSoA(Workspace& w) { simd::TransformPointsBatchSoA(w.matrix, w.soaA, w.outSoA, w.n); }
        void TransSimdSoAWithConversion(Workspace& w)
        {
            AoSToSoA(w.a.data(), w.tmpA, w.n);
            simd::TransformPointsBatchSoA(w.matrix, w.tmpA, w.tmpB, w.n);
            SoAToAoS(w.tmpB, w.outVectors.data(), w.n);
        }

        // ---- layout conversions alone ----
        void ConvAoSToSoA(Workspace& w) { AoSToSoA(w.a.data(), w.outSoA, w.n); }
        void ConvSoAToAoS(Workspace& w) { SoAToAoS(w.soaA, w.outVectors.data(), w.n); }

        // ---- Mat4 x Mat4 (optional extension) ----
        void MatMulRef(Workspace& w) { ref::MultiplyBatch(w.matA.data(), w.matB.data(), w.outMatrices.data(), w.n); }
        void MatMulSimd(Workspace& w) { simd::MultiplyBatch(w.matA.data(), w.matB.data(), w.outMatrices.data(), w.n); }

        // ---- Vector2 (same functions as the benchmarks of the Game project) ----
        constexpr float kLerpT = 0.5f;

        void Vec2DotRefAoS(Workspace& w)
        {
            float result = 0.0f;
            for (std::size_t i = 0; i < w.n; ++i)
                result += w.v2A[i].Dot(w.v2B[i]);
            w.outScalar = result;
        }
        void Vec2DotSimdAoS(Workspace& w)
        {
            float result = 0.0f;
            for (std::size_t i = 0; i < w.n; ++i)
                result += Vector2SSE::Dot(w.v2A[i], w.v2B[i]);
            w.outScalar = result;
        }
        void Vec2DotRefSoA(Workspace& w)
        {
            float result = 0.0f;
            for (std::size_t i = 0; i < w.n; ++i)
                result += w.v2SoaA.x[i] * w.v2SoaB.x[i] + w.v2SoaA.y[i] * w.v2SoaB.y[i];
            w.outScalar = result;
        }
        void Vec2DotSimdSoA(Workspace& w) { w.outScalar = Vector2SSE::DotSoA(w.v2SoaA, w.v2SoaB); }

        void Vec2ScaleRefAoS(Workspace& w)
        {
            for (std::size_t i = 0; i < w.n; ++i)
                w.outV2[i] = Vec2f::Scale(w.v2A[i], w.v2B[i]);
        }
        void Vec2ScaleSimdAoS(Workspace& w)
        {
            for (std::size_t i = 0; i < w.n; ++i)
                w.outV2[i] = Vector2SSE::Scale(w.v2A[i], w.v2B[i]);
        }
        void Vec2ScaleRefSoA(Workspace& w)
        {
            for (std::size_t i = 0; i < w.n; ++i)
            {
                w.outV2SoA.x[i] = w.v2SoaA.x[i] * w.v2SoaB.x[i];
                w.outV2SoA.y[i] = w.v2SoaA.y[i] * w.v2SoaB.y[i];
            }
        }
        void Vec2ScaleSimdSoA(Workspace& w) { Vector2SSE::ScaleSoA(w.v2SoaA, w.v2SoaB, w.outV2SoA); }

        void Vec2LerpRefAoS(Workspace& w)
        {
            for (std::size_t i = 0; i < w.n; ++i)
                w.outV2[i] = Vec2f::Lerp(w.v2A[i], w.v2B[i], kLerpT);
        }
        void Vec2LerpSimdAoS(Workspace& w)
        {
            for (std::size_t i = 0; i < w.n; ++i)
                w.outV2[i] = Vector2SSE::Lerp(w.v2A[i], w.v2B[i], kLerpT);
        }
        void Vec2LerpRefSoA(Workspace& w)
        {
            for (std::size_t i = 0; i < w.n; ++i)
            {
                w.outV2SoA.x[i] = w.v2SoaA.x[i] + (w.v2SoaB.x[i] - w.v2SoaA.x[i]) * kLerpT;
                w.outV2SoA.y[i] = w.v2SoaA.y[i] + (w.v2SoaB.y[i] - w.v2SoaA.y[i]) * kLerpT;
            }
        }
        void Vec2LerpSimdSoA(Workspace& w) { Vector2SSE::LerpSoA(w.v2SoaA, w.v2SoaB, kLerpT, w.outV2SoA); }

        // ---- Mat3 (same functions as the benchmarks of the Game project) ----
        void Mat3DetRefAoS(Workspace& w)
        {
            float result = 0.0f;
            for (std::size_t i = 0; i < w.n; ++i)
                result += w.m3A[i].Determinant();
            w.outScalar = result;
        }
        void Mat3DetSimdAoS(Workspace& w)
        {
            float result = 0.0f;
            for (std::size_t i = 0; i < w.n; ++i)
                result += Mat3SSE::Determinant(w.m3A[i]);
            w.outScalar = result;
        }
        void Mat3DetRefSoA(Workspace& w) { w.outScalar = Mat3SSE::DeterminantSoA(w.m3SoaA); }
        void Mat3DetSimdSoA(Workspace& w) { w.outScalar = Mat3SSE::DeterminantSoASIMD(w.m3SoaA); }

        void Mat3TransRefAoS(Workspace& w)
        {
            for (std::size_t i = 0; i < w.n; ++i)
                w.outM3[i] = Mat3SSE::Transpose(w.m3A[i]);
        }
        void Mat3TransSimdAoS(Workspace& w)
        {
            for (std::size_t i = 0; i < w.n; ++i)
                w.outM3[i] = Mat3SSE::TransposeSIMD(w.m3A[i]);
        }
        void Mat3TransRefSoA(Workspace& w) { Mat3SSE::TransposeSoA(w.m3SoaA, w.outM3SoA); }
        void Mat3TransSimdSoA(Workspace& w) { Mat3SSE::TransposeSoASIMD(w.m3SoaA, w.outM3SoA); }

        void Mat3MulRefAoS(Workspace& w)
        {
            for (std::size_t i = 0; i < w.n; ++i)
                w.outM3[i] = Mat3SSE::Multiply(w.m3A[i], w.m3B[i]);
        }
        void Mat3MulSimdAoS(Workspace& w)
        {
            for (std::size_t i = 0; i < w.n; ++i)
                w.outM3[i] = Mat3SSE::MultiplySIMD(w.m3A[i], w.m3B[i]);
        }
        void Mat3MulRefSoA(Workspace& w) { Mat3SSE::MultiplySoA(w.m3SoaA, w.m3SoaB, w.outM3SoA); }
        void Mat3MulSimdSoA(Workspace& w) { Mat3SSE::MultiplySoASIMD(w.m3SoaA, w.m3SoaB, w.outM3SoA); }

        // ---- layout conversions of the Vector2 / Mat3 inputs (preparation only, never measured) ----
        void ToSoA(const std::vector<Vec2f>& in, Vector2SSE::Vector2SoA& out)
        {
            out.Reserve(in.size());
            for (const Vec2f& v : in)
                out.Add(v.x, v.y);
        }
        void ToSoA(const std::vector<Mat3f>& in, Mat3SSE::Mat3SoA& out)
        {
            out.Reserve(in.size());
            for (const Mat3f& m : in)
                out.Add(m.m[0][0], m.m[0][1], m.m[0][2], m.m[1][0], m.m[1][1], m.m[1][2], m.m[2][0], m.m[2][1], m.m[2][2]);
        }

        // Small = 10 (not a multiple of 4: tests the remainder), medium = 1000 (fits in L1/L2 cache),
        // large = 100 000 (L2/L3), very large = 4 000 000 (48 MB per AoS array: does not fit in most caches).
        const std::vector<std::size_t> kVectorSizes = { 10, 1'000, 100'000, 4'000'000 };
        const std::vector<std::size_t> kMatrixSizes = { 10, 1'000, 100'000 }; // 64 bytes per matrix
    }

    const std::vector<Operation>& AllOperations()
    {
        static const std::vector<Operation> operations = {
            { "dot", "Dot products between two arrays of Vector3",
              {
                  { "ref-aos", "C++ reference, AoS (Vector3::Dot in a loop)", DotRefAoS, Output::Floats, true },
                  { "simd-aos", "SSE, AoS, 4 dot products per iteration (transposition)", DotSimdAoS, Output::Floats, true },
                  { "asm-aos", "hand-written x64 assembly, scalar loop", DotAsmAoS, Output::Floats, true },
                  { "ref-soa", "C++ reference, SoA", DotRefSoA, Output::Floats, true },
                  { "simd-soa", "SSE, SoA, no transposition", DotSimdSoA, Output::Floats, true },
                  { "simd-soa+conv", "AoS->SoA conversion of both inputs + SSE SoA", DotSimdSoAWithConversion, Output::Floats, true },
              },
              kVectorSizes, true },
            { "normalize", "Normalization of an array of Vector3",
              {
                  { "ref-aos", "C++ reference, AoS (Vector3::Normalized in a loop)", NormRefAoS, Output::Vectors, true },
                  { "simd-aos", "SSE, AoS, sqrt + div, mask for zero vectors", NormSimdAoS, Output::Vectors, true },
                  { "ref-soa", "C++ reference, SoA", NormRefSoA, Output::VectorsSoA, true },
                  { "simd-soa", "SSE, SoA", NormSimdSoA, Output::VectorsSoA, true },
                  { "simd-soa+conv", "AoS->SoA + SSE SoA + SoA->AoS", NormSimdSoAWithConversion, Output::Vectors, true },
                  { "simd-aos-approx", "(extension) SSE AoS with rsqrt (approximate)", NormSimdAoSApprox, Output::Vectors, false },
              },
              kVectorSizes, true },
            { "transform", "Transformation of 3D points by the same affine Mat4 (w = 1)",
              {
                  { "ref-aos", "C++ reference, AoS (Mat4::MultiplyPointAffine in a loop)", TransRefAoS, Output::Vectors, true },
                  { "simd-aos", "SSE, AoS, 4 points per iteration (transposition)", TransSimdAoS, Output::Vectors, true },
                  { "simd-aos-naive", "SSE, AoS, 1 point per iteration (lanes = x', y', z', w')", TransSimdAoSNaive, Output::Vectors, true },
                  { "ref-soa", "C++ reference, SoA", TransRefSoA, Output::VectorsSoA, true },
                  { "simd-soa", "SSE, SoA", TransSimdSoA, Output::VectorsSoA, true },
                  { "simd-soa+conv", "AoS->SoA + SSE SoA + SoA->AoS", TransSimdSoAWithConversion, Output::Vectors, true },
              },
              kVectorSizes, true },
            { "convert", "AoS <-> SoA conversions alone (cost of changing the layout)",
              {
                  { "aos-to-soa", "AoS -> SoA (plain C++)", ConvAoSToSoA, Output::None, true },
                  { "soa-to-aos", "SoA -> AoS (plain C++)", ConvSoAToAoS, Output::None, true },
              },
              kVectorSizes, false },
            { "mat4mul", "(extension) Products of two arrays of Mat4",
              {
                  { "ref", "C++ reference (Mat4::operator*)", MatMulRef, Output::Matrices, true },
                  { "simd", "SSE, one row of the result per register", MatMulSimd, Output::Matrices, true },
              },
              kMatrixSizes, true },
            { "vec2dot", "Sum of the dot products between two arrays of Vector2",
              {
                  { "ref-aos", "C++ reference, AoS (Vector2::Dot in a loop)", Vec2DotRefAoS, Output::Scalar, true },
                  { "simd-aos", "SSE, AoS, 1 dot product per call (Vector2SSE::Dot)", Vec2DotSimdAoS, Output::Scalar, true },
                  { "ref-soa", "C++ reference, SoA", Vec2DotRefSoA, Output::Scalar, true },
                  { "simd-soa", "SSE, SoA (Vector2SSE::DotSoA)", Vec2DotSimdSoA, Output::Scalar, true },
              },
              kVectorSizes, true },
            { "vec2scale", "Component-wise products of two arrays of Vector2",
              {
                  { "ref-aos", "C++ reference, AoS (Vector2::Scale in a loop)", Vec2ScaleRefAoS, Output::Vectors2, true },
                  { "simd-aos", "SSE, AoS, 1 vector per call (Vector2SSE::Scale)", Vec2ScaleSimdAoS, Output::Vectors2, true },
                  { "ref-soa", "C++ reference, SoA", Vec2ScaleRefSoA, Output::Vectors2SoA, true },
                  { "simd-soa", "SSE, SoA (Vector2SSE::ScaleSoA)", Vec2ScaleSimdSoA, Output::Vectors2SoA, true },
              },
              kVectorSizes, true },
            { "vec2lerp", "Linear interpolations (t = 0.5) between two arrays of Vector2",
              {
                  { "ref-aos", "C++ reference, AoS (Vector2::Lerp in a loop)", Vec2LerpRefAoS, Output::Vectors2, true },
                  { "simd-aos", "SSE, AoS, 1 vector per call (Vector2SSE::Lerp)", Vec2LerpSimdAoS, Output::Vectors2, true },
                  { "ref-soa", "C++ reference, SoA", Vec2LerpRefSoA, Output::Vectors2SoA, true },
                  { "simd-soa", "SSE, SoA (Vector2SSE::LerpSoA)", Vec2LerpSimdSoA, Output::Vectors2SoA, true },
              },
              kVectorSizes, true },
            { "mat3det", "Sum of the determinants of an array of Mat3",
              {
                  { "ref-aos", "C++ reference, AoS (Mat3::Determinant in a loop)", Mat3DetRefAoS, Output::Scalar, true },
                  { "simd-aos", "AoS, Mat3SSE::Determinant in a loop", Mat3DetSimdAoS, Output::Scalar, true },
                  { "ref-soa", "C++ reference, SoA (Mat3SSE::DeterminantSoA)", Mat3DetRefSoA, Output::Scalar, true },
                  // The 4 determinants of a register are added together before the running sum: other rounding.
                  { "simd-soa", "SSE, SoA (Mat3SSE::DeterminantSoASIMD)", Mat3DetSimdSoA, Output::Scalar, false },
              },
              kMatrixSizes, true },
            { "mat3transpose", "Transposition of an array of Mat3",
              {
                  { "ref-aos", "C++ reference, AoS (Mat3SSE::Transpose in a loop)", Mat3TransRefAoS, Output::Matrices3, true },
                  { "simd-aos", "SSE, AoS, _MM_TRANSPOSE4_PS (Mat3SSE::TransposeSIMD)", Mat3TransSimdAoS, Output::Matrices3, true },
                  { "ref-soa", "C++ reference, SoA (Mat3SSE::TransposeSoA)", Mat3TransRefSoA, Output::Matrices3SoA, true },
                  { "simd-soa", "SSE, SoA (Mat3SSE::TransposeSoASIMD)", Mat3TransSimdSoA, Output::Matrices3SoA, true },
              },
              kMatrixSizes, true },
            { "mat3mul", "Products of two arrays of Mat3",
              {
                  { "ref-aos", "C++ reference, AoS (Mat3SSE::Multiply in a loop)", Mat3MulRefAoS, Output::Matrices3, true },
                  { "simd-aos", "SSE, AoS, one dot product per coefficient (Mat3SSE::MultiplySIMD)", Mat3MulSimdAoS, Output::Matrices3, true },
                  { "ref-soa", "C++ reference, SoA (Mat3SSE::MultiplySoA)", Mat3MulRefSoA, Output::Matrices3SoA, true },
                  { "simd-soa", "SSE, SoA (Mat3SSE::MultiplySoASIMD)", Mat3MulSimdSoA, Output::Matrices3SoA, true },
              },
              kMatrixSizes, true },
        };
        return operations;
    }

    const Operation* FindOperation(const std::string& name)
    {
        for (const Operation& op : AllOperations())
            if (op.name == name)
                return &op;
        return nullptr;
    }

    void PrepareWorkspace(Workspace& w, const Operation& op, std::size_t n, std::uint32_t seed)
    {
        Rng rng(seed);
        w = Workspace{};
        w.n = n;

        if (op.name == "mat4mul")
        {
            w.matA = RandomMatrices(n, rng);
            w.matB = RandomMatrices(n, rng);
            w.outMatrices.resize(n);
            return;
        }

        if (op.name.rfind("vec2", 0) == 0)
        {
            w.v2A = RandomVectors2(n, rng);
            w.v2B = RandomVectors2(n, rng);
            ToSoA(w.v2A, w.v2SoaA);
            ToSoA(w.v2B, w.v2SoaB);
            w.outV2.resize(n);
            w.outV2SoA.x.resize(n);
            w.outV2SoA.y.resize(n);
            return;
        }

        if (op.name.rfind("mat3", 0) == 0)
        {
            w.m3A = RandomMatrices3(n, rng);
            w.m3B = RandomMatrices3(n, rng);
            ToSoA(w.m3A, w.m3SoaA);
            ToSoA(w.m3B, w.m3SoaB);
            w.outM3.resize(n);
            w.outM3SoA.Resize(n);
            return;
        }

        w.matrix = RandomAffine(rng);
        w.a = RandomVectors(n, rng);
        w.b = RandomVectors(n, rng);
        w.soaA.Resize(n);
        w.soaB.Resize(n);
        AoSToSoA(w.a.data(), w.soaA, n);
        AoSToSoA(w.b.data(), w.soaB, n);

        w.outFloats.resize(n);
        w.outVectors.resize(n);
        w.outSoA.Resize(n);
        w.tmpA.Resize(n);
        w.tmpB.Resize(n);
    }

    std::vector<float> ReadOutput(const Workspace& w, Output output)
    {
        std::vector<float> values;
        switch (output)
        {
        case Output::Floats:
            values = w.outFloats;
            break;
        case Output::Vectors:
            for (const Vec3f& v : w.outVectors)
            {
                values.push_back(v.x);
                values.push_back(v.y);
                values.push_back(v.z);
            }
            break;
        case Output::VectorsSoA:
            for (std::size_t i = 0; i < w.n; ++i)
            {
                values.push_back(w.outSoA.x[i]);
                values.push_back(w.outSoA.y[i]);
                values.push_back(w.outSoA.z[i]);
            }
            break;
        case Output::Matrices:
            for (const Mat4f& m : w.outMatrices)
                for (int r = 0; r < 4; ++r)
                    for (int c = 0; c < 4; ++c)
                        values.push_back(m.m[r][c]);
            break;
        case Output::Scalar:
            values.push_back(w.outScalar);
            break;
        case Output::Vectors2:
            for (const Vec2f& v : w.outV2)
            {
                values.push_back(v.x);
                values.push_back(v.y);
            }
            break;
        case Output::Vectors2SoA:
            for (std::size_t i = 0; i < w.n; ++i)
            {
                values.push_back(w.outV2SoA.x[i]);
                values.push_back(w.outV2SoA.y[i]);
            }
            break;
        case Output::Matrices3:
            for (const Mat3f& m : w.outM3)
                for (int r = 0; r < 3; ++r)
                    for (int c = 0; c < 3; ++c)
                        values.push_back(m.m[r][c]);
            break;
        case Output::Matrices3SoA:
        {
            const math::Mat3SSE::Mat3SoA& s = w.outM3SoA;
            for (std::size_t i = 0; i < w.n; ++i)
                for (const std::vector<float>* e : { &s.m00, &s.m01, &s.m02, &s.m10, &s.m11, &s.m12, &s.m20, &s.m21, &s.m22 })
                    values.push_back((*e)[i]);
            break;
        }
        case Output::None:
            break;
        }
        return values;
    }

    double Checksum(const std::vector<float>& values)
    {
        double sum = 0.0;
        for (float v : values)
            sum += v;
        return sum;
    }
}
