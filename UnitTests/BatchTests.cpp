#include "pch.h"
#include "CppUnitTest.h"

#include "TestHelpers.h"
#include "AsmFunctions.h"
#include "Quaternion.h"

#include <numbers>
#include <utility>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace math;
using namespace test;

/*
 * Tests of the batch processing (Vector3Batch.h, Mat4Batch.h) and of the ASM functions.
 *
 * Strategy (the agreement between two versions does NOT prove that they are correct):
 *  1. known results computed by hand (exact values when the float result is exact);
 *  2. random data compared to the same formula computed in double, with an error bound
 *     derived from the number of float operations (see TestHelpers.h);
 *  3. ref and simd compared to each other: same operations in the same order, so the values
 *     must be identical (== comparison, so +0 and -0 are considered equal);
 *  4. edge cases: empty batch, sizes 0..40 (all remainders), zero vectors, in-place processing,
 *     and out-of-bounds accesses (guard page + sentinels).
 */
namespace BatchTests
{
    using FloatResults = std::vector<std::pair<std::wstring, std::vector<float>>>;
    using VectorResults = std::vector<std::pair<std::wstring, std::vector<Vec3f>>>;

    // ====================================================================================
    // Helpers that run every version of a processing on the same input
    // ====================================================================================

    FloatResults RunAllDot(const std::vector<Vec3f>& a, const std::vector<Vec3f>& b)
    {
        const std::size_t n = a.size();
        Vec3SoA sa(n), sb(n);
        AoSToSoA(a.data(), sa, n);
        AoSToSoA(b.data(), sb, n);

        FloatResults results;
        std::vector<float> out(n);

        ref::DotBatch(a.data(), b.data(), out.data(), n);
        results.push_back({ L"ref AoS", out });
        simd::DotBatch(a.data(), b.data(), out.data(), n);
        results.push_back({ L"simd AoS", out });
        ref::DotBatchSoA(sa, sb, out.data(), n);
        results.push_back({ L"ref SoA", out });
        simd::DotBatchSoA(sa, sb, out.data(), n);
        results.push_back({ L"simd SoA", out });
        x64asm::DotBatch(a.data(), b.data(), out.data(), n);
        results.push_back({ L"asm batch", out });
        for (std::size_t i = 0; i < n; ++i)
            out[i] = x64asm::Dot(a[i], b[i]);
        results.push_back({ L"asm Dot3", out });
        return results;
    }

    VectorResults RunAllNormalize(const std::vector<Vec3f>& in)
    {
        const std::size_t n = in.size();
        Vec3SoA sin(n), sout(n);
        AoSToSoA(in.data(), sin, n);

        VectorResults results;
        std::vector<Vec3f> out(n);

        ref::NormalizeBatch(in.data(), out.data(), n);
        results.push_back({ L"ref AoS", out });
        simd::NormalizeBatch(in.data(), out.data(), n);
        results.push_back({ L"simd AoS", out });
        ref::NormalizeBatchSoA(sin, sout, n);
        SoAToAoS(sout, out.data(), n);
        results.push_back({ L"ref SoA", out });
        simd::NormalizeBatchSoA(sin, sout, n);
        SoAToAoS(sout, out.data(), n);
        results.push_back({ L"simd SoA", out });
        return results;
    }

    VectorResults RunAllTransform(const Mat4f& m, const std::vector<Vec3f>& in)
    {
        const std::size_t n = in.size();
        Vec3SoA sin(n), sout(n);
        AoSToSoA(in.data(), sin, n);

        VectorResults results;
        std::vector<Vec3f> out(n);

        ref::TransformPointsBatch(m, in.data(), out.data(), n);
        results.push_back({ L"ref AoS", out });
        simd::TransformPointsBatch(m, in.data(), out.data(), n);
        results.push_back({ L"simd AoS", out });
        simd::TransformPointsBatchNaive(m, in.data(), out.data(), n);
        results.push_back({ L"simd AoS naive", out });
        ref::TransformPointsBatchSoA(m, sin, sout, n);
        SoAToAoS(sout, out.data(), n);
        results.push_back({ L"ref SoA", out });
        simd::TransformPointsBatchSoA(m, sin, sout, n);
        SoAToAoS(sout, out.data(), n);
        results.push_back({ L"simd SoA", out });
        return results;
    }

    void AssertVecEqual(const Vec3f& expected, const Vec3f& actual, const std::wstring& label)
    {
        Assert::AreEqual(expected.x, actual.x, (label + L" x").c_str());
        Assert::AreEqual(expected.y, actual.y, (label + L" y").c_str());
        Assert::AreEqual(expected.z, actual.z, (label + L" z").c_str());
    }

    void AssertVecNear(const Vec3f& expected, const Vec3f& actual, const std::wstring& label)
    {
        Assert::IsTrue(NearlyEqual(expected.x, actual.x, kAngleAbsTol, kAngleRelTol), (label + L" x").c_str());
        Assert::IsTrue(NearlyEqual(expected.y, actual.y, kAngleAbsTol, kAngleRelTol), (label + L" y").c_str());
        Assert::IsTrue(NearlyEqual(expected.z, actual.z, kAngleAbsTol, kAngleRelTol), (label + L" z").c_str());
    }

    Mat4f RotationZ90()
    {
        return Mat4f::Rotate(Quaternion::FromAxisAngle(Vec3f(0.0f, 0.0f, 1.0f), std::numbers::pi_v<float> / 2.0f));
    }

    /** Affine matrix with a rotation around a random axis, a uniform scale and a translation. */
    Mat4f RandomAffine(Rng& rng)
    {
        Vec3f axis = rng.Vector(1.0f).Normalized();
        if (axis.LengthSquared() == 0.0f)
            axis = Vec3f(0.0f, 0.0f, 1.0f);
        const float angle = rng.Uniform(-3.14159265f, 3.14159265f);
        const float s = rng.Uniform(0.5f, 2.0f);
        return Mat4f::TRS(rng.Vector(50.0f), Quaternion::FromAxisAngle(axis, angle), Vec3f(s, s, s));
    }

    // ====================================================================================
    // Dot product
    // ====================================================================================
    TEST_CLASS(DotBatchTests)
    {
    public:
        TEST_METHOD(KnownValues)
        {
            // 6 vectors: one SIMD block of 4 + a remainder of 2, so both code paths are tested.
            // All values are small integers or powers of 2: the float results are exact.
            const std::vector<Vec3f> a = { {1, 2, 3}, {1, 0, 0}, {-1, 2, -3}, {0.5f, 0.25f, 2}, {1, 2, 3}, {1e3f, 0, 0} };
            const std::vector<Vec3f> b = { {4, 5, 6}, {0, 1, 0}, { 4, -5, 6}, {4, 8, 0.5f},     {-1, -2, -3}, {1e3f, 0, 0} };
            const std::vector<float> expected = { 32.0f, 0.0f, -32.0f, 5.0f, -14.0f, 1e6f };

            for (const auto& [name, out] : RunAllDot(a, b))
                for (std::size_t i = 0; i < a.size(); ++i)
                    Assert::AreEqual(expected[i], out[i], Label(name, a.size(), i).c_str());
        }

        TEST_METHOD(ZeroVectors)
        {
            const std::vector<Vec3f> a = { {0, 0, 0}, {1, 2, 3}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} };
            const std::vector<Vec3f> b = { {1, 2, 3}, {0, 0, 0}, {0, 0, 0}, {-7, 8, 9}, {0, 0, 0} };

            for (const auto& [name, out] : RunAllDot(a, b))
                for (std::size_t i = 0; i < a.size(); ++i)
                    Assert::AreEqual(0.0f, out[i], Label(name, a.size(), i).c_str());
        }

        TEST_METHOD(EmptyBatch)
        {
            // n = 0 with null pointers: nothing must be read or written.
            ref::DotBatch(nullptr, nullptr, nullptr, 0);
            simd::DotBatch(nullptr, nullptr, nullptr, 0);
            x64asm::DotBatch(nullptr, nullptr, nullptr, 0);
            Vec3SoA empty;
            ref::DotBatchSoA(empty, empty, nullptr, 0);
            simd::DotBatchSoA(empty, empty, nullptr, 0);
        }

        TEST_METHOD(AllSizesAgainstDouble)
        {
            Rng rng(1234);
            for (std::size_t n : TestSizes())
            {
                const std::vector<Vec3f> a = RandomVectors(n, rng);
                const std::vector<Vec3f> b = RandomVectors(n, rng);
                for (const auto& [name, out] : RunAllDot(a, b))
                {
                    for (std::size_t i = 0; i < n; ++i)
                    {
                        double bound = 0.0;
                        const double exact = DotDouble(a[i], b[i], bound);
                        Assert::IsTrue(std::fabs(out[i] - exact) <= bound, Label(name, n, i).c_str());
                    }
                }
            }
        }

        TEST_METHOD(AllVersionsGiveIdenticalValues)
        {
            Rng rng(99);
            for (std::size_t n : TestSizes())
            {
                const FloatResults results = RunAllDot(RandomVectors(n, rng), RandomVectors(n, rng));
                const std::vector<float>& reference = results.front().second;
                for (const auto& [name, out] : results)
                    for (std::size_t i = 0; i < n; ++i)
                        Assert::AreEqual(reference[i], out[i], Label(name, n, i).c_str());
            }
        }

        TEST_METHOD(NoOutOfBoundsAccess)
        {
            // AoS inputs and outputs end just before a no-access page: any access past the end
            // crashes the test. Sizes 0..9 cover the empty batch and every remainder.
            Rng rng(7);
            for (std::size_t n = 0; n <= 9; ++n)
            {
                GuardedArray<Vec3f> a(RandomVectors(n, rng));
                GuardedArray<Vec3f> b(RandomVectors(n, rng));
                GuardedArray<float> out(n);
                ref::DotBatch(a.Data(), b.Data(), out.Data(), n);
                simd::DotBatch(a.Data(), b.Data(), out.Data(), n);
                x64asm::DotBatch(a.Data(), b.Data(), out.Data(), n);
                for (std::size_t i = 0; i < n; ++i)
                    Assert::AreEqual(a[i].Dot(b[i]), out[i]);
            }
        }

        TEST_METHOD(SoAWritesOnlyNResults)
        {
            Rng rng(8);
            for (std::size_t n = 0; n <= 9; ++n)
            {
                Vec3SoA a(n), b(n);
                AoSToSoA(RandomVectors(n, rng).data(), a, n);
                AoSToSoA(RandomVectors(n, rng).data(), b, n);
                std::vector<float> out(n + 4, kSentinel);
                simd::DotBatchSoA(a, b, out.data(), n);
                for (std::size_t i = n; i < n + 4; ++i)
                    Assert::AreEqual(kSentinel, out[i], Label(L"simd SoA sentinel", n, i).c_str());
            }
        }
    };

    // ====================================================================================
    // Normalization
    // ====================================================================================
    TEST_CLASS(NormalizeBatchTests)
    {
    public:
        TEST_METHOD(KnownValues)
        {
            // (3, 4, 0) has length 5 exactly, 3/5 and 4/5 are correctly rounded -> exactly 0.6f and 0.8f.
            const std::vector<Vec3f> in = { {3, 4, 0}, {0, 0, 5}, {-2, 0, 0}, {0, -0.5f, 0}, {0, 3, -4}, {8, 0, 0} };
            const std::vector<Vec3f> expected = { {0.6f, 0.8f, 0}, {0, 0, 1}, {-1, 0, 0}, {0, -1, 0}, {0, 0.6f, -0.8f}, {1, 0, 0} };

            for (const auto& [name, out] : RunAllNormalize(in))
                for (std::size_t i = 0; i < in.size(); ++i)
                    AssertVecEqual(expected[i], out[i], Label(name, in.size(), i));
        }

        TEST_METHOD(ZeroVectorGivesZero)
        {
            // Zero vectors placed in the SIMD block (indices 0, 2) and in the remainder (index 5).
            // Also NaN and a tiny vector whose squared length underflows to 0: documented as zero.
            const float nan = std::numeric_limits<float>::quiet_NaN();
            const std::vector<Vec3f> in = { {0, 0, 0}, {1, 0, 0}, {0, 0, 0}, {nan, 1, 2}, {1e-30f, 0, 0}, {0, 0, 0}, {0, 2, 0} };

            for (const auto& [name, out] : RunAllNormalize(in))
            {
                for (std::size_t i : { 0u, 2u, 3u, 4u, 5u })
                    AssertVecEqual(Vec3f(0, 0, 0), out[i], Label(name, in.size(), i));
                AssertVecEqual(Vec3f(1, 0, 0), out[1], Label(name, in.size(), 1));
                AssertVecEqual(Vec3f(0, 1, 0), out[6], Label(name, in.size(), 6));
            }
        }

        TEST_METHOD(AllSizesAgainstDouble)
        {
            Rng rng(4321);
            for (std::size_t n : TestSizes())
            {
                const std::vector<Vec3f> in = RandomVectors(n, rng);
                for (const auto& [name, out] : RunAllNormalize(in))
                {
                    for (std::size_t i = 0; i < n; ++i)
                    {
                        double exact[3];
                        NormalizeDouble(in[i], exact);
                        Assert::IsTrue(std::fabs(out[i].x - exact[0]) <= kNormalizeAbsTol, Label(name, n, i).c_str());
                        Assert::IsTrue(std::fabs(out[i].y - exact[1]) <= kNormalizeAbsTol, Label(name, n, i).c_str());
                        Assert::IsTrue(std::fabs(out[i].z - exact[2]) <= kNormalizeAbsTol, Label(name, n, i).c_str());
                    }
                }
            }
        }

        TEST_METHOD(AllVersionsGiveIdenticalValues)
        {
            Rng rng(5);
            for (std::size_t n : TestSizes())
            {
                const VectorResults results = RunAllNormalize(RandomVectors(n, rng));
                const std::vector<Vec3f>& reference = results.front().second;
                for (const auto& [name, out] : results)
                    for (std::size_t i = 0; i < n; ++i)
                        AssertVecEqual(reference[i], out[i], Label(name, n, i));
            }
        }

        TEST_METHOD(InPlace)
        {
            Rng rng(6);
            const std::vector<Vec3f> in = RandomVectors(11, rng);
            std::vector<Vec3f> expected(in.size());
            ref::NormalizeBatch(in.data(), expected.data(), in.size());

            std::vector<Vec3f> data = in;
            simd::NormalizeBatch(data.data(), data.data(), data.size());
            for (std::size_t i = 0; i < data.size(); ++i)
                AssertVecEqual(expected[i], data[i], Label(L"simd AoS in-place", data.size(), i));
        }

        TEST_METHOD(NoOutOfBoundsAccess)
        {
            Rng rng(9);
            for (std::size_t n = 0; n <= 9; ++n)
            {
                GuardedArray<Vec3f> in(RandomVectors(n, rng));
                GuardedArray<Vec3f> out(n);
                ref::NormalizeBatch(in.Data(), out.Data(), n);
                simd::NormalizeBatch(in.Data(), out.Data(), n);
                simd::NormalizeBatchApprox(in.Data(), out.Data(), n);
            }
        }

        TEST_METHOD(SoAWritesOnlyNResults)
        {
            Rng rng(10);
            for (std::size_t n = 0; n <= 9; ++n)
            {
                Vec3SoA in(n), out(n + 4);
                AoSToSoA(RandomVectors(n, rng).data(), in, n);
                std::fill(out.x.begin(), out.x.end(), kSentinel);
                std::fill(out.y.begin(), out.y.end(), kSentinel);
                std::fill(out.z.begin(), out.z.end(), kSentinel);
                simd::NormalizeBatchSoA(in, out, n);
                for (std::size_t i = n; i < n + 4; ++i)
                {
                    Assert::AreEqual(kSentinel, out.x[i]);
                    Assert::AreEqual(kSentinel, out.y[i]);
                    Assert::AreEqual(kSentinel, out.z[i]);
                }
            }
        }

        TEST_METHOD(ApproxErrorIsBounded)
        {
            // Optional extension: rsqrt has a relative error <= 1.5 * 2^-12 (Intel documentation).
            // Each component is v * rsqrt(lenSq): its relative error is that bound + a few roundings.
            const double bound = 1.5 / 4096.0 + 8.0 * kU;
            Rng rng(11);
            for (std::size_t n : TestSizes())
            {
                const std::vector<Vec3f> in = RandomVectors(n, rng);
                std::vector<Vec3f> out(n);
                simd::NormalizeBatchApprox(in.data(), out.data(), n);
                for (std::size_t i = 0; i < n; ++i)
                {
                    double exact[3];
                    NormalizeDouble(in[i], exact);
                    Assert::IsTrue(NearlyEqual(out[i].x, exact[0], 1e-30, bound), Label(L"approx", n, i).c_str());
                    Assert::IsTrue(NearlyEqual(out[i].y, exact[1], 1e-30, bound), Label(L"approx", n, i).c_str());
                    Assert::IsTrue(NearlyEqual(out[i].z, exact[2], 1e-30, bound), Label(L"approx", n, i).c_str());
                }
            }

            const std::vector<Vec3f> zeros = { {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} };
            std::vector<Vec3f> out(zeros.size());
            simd::NormalizeBatchApprox(zeros.data(), out.data(), zeros.size());
            for (std::size_t i = 0; i < out.size(); ++i)
                AssertVecEqual(Vec3f(0, 0, 0), out[i], Label(L"approx zero", out.size(), i));
        }
    };

    // ====================================================================================
    // Transformation of points by an affine Mat4 (w = 1, no perspective divide)
    // ====================================================================================
    TEST_CLASS(TransformBatchTests)
    {
    public:
        TEST_METHOD(Identity)
        {
            const std::vector<Vec3f> in = { {1, 2, 3}, {-4, 5.5f, 0}, {0, 0, 0}, {1e6f, -1e-6f, 7}, {9, 8, 7} };
            for (const auto& [name, out] : RunAllTransform(Mat4f::Identity(), in))
                for (std::size_t i = 0; i < in.size(); ++i)
                    AssertVecEqual(in[i], out[i], Label(name, in.size(), i));
        }

        TEST_METHOD(Translation)
        {
            const Mat4f t = Mat4f::Translate(Vec3f(10, 20, 30));
            const std::vector<Vec3f> in = { {1, 2, 3}, {0, 0, 0}, {-10, -20, -30}, {0.5f, 0.25f, 0.125f}, {100, 0, -1} };
            const std::vector<Vec3f> expected = { {11, 22, 33}, {10, 20, 30}, {0, 0, 0}, {10.5f, 20.25f, 30.125f}, {110, 20, 29} };
            for (const auto& [name, out] : RunAllTransform(t, in))
                for (std::size_t i = 0; i < in.size(); ++i)
                    AssertVecEqual(expected[i], out[i], Label(name, in.size(), i));
        }

        TEST_METHOD(Scale)
        {
            const Mat4f s = Mat4f::Scale(Vec3f(2, -3, 0.5f));
            const std::vector<Vec3f> in = { {1, 1, 1}, {0, 2, 4}, {-1, 0, 8}, {3, 3, 3}, {0, 0, 0}, {1, 2, 3} };
            const std::vector<Vec3f> expected = { {2, -3, 0.5f}, {0, -6, 2}, {-2, 0, 4}, {6, -9, 1.5f}, {0, 0, 0}, {2, -6, 1.5f} };
            for (const auto& [name, out] : RunAllTransform(s, in))
                for (std::size_t i = 0; i < in.size(); ++i)
                    AssertVecEqual(expected[i], out[i], Label(name, in.size(), i));
        }

        TEST_METHOD(RotationAroundZ)
        {
            // Right-handed rotation of +90 degrees around Z: X -> Y, Y -> -X, Z unchanged.
            const std::vector<Vec3f> in = { {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {2, 3, 4}, {-1, 0, 0} };
            const std::vector<Vec3f> expected = { {0, 1, 0}, {-1, 0, 0}, {0, 0, 1}, {-3, 2, 4}, {0, -1, 0} };
            for (const auto& [name, out] : RunAllTransform(RotationZ90(), in))
                for (std::size_t i = 0; i < in.size(); ++i)
                    AssertVecNear(expected[i], out[i], Label(name, in.size(), i));
        }

        TEST_METHOD(CompositionOrder)
        {
            // Row vectors: p * A * B applies A first, then B.
            const Mat4f t = Mat4f::Translate(Vec3f(1, 0, 0));
            const Mat4f r = RotationZ90();
            const std::vector<Vec3f> in = { {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0} };

            // translate then rotate: (1,0,0) -> (2,0,0) -> (0,2,0)
            for (const auto& [name, out] : RunAllTransform(t * r, in))
                for (std::size_t i = 0; i < in.size(); ++i)
                    AssertVecNear(Vec3f(0, 2, 0), out[i], Label(L"T*R " + name, in.size(), i));

            // rotate then translate: (1,0,0) -> (0,1,0) -> (1,1,0)
            for (const auto& [name, out] : RunAllTransform(r * t, in))
                for (std::size_t i = 0; i < in.size(); ++i)
                    AssertVecNear(Vec3f(1, 1, 0), out[i], Label(L"R*T " + name, in.size(), i));
        }

        TEST_METHOD(CompositionEqualsSuccessiveTransforms)
        {
            // Transforming by S*R*T in one pass = transforming by S, then R, then T.
            Rng rng(12);
            const Mat4f s = Mat4f::Scale(Vec3f(2, 0.5f, 3));
            const Mat4f r = Mat4f::Rotate(Quaternion::FromAxisAngle(Vec3f(1, 1, 0), 0.7f));
            const Mat4f t = Mat4f::Translate(Vec3f(-4, 5, 6));
            const std::vector<Vec3f> in = RandomVectors(13, rng, 10.0f);

            std::vector<Vec3f> step(in.size()), composed(in.size());
            simd::TransformPointsBatch(s, in.data(), step.data(), in.size());
            simd::TransformPointsBatch(r, step.data(), step.data(), in.size()); // in place
            simd::TransformPointsBatch(t, step.data(), step.data(), in.size());
            simd::TransformPointsBatch(s * r * t, in.data(), composed.data(), in.size());

            for (std::size_t i = 0; i < in.size(); ++i)
            {
                // Values up to ~100: the absolute tolerance is scaled (relative 1e-5, see report).
                Assert::IsTrue(NearlyEqual(step[i].x, composed[i].x, 1e-4, 1e-5), Label(L"x", in.size(), i).c_str());
                Assert::IsTrue(NearlyEqual(step[i].y, composed[i].y, 1e-4, 1e-5), Label(L"y", in.size(), i).c_str());
                Assert::IsTrue(NearlyEqual(step[i].z, composed[i].z, 1e-4, 1e-5), Label(L"z", in.size(), i).c_str());
            }
        }

        TEST_METHOD(ColumnThreeIsIgnored)
        {
            // Affine assumption: w = 1 and no perspective divide, column 3 is never read.
            Mat4f m = Mat4f::Translate(Vec3f(1, 2, 3));
            m.m[0][3] = 5.0f;
            m.m[3][3] = 9.0f;
            const std::vector<Vec3f> in = { {1, 1, 1}, {0, 0, 0}, {2, 2, 2}, {1, 0, 0}, {0, 1, 0} };
            for (const auto& [name, out] : RunAllTransform(m, in))
                for (std::size_t i = 0; i < in.size(); ++i)
                    AssertVecEqual(in[i] + Vec3f(1, 2, 3), out[i], Label(name, in.size(), i));
        }

        TEST_METHOD(AllSizesAgainstDouble)
        {
            Rng rng(2024);
            for (std::size_t n : TestSizes())
            {
                const Mat4f m = RandomAffine(rng);
                const std::vector<Vec3f> in = RandomVectors(n, rng);
                for (const auto& [name, out] : RunAllTransform(m, in))
                {
                    for (std::size_t i = 0; i < n; ++i)
                    {
                        double exact[3], bound[3];
                        TransformDouble(m, in[i], exact, bound);
                        Assert::IsTrue(std::fabs(out[i].x - exact[0]) <= bound[0], Label(name, n, i).c_str());
                        Assert::IsTrue(std::fabs(out[i].y - exact[1]) <= bound[1], Label(name, n, i).c_str());
                        Assert::IsTrue(std::fabs(out[i].z - exact[2]) <= bound[2], Label(name, n, i).c_str());
                    }
                }
            }
        }

        TEST_METHOD(AllVersionsGiveIdenticalValues)
        {
            Rng rng(77);
            for (std::size_t n : TestSizes())
            {
                const VectorResults results = RunAllTransform(RandomAffine(rng), RandomVectors(n, rng));
                const std::vector<Vec3f>& reference = results.front().second;
                for (const auto& [name, out] : results)
                    for (std::size_t i = 0; i < n; ++i)
                        AssertVecEqual(reference[i], out[i], Label(name, n, i));
            }
        }

        TEST_METHOD(InPlace)
        {
            Rng rng(13);
            const Mat4f m = RandomAffine(rng);
            const std::vector<Vec3f> in = RandomVectors(10, rng);
            std::vector<Vec3f> expected(in.size());
            ref::TransformPointsBatch(m, in.data(), expected.data(), in.size());

            std::vector<Vec3f> data = in;
            simd::TransformPointsBatch(m, data.data(), data.data(), data.size());
            for (std::size_t i = 0; i < data.size(); ++i)
                AssertVecEqual(expected[i], data[i], Label(L"simd AoS in-place", data.size(), i));
        }

        TEST_METHOD(EmptyBatch)
        {
            const Mat4f m = Mat4f::Identity();
            ref::TransformPointsBatch(m, nullptr, nullptr, 0);
            simd::TransformPointsBatch(m, nullptr, nullptr, 0);
            simd::TransformPointsBatchNaive(m, nullptr, nullptr, 0);
            Vec3SoA empty, emptyOut;
            ref::TransformPointsBatchSoA(m, empty, emptyOut, 0);
            simd::TransformPointsBatchSoA(m, empty, emptyOut, 0);
        }

        TEST_METHOD(NoOutOfBoundsAccess)
        {
            Rng rng(14);
            const Mat4f m = RandomAffine(rng);
            for (std::size_t n = 0; n <= 9; ++n)
            {
                GuardedArray<Vec3f> in(RandomVectors(n, rng));
                GuardedArray<Vec3f> out(n);
                ref::TransformPointsBatch(m, in.Data(), out.Data(), n);
                simd::TransformPointsBatch(m, in.Data(), out.Data(), n);
                simd::TransformPointsBatchNaive(m, in.Data(), out.Data(), n);
            }
        }
    };

    // ====================================================================================
    // Hand-written assembly
    // ====================================================================================
    TEST_CLASS(AsmTests)
    {
    public:
        TEST_METHOD(Dot3KnownValues)
        {
            Assert::AreEqual(32.0f, x64asm::Dot(Vec3f(1, 2, 3), Vec3f(4, 5, 6)));
            Assert::AreEqual(0.0f, x64asm::Dot(Vec3f(1, 0, 0), Vec3f(0, 1, 0)));
            Assert::AreEqual(0.0f, x64asm::Dot(Vec3f(0, 0, 0), Vec3f(7, 8, 9)));
            Assert::AreEqual(-14.0f, x64asm::Dot(Vec3f(1, 2, 3), Vec3f(-1, -2, -3)));
        }

        TEST_METHOD(Dot3SameAsCpp)
        {
            // Same operations in the same order as Vector3::Dot -> identical result.
            Rng rng(15);
            for (int i = 0; i < 10000; ++i)
            {
                const Vec3f a = rng.Vector(1000.0f);
                const Vec3f b = rng.Vector(1000.0f);
                Assert::AreEqual(a.Dot(b), x64asm::Dot(a, b));
            }
        }
    };

    // ====================================================================================
    // Optional extension: Mat4 x Mat4 with SSE
    // ====================================================================================
    TEST_CLASS(Mat4MultiplySimdTests)
    {
    public:
        static void AssertMatEqual(const Mat4f& expected, const Mat4f& actual)
        {
            for (int r = 0; r < 4; ++r)
                for (int c = 0; c < 4; ++c)
                    Assert::AreEqual(expected.m[r][c], actual.m[r][c], (L"element " + std::to_wstring(r) + L"," + std::to_wstring(c)).c_str());
        }

        TEST_METHOD(KnownProduct)
        {
            const Mat4f a(1, 0, 0, 0,
                          0, 2, 0, 0,
                          0, 0, 3, 0,
                          0, 0, 0, 1);
            const Mat4f b(1, 2, 3, 4,
                          5, 6, 7, 8,
                          9, 10, 11, 12,
                          13, 14, 15, 16);
            // a is diagonal: row i of a*b = a[i][i] * row i of b.
            const Mat4f expected(1, 2, 3, 4,
                                 10, 12, 14, 16,
                                 27, 30, 33, 36,
                                 13, 14, 15, 16);
            AssertMatEqual(expected, simd::Multiply(a, b));
            AssertMatEqual(b, simd::Multiply(Mat4f::Identity(), b));
            AssertMatEqual(b, simd::Multiply(b, Mat4f::Identity()));
        }

        TEST_METHOD(SameAsOperatorStar)
        {
            Rng rng(16);
            for (int k = 0; k < 1000; ++k)
            {
                Mat4f a, b;
                for (int r = 0; r < 4; ++r)
                    for (int c = 0; c < 4; ++c)
                    {
                        a.m[r][c] = rng.Uniform(-2.0f, 2.0f);
                        b.m[r][c] = rng.Uniform(-2.0f, 2.0f);
                    }
                AssertMatEqual(a * b, simd::Multiply(a, b));
            }
        }

        TEST_METHOD(BatchAndAliasing)
        {
            Rng rng(17);
            std::vector<Mat4f> a(5), b(5), expected(5);
            for (std::size_t i = 0; i < a.size(); ++i)
            {
                a[i] = RandomAffine(rng);
                b[i] = RandomAffine(rng);
            }
            ref::MultiplyBatch(a.data(), b.data(), expected.data(), a.size());
            simd::MultiplyBatch(a.data(), b.data(), a.data(), a.size()); // out == a
            for (std::size_t i = 0; i < a.size(); ++i)
                AssertMatEqual(expected[i], a[i]);
        }
    };

    // ====================================================================================
    // AoS <-> SoA conversions
    // ====================================================================================
    TEST_CLASS(LayoutConversionTests)
    {
    public:
        TEST_METHOD(RoundTrip)
        {
            Rng rng(18);
            for (std::size_t n : TestSizes())
            {
                const std::vector<Vec3f> in = RandomVectors(n, rng);
                Vec3SoA soa(n);
                AoSToSoA(in.data(), soa, n);
                for (std::size_t i = 0; i < n; ++i)
                {
                    Assert::AreEqual(in[i].x, soa.x[i]);
                    Assert::AreEqual(in[i].y, soa.y[i]);
                    Assert::AreEqual(in[i].z, soa.z[i]);
                }
                std::vector<Vec3f> back(n);
                SoAToAoS(soa, back.data(), n);
                for (std::size_t i = 0; i < n; ++i)
                    AssertVecEqual(in[i], back[i], Label(L"round trip", n, i));
            }
        }
    };
}
