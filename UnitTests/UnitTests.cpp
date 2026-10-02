#include "pch.h"
#include "CppUnitTest.h"

#include "Vector2.h" 
#include "Vector3.h"
#include "Mat3.h"
#include "Mat4.h"
#include "Quaternion.h"

#include "nanobench.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace math;

namespace MathsLibTests
{
	TEST_CLASS(TestVector2)
	{
	public:

		/*
		* Helper method to compare floats
		*/
		static bool FloatEquals(float a, float b, float epsilon = 1e-6f)
		{
			return std::fabs(a - b) < epsilon;
		}

		/*
		* Helper assertion for vectors
		*/
		static void AssertVector2Equals(const Vector2<float>& expected, const Vector2<float>& actual, float epsilon = 1e-6f)
		{
			Assert::IsTrue(FloatEquals(expected.x, actual.x, epsilon), L"X component mismatch");
			Assert::IsTrue(FloatEquals(expected.y, actual.y, epsilon), L"Y component mismatch");
		}

		/*
		* Test default and parameterized constructors
		*/
		TEST_METHOD(TestConstructors)
		{
			Vector2<float> v1;
			Assert::AreEqual(0.0f, v1.x);
			Assert::AreEqual(0.0f, v1.y);

			Vector2<float> v2(3.0f, 4.0f);
			Assert::AreEqual(3.0f, v2.x);
			Assert::AreEqual(4.0f, v2.y);

			Vector2<int> vi(2, 5);
			Vector2<float> vf(vi);
			Assert::AreEqual(2.0f, vf.x);
			Assert::AreEqual(5.0f, vf.y);
		}

		/*
		* Test arithmetic operators
		*/
		TEST_METHOD(TestArithmetic)
		{
			Vector2<float> a(1.0f, 2.0f);
			Vector2<float> b(3.0f, 4.0f);

			Vector2<float> c = a + b;
			AssertVector2Equals(Vector2<float>(4.0f, 6.0f), c);

			Vector2<float> d = b - a;
			AssertVector2Equals(Vector2<float>(2.0f, 2.0f), d);

			Vector2<float> e = a * 2.0f;
			AssertVector2Equals(Vector2<float>(2.0f, 4.0f), e);

			Vector2<float> f = e / 2.0f;
			AssertVector2Equals(Vector2<float>(1.0f, 2.0f), f);

			Assert::IsTrue(a == Vector2<float>(1.0f, 2.0f));
			Assert::IsTrue(a != b);
		}

		/*
		* Test Dot and Angle
		*/
		TEST_METHOD(TestDotAndAngle)
		{
			Vector2<float> a(1.0f, 0.0f);
			Vector2<float> b(0.0f, 1.0f);

			float dot = a.Dot(b);
			Assert::AreEqual(0.0f, dot, 1e-6f);

			float angle = Vector2<float>::Angle(a, b);
			Assert::AreEqual(90.0f, angle, 1e-4f);
		}

		/*
		* Test normalization and length
		*/
		TEST_METHOD(TestNormalization)
		{
			Vector2<float> v(3.0f, 4.0f);
			Assert::AreEqual(5.0f, v.Length(), 1e-6f);

			Vector2<float> n = v.Normalized();
			Assert::AreEqual(1.0f, n.Length(), 1e-6f);
		}

		/*
		* Test static utility functions
		*/
		TEST_METHOD(TestUtilities)
		{
			Vector2<float> a(1.0f, 5.0f);
			Vector2<float> b(3.0f, 2.0f);

			Vector2<float> min = Vector2<float>::Min(a, b);
			AssertVector2Equals(Vector2<float>(1.0f, 2.0f), min);

			Vector2<float> max = Vector2<float>::Max(a, b);
			AssertVector2Equals(Vector2<float>(3.0f, 5.0f), max);

			Vector2<float> scaled = Vector2<float>::Scale(a, b);
			AssertVector2Equals(Vector2<float>(3.0f, 10.0f), scaled);

			Vector2<float> perp = a.Perpendicular();
			AssertVector2Equals(Vector2<float>(-5.0f, 1.0f), perp);

			Vector2<float> start(0.0f, 0.0f);
			Vector2<float> target(10.0f, 0.0f);
			Vector2<float> moved = Vector2<float>::MoveTowards(start, target, 3.0f);
			AssertVector2Equals(Vector2<float>(3.0f, 0.0f), moved);
		}
	};

	TEST_CLASS(TestVector3)
	{
	public:

		/*
		* Helper function for float comparison
		*/
		static bool FloatEquals(float a, float b, float epsilon = 1e-6f)
		{
			return std::fabs(a - b) < epsilon;
		}

		/*
		* Helper function to compare Vector3
		*/
		static void AssertVector3Equals(const Vector3<float>& expected, const Vector3<float>& actual, float epsilon = 1e-6f)
		{
			Assert::IsTrue(FloatEquals(expected.x, actual.x, epsilon), L"X mismatch");
			Assert::IsTrue(FloatEquals(expected.y, actual.y, epsilon), L"Y mismatch");
			Assert::IsTrue(FloatEquals(expected.z, actual.z, epsilon), L"Z mismatch");
		}

		/*
		* Test constructors
		*/
		TEST_METHOD(TestConstructors)
		{
			Vector3<float> v1;
			Assert::AreEqual(0.0f, v1.x);
			Assert::AreEqual(0.0f, v1.y);
			Assert::AreEqual(0.0f, v1.z);

			Vector3<float> v2(1.0f, 2.0f, 3.0f);
			Assert::AreEqual(1.0f, v2.x);
			Assert::AreEqual(2.0f, v2.y);
			Assert::AreEqual(3.0f, v2.z);

			Vector3<int> vi(5, 6, 7);
			Vector3<float> vf(vi);
			Assert::AreEqual(5.0f, vf.x);
			Assert::AreEqual(6.0f, vf.y);
			Assert::AreEqual(7.0f, vf.z);
		}

		/*
		* Test arithmetic operators
		*/
		TEST_METHOD(TestArithmetic)
		{
			Vector3<float> a(1.0f, 2.0f, 3.0f);
			Vector3<float> b(4.0f, 5.0f, 6.0f);

			AssertVector3Equals(Vector3<float>(5.0f, 7.0f, 9.0f), a + b);
			AssertVector3Equals(Vector3<float>(-3.0f, -3.0f, -3.0f), a - b);
			AssertVector3Equals(Vector3<float>(2.0f, 4.0f, 6.0f), a * 2.0f);
			AssertVector3Equals(Vector3<float>(0.5f, 1.0f, 1.5f), a / 2.0f);

			a += b;
			AssertVector3Equals(Vector3<float>(5.0f, 7.0f, 9.0f), a);

			a -= b;
			AssertVector3Equals(Vector3<float>(1.0f, 2.0f, 3.0f), a);
		}

		/*
		* Test equality
		*/
		TEST_METHOD(TestEquality)
		{
			Vector3<float> a(1.0f, 2.0f, 3.0f);
			Vector3<float> b(1.0f, 2.0f, 3.0f);
			Vector3<float> c(3.0f, 2.0f, 1.0f);

			Assert::IsTrue(a == b);
			Assert::IsTrue(a != c);
			Assert::IsTrue(a.Equals(b));
			Assert::IsFalse(a.Equals(c));
		}

		/*
		* Test Dot and Cross
		*/
		TEST_METHOD(TestDotAndCross)
		{
			Vector3<float> a(1.0f, 0.0f, 0.0f);
			Vector3<float> b(0.0f, 1.0f, 0.0f);

			float dot = a.Dot(b);
			Assert::AreEqual(0.0f, dot, 1e-6f);

			Vector3<float> cross = a.Cross(b);
			AssertVector3Equals(Vector3<float>(0.0f, 0.0f, 1.0f), cross);
		}

		/*
		* Test magnitude and normalization
		*/
		TEST_METHOD(TestMagnitudeAndNormalization)
		{
			Vector3<float> v(3.0f, 4.0f, 0.0f);
			Assert::AreEqual(5.0f, v.Length(), 1e-6f);
			Assert::AreEqual(25.0f, v.LengthSquared(), 1e-6f);

			Vector3<float> n = v.Normalized();
			Assert::AreEqual(1.0f, n.Length(), 1e-6f);
		}

		/*
		* Test static math functions
		*/
		TEST_METHOD(TestStaticFunctions)
		{
			Vector3<float> a(1.0f, 5.0f, 2.0f);
			Vector3<float> b(3.0f, 2.0f, 4.0f);

			AssertVector3Equals(Vector3<float>(1.0f, 2.0f, 2.0f), Vector3<float>::Min(a, b));
			AssertVector3Equals(Vector3<float>(3.0f, 5.0f, 4.0f), Vector3<float>::Max(a, b));
			AssertVector3Equals(Vector3<float>(3.0f, 10.0f, 8.0f), Vector3<float>::Scale(a, b));
		}

		/*
		* Test angle and reflection
		*/
		TEST_METHOD(TestAngleAndReflect)
		{
			Vector3<float> a(1.0f, 0.0f, 0.0f);
			Vector3<float> b(0.0f, 1.0f, 0.0f);

			float angle = Vector3<float>::Angle(a, b);
			Assert::AreEqual(90.0f, angle, 1e-4f);

			Vector3<float> normal(0.0f, 1.0f, 0.0f);
			Vector3<float> dir(0.0f, -1.0f, 0.0f);
			Vector3<float> reflected = Vector3<float>::Reflect(dir, normal);

			AssertVector3Equals(Vector3<float>(0.0f, 1.0f, 0.0f), reflected);
		}

		/*
		* Test MoveTowards and Lerp
		*/
		TEST_METHOD(TestMoveTowardsAndLerp)
		{
			Vector3<float> start(0.0f, 0.0f, 0.0f);
			Vector3<float> target(10.0f, 0.0f, 0.0f);

			Vector3<float> moved = Vector3<float>::MoveTowards(start, target, 3.0f);
			AssertVector3Equals(Vector3<float>(3.0f, 0.0f, 0.0f), moved);

			Vector3<float> lerpHalf = Vector3<float>::Lerp(start, target, 0.5f);
			AssertVector3Equals(Vector3<float>(5.0f, 0.0f, 0.0f), lerpHalf);
		}

		/*
		* Test Perpendicular generation
		*/
		TEST_METHOD(TestPerpendicular)
		{
			Vector3<float> v(1.0f, 0.0f, 0.0f);
			Vector3<float> p = v.Perpendicular();

			Assert::IsTrue(FloatEquals(0.0f, v.Dot(p), 1e-6f), L"Perpendicular vector should be orthogonal");
		}
	};

	TEST_CLASS(TestQuaternion)
	{
	public:

		// -------------------- Constructors --------------------

		TEST_METHOD(DefaultConstructor_ShouldBeIdentity)
		{
			Quaternion q;
			Assert::AreEqual(1.0f, q.w, 1e-6f);
			Assert::AreEqual(0.0f, q.x, 1e-6f);
			Assert::AreEqual(0.0f, q.y, 1e-6f);
			Assert::AreEqual(0.0f, q.z, 1e-6f);
		}

		TEST_METHOD(ConstructorWithComponents_ShouldSetValues)
		{
			Quaternion q(0.1f, 0.2f, 0.3f, 0.4f);
			Assert::AreEqual(0.1f, q.w, 1e-6f);
			Assert::AreEqual(0.2f, q.x, 1e-6f);
			Assert::AreEqual(0.3f, q.y, 1e-6f);
			Assert::AreEqual(0.4f, q.z, 1e-6f);
		}

		// -------------------- Static Methods --------------------

		TEST_METHOD(Identity_ShouldReturnIdentityQuaternion)
		{
			Quaternion q = Quaternion::Identity();
			Assert::AreEqual(1.0f, q.w, 1e-6f);
			Assert::AreEqual(0.0f, q.x, 1e-6f);
			Assert::AreEqual(0.0f, q.y, 1e-6f);
			Assert::AreEqual(0.0f, q.z, 1e-6f);
		}

		TEST_METHOD(FromAxisAngle_ShouldCreateCorrectRotation)
		{
			Vector3<float> axis(0.0f, 1.0f, 0.0f);
			float angle = std::numbers::pi_v<float> / 2.0f;
			Quaternion q = Quaternion::FromAxisAngle(axis, angle);
			Assert::IsTrue(std::fabs(q.Magnitude() - 1.0f) < 1e-5f);
		}

		TEST_METHOD(FromEuler_ShouldMatchRoundTripEuler)
		{
			Vector3<float> euler{
				10.f * std::numbers::pi_v<float> / 180.f,
				20.f * std::numbers::pi_v<float> / 180.f,
				30.f * std::numbers::pi_v<float> / 180.f
			};

			Quaternion q = Quaternion::FromEuler(euler);
			Vector3<float> eulerOut = q.ToEuler();

			Assert::IsTrue(std::fabs(eulerOut.x - euler.x) < 1e-5f);
			Assert::IsTrue(std::fabs(eulerOut.y - euler.y) < 1e-5f);
			Assert::IsTrue(std::fabs(eulerOut.z - euler.z) < 1e-5f);
		}

		TEST_METHOD(FromToRotation_ShouldRotateVectorCorrectly)
		{
			Vector3<float> from(1.0f, 0.0f, 0.0f);
			Vector3<float> to(0.0f, 0.0f, 1.0f);
			Quaternion q = Quaternion::FromToRotation(from, to);
			Vector3<float> result = q.RotateVector(from);
			Assert::IsTrue(std::fabs(result.x - to.x) < 1e-5f);
			Assert::IsTrue(std::fabs(result.y - to.y) < 1e-5f);
			Assert::IsTrue(std::fabs(result.z - to.z) < 1e-5f);
		}

		TEST_METHOD(LookRotation_ShouldOrientForward)
		{
			Vector3<float> forward(0.0f, 0.0f, 1.0f);
			Quaternion q = Quaternion::LookRotation(forward);
			Vector3<float> rotated = q.RotateVector({ 0.0f, 0.0f, 1.0f });
			Assert::IsTrue(std::fabs(rotated.x - forward.x) < 1e-5f);
			Assert::IsTrue(std::fabs(rotated.y - forward.y) < 1e-5f);
			Assert::IsTrue(std::fabs(rotated.z - forward.z) < 1e-5f);
		}

		// -------------------- Normalization --------------------

		TEST_METHOD(Normalize_ShouldYieldUnitQuaternion)
		{
			Quaternion q(2.0f, 0.0f, 0.0f, 0.0f);
			q.Normalize();
			Assert::AreEqual(1.0f, q.Magnitude(), 1e-5f);
		}

		TEST_METHOD(Normalized_ShouldReturnUnitQuaternionWithoutChangingOriginal)
		{
			Quaternion q(2.0f, 0.0f, 0.0f, 0.0f);
			Quaternion copy = q.Normalized();
			Assert::AreEqual(2.0f, q.w, 1e-6f);
			Assert::AreEqual(1.0f, copy.Magnitude(), 1e-5f);
		}

		// -------------------- Conjugate / Inverse --------------------

		TEST_METHOD(Conjugate_ShouldNegateVectorPart)
		{
			Quaternion q(1.0f, 2.0f, 3.0f, 4.0f);
			Quaternion c = q.Conjugate();
			Assert::AreEqual(1.0f, c.w, 1e-6f);
			Assert::AreEqual(-2.0f, c.x, 1e-6f);
			Assert::AreEqual(-3.0f, c.y, 1e-6f);
			Assert::AreEqual(-4.0f, c.z, 1e-6f);
		}

		TEST_METHOD(Inverse_ShouldReturnInverseQuaternion)
		{
			Quaternion q = Quaternion::FromEuler({ 0.1f, 0.2f, 0.3f });
			Quaternion inv = q.Inverse();
			Quaternion prod = q * inv;
			Assert::IsTrue(std::fabs(prod.w - 1.0f) < 1e-5f);
		}

		// -------------------- Magnitude --------------------

		TEST_METHOD(Magnitude_ShouldMatchSqrtSumSquares)
		{
			Quaternion q(1.0f, 2.0f, 3.0f, 4.0f);
			Assert::AreEqual((float)std::sqrt(1 + 4 + 9 + 16), q.Magnitude(), 1e-6f);
		}

		TEST_METHOD(GetMagnitudeSquared_ShouldReturnSumSquares)
		{
			Quaternion q(1.0f, 2.0f, 3.0f, 4.0f);
			Assert::AreEqual(30.0f, q.GetMagnitudeSquared(), 1e-6f);
		}

		// -------------------- Dot / Angle --------------------

		TEST_METHOD(Dot_ShouldReturnCorrectValue)
		{
			Quaternion a(1.0f, 0.0f, 1.0f, 0.0f);
			Quaternion b(1.0f, 0.5f, 0.5f, 0.75f);
			float dot = Quaternion::Dot(a, b);
			Assert::AreEqual(1.5f, dot, 1e-6f);

			Quaternion c(1.0f, 1.0f, 1.0f, 1.0f);
			Quaternion d(1.0f, 0.f, 0.f, 0.f);
			float dot2 = Quaternion::Dot(c, d);
			Assert::AreEqual(1.f, dot2, 1e-6f);
		}

		TEST_METHOD(Angle_ShouldReturnAngleBetweenQuaternions)
		{
			Quaternion a = Quaternion::Identity();
			Quaternion b = Quaternion::FromAxisAngle({ 0.0f,1.0f,0.0f }, std::numbers::pi_v<float> / 2.0f);
			float angle = Quaternion::Angle(a, b);
			Assert::IsTrue(std::fabs(angle - std::numbers::pi_v<float> / 2.0f) < 1e-4f);
		}

		// -------------------- Operators --------------------

		TEST_METHOD(Multiplication_ShouldComposeRotation)
		{
			Quaternion a = Quaternion::FromEuler({ 0.1f,0.2f,0.3f });
			Quaternion b = Quaternion::FromEuler({ 0.0f,0.5f,0.0f });
			Quaternion c = a * b;
			Assert::IsTrue(std::fabs(c.Magnitude() - 1.0f) < 1e-5f);
		}

		TEST_METHOD(MultiplyAssign_ShouldModifyOriginal)
		{
			Quaternion a = Quaternion::FromEuler({ 0.1f,0.2f,0.3f });
			Quaternion b = Quaternion::FromEuler({ 0.0f,0.5f,0.0f });
			a *= b;
			Assert::IsTrue(std::fabs(a.Magnitude() - 1.0f) < 1e-5f);
		}

		TEST_METHOD(EqualOperator_ShouldDetectEquality)
		{
			Quaternion q1(1.0f, 0.0f, 0.0f, 0.0f);
			Quaternion q2 = Quaternion::Identity();
			Assert::IsTrue(q1 == q2);
		}

		TEST_METHOD(NotEqualOperator_ShouldDetectInequality)
		{
			Quaternion q1(1.0f, 0.0f, 0.0f, 0.0f);
			Quaternion q2 = Quaternion::FromEuler({ 0.1f,0.0f,0.0f });
			Assert::IsTrue(q1 != q2);
		}

		// -------------------- Vector Rotation --------------------

		TEST_METHOD(RotateVector_ShouldRotateCorrectly)
		{
			Quaternion q = Quaternion::FromAxisAngle({ 0.0f, 1.0f, 0.0f }, std::numbers::pi_v<float> / 2.0f);
			Vector3<float> v(1.0f, 0.0f, 0.0f);
			Vector3<float> r = q.RotateVector(v);

			Assert::IsTrue(std::fabs(r.x) < 1e-5f);
			Assert::IsTrue(std::fabs(r.y) < 1e-5f);
			Assert::IsTrue(std::fabs(r.z + 1.0f) < 1e-5f);
		}

		// -------------------- Interpolation --------------------

		TEST_METHOD(Slerp_ShouldInterpolate)
		{
			Quaternion a = Quaternion::Identity();
			Quaternion b = Quaternion::FromAxisAngle({ 0.0f,1.0f,0.0f }, std::numbers::pi_v<float> / 2.0f);
			Quaternion mid = Quaternion::Slerp(a, b, 0.5f);
			Assert::IsTrue(std::fabs(mid.Magnitude() - 1.0f) < 1e-5f);
		}

		TEST_METHOD(SlerpUnclamped_ShouldInterpolateBeyondRange)
		{
			Quaternion a = Quaternion::Identity();
			Quaternion b = Quaternion::FromAxisAngle({ 0.0f,1.0f,0.0f }, std::numbers::pi_v<float> / 2.0f);
			Quaternion mid = Quaternion::SlerpUnclamped(a, b, 1.5f);
			Assert::IsTrue(std::fabs(mid.Magnitude() - 1.0f) < 1e-5f);
		}

		TEST_METHOD(Lerp_ShouldInterpolate)
		{
			Quaternion a = Quaternion::Identity();
			Quaternion b = Quaternion::FromAxisAngle({ 0.0f,1.0f,0.0f }, std::numbers::pi_v<float> / 2.0f);
			Quaternion mid = Quaternion::Lerp(a, b, 0.5f);
			Assert::IsTrue(std::fabs(mid.Magnitude() - 1.0f) < 1e-5f);
		}

		TEST_METHOD(LerpUnclamped_ShouldInterpolateBeyondRange)
		{
			Quaternion a = Quaternion::Identity();
			Quaternion b = Quaternion::FromAxisAngle({ 0.0f,1.0f,0.0f }, std::numbers::pi_v<float> / 2.0f);
			Quaternion mid = Quaternion::LerpUnclamped(a, b, 1.5f);
			Assert::IsTrue(std::fabs(mid.Magnitude() - 1.0f) < 1e-5f);
		}

		TEST_METHOD(RotateTowards_ShouldLimitRotation)
		{
			Quaternion a = Quaternion::Identity();
			Quaternion b = Quaternion::FromAxisAngle({ 0.0f,1.0f,0.0f }, std::numbers::pi_v<float> / 2.0f);
			Quaternion r = Quaternion::RotateTowards(a, b, std::numbers::pi_v<float> / 4.0f);
			float angle = Quaternion::Angle(a, r);
			Assert::IsTrue(std::fabs(angle - std::numbers::pi_v<float> / 4.0f) < 1e-4f);
		}

		// -------------------- Axis-Angle / Euler Conversion --------------------

		TEST_METHOD(ToAxisAngle_ShouldBeConsistentWithFromAxisAngle)
		{
			Vector3<float> axis(0.0f, 1.0f, 0.0f);
			float angle = std::numbers::pi_v<float> / 3.0f;
			Quaternion q = Quaternion::FromAxisAngle(axis, angle);
			Vector3<float> outAxis;
			float outAngle;
			q.ToAxisAngle(outAxis, outAngle);
			Assert::IsTrue(std::fabs(outAngle - angle) < 1e-5f);
			Assert::IsTrue(std::fabs(outAxis.y - 1.0f) < 1e-5f);
		}

		TEST_METHOD(ToEuler_ShouldBeConsistentWithFromEuler)
		{
			Vector3<float> euler{ 0.1f,0.2f,0.3f };
			Quaternion q = Quaternion::FromEuler(euler);
			Vector3<float> eOut = q.ToEuler();
			Assert::IsTrue(std::fabs(eOut.x - euler.x) < 1e-5f);
			Assert::IsTrue(std::fabs(eOut.y - euler.y) < 1e-5f);
			Assert::IsTrue(std::fabs(eOut.z - euler.z) < 1e-5f);
		}
	};

	TEST_CLASS(TestMat3)
	{
	public:

		TEST_METHOD(TestIdentity)
		{
			Mat3<float> m = Mat3<float>::Identity();

			Assert::AreEqual(1.0f, m(0, 0), 0.0001f);
			Assert::AreEqual(1.0f, m(1, 1), 0.0001f);
			Assert::AreEqual(1.0f, m(2, 2), 0.0001f);

			Assert::AreEqual(0.0f, m(0, 1), 0.0001f);
			Assert::AreEqual(0.0f, m(1, 0), 0.0001f);
			Assert::AreEqual(0.0f, m(2, 0), 0.0001f);
		}

		TEST_METHOD(TestTranspose)
		{
			Mat3<float> m(
				1, 2, 3,
				4, 5, 6,
				7, 8, 9
			);

			Mat3<float> t = m.Transposed();

			Assert::AreEqual(1.0f, t(0, 0), 0.0001f);
			Assert::AreEqual(4.0f, t(0, 1), 0.0001f);
			Assert::AreEqual(7.0f, t(0, 2), 0.0001f);
			Assert::AreEqual(2.0f, t(1, 0), 0.0001f);
			Assert::AreEqual(5.0f, t(1, 1), 0.0001f);
			Assert::AreEqual(8.0f, t(1, 2), 0.0001f);
		}

		TEST_METHOD(TestDeterminant)
		{
			Mat3<float> m(
				1, 2, 3,
				0, 1, 4,
				5, 6, 0
			);

			float det = m.Determinant();
			Assert::AreEqual(1.0f, det, 0.0001f); // Expected determinant = 1
		}

		TEST_METHOD(TestInverse)
		{
			Mat3<float> m(
				1, 2, 3,
				0, 1, 4,
				5, 6, 0
			);

			Mat3<float> inv = m.Inverted();

			// Theoretical inverse of m
			Mat3<float> expected(
				-24, 18, 5,
				20, -15, -4,
				-5, 4, 1
			);

			for (int r = 0; r < 3; ++r)
			{
				for (int c = 0; c < 3; ++c)
				{
					Assert::AreEqual(expected(r, c), inv(r, c) * m.Determinant(), 0.0001f);
				}
			}
		}

		TEST_METHOD(TestMatrixMultiplication)
		{
			Mat3<float> a(
				1, 2, 3,
				4, 5, 6,
				7, 8, 9
			);

			Mat3<float> b(
				9, 8, 7,
				6, 5, 4,
				3, 2, 1
			);

			Mat3<float> result = a * b;

			Assert::AreEqual(30.0f, result(0, 0), 0.0001f);
			Assert::AreEqual(24.0f, result(0, 1), 0.0001f);
			Assert::AreEqual(18.0f, result(0, 2), 0.0001f);
		}

		TEST_METHOD(TestVectorMultiplication)
		{
			Mat3<float> m(
				1, 0, 0,
				0, 2, 0,
				0, 0, 3
			);

			Vector3<float> v(1, 2, 3);
			Vector3<float> result = m * v;

			Assert::AreEqual(1.0f, result.x, 0.0001f);
			Assert::AreEqual(4.0f, result.y, 0.0001f);
			Assert::AreEqual(9.0f, result.z, 0.0001f);
		}

		TEST_METHOD(TestGetSetRowColumn)
		{
			Mat3<float> m = Mat3<float>::Identity();

			Vector3<float> newRow(1, 2, 3);
			m.SetRow(1, newRow);

			Vector3<float> row = m.GetRow(1);
			Assert::AreEqual(1.0f, row.x, 0.0001f);
			Assert::AreEqual(2.0f, row.y, 0.0001f);
			Assert::AreEqual(3.0f, row.z, 0.0001f);

			Vector3<float> newCol(7, 8, 9);
			m.SetColumn(0, newCol);

			Vector3<float> col = m.GetColumn(0);
			Assert::AreEqual(7.0f, col.x, 0.0001f);
			Assert::AreEqual(8.0f, col.y, 0.0001f);
			Assert::AreEqual(9.0f, col.z, 0.0001f);
		}
	};

	TEST_CLASS(TestMat4)
	{
	public:

		/*
		* Helper function to compare two floats
		*/
		static bool FloatEquals(float a, float b, float epsilon = 1e-5f)
		{
			return std::fabs(a - b) < epsilon;
		}

		/*
		* Helper function to compare two Mat4
		*/
		static void AssertMat4Equals(const Mat4<float>& expected, const Mat4<float>& actual, float epsilon = 1e-5f)
		{
			for (int r = 0; r < 4; ++r)
			{
				for (int c = 0; c < 4; ++c)
				{
					Assert::IsTrue(FloatEquals(expected(r, c), actual(r, c), epsilon),
						(std::wstring(L"Mismatch at element [") + std::to_wstring(r) + L"," + std::to_wstring(c) + L"]").c_str());
				}
			}
		}

		// --------------------------------------------------------------------
		// Constructors and Identity / Zero
		// --------------------------------------------------------------------

		TEST_METHOD(TestIdentityMatrix)
		{
			Mat4<float> m = Mat4<float>::Identity();

			for (int i = 0; i < 4; ++i)
			{
				for (int j = 0; j < 4; ++j)
				{
					float expected = (i == j) ? 1.0f : 0.0f;
					Assert::AreEqual(expected, m(i, j), 1e-6f);
				}
			}
		}

		TEST_METHOD(TestZeroMatrix)
		{
			Mat4<float> m = Mat4<float>::Zero();

			for (int r = 0; r < 4; ++r)
				for (int c = 0; c < 4; ++c)
					Assert::AreEqual(0.0f, m(r, c), 1e-6f);
		}

		// --------------------------------------------------------------------
		// Basic Operations
		// --------------------------------------------------------------------

		TEST_METHOD(TestTranspose)
		{
			Mat4<float> m(
				1, 2, 3, 4,
				5, 6, 7, 8,
				9, 10, 11, 12,
				13, 14, 15, 16
			);

			Mat4<float> t = m.Transpose();

			Assert::AreEqual(m(0, 1), t(1, 0), 1e-6f);
			Assert::AreEqual(m(2, 3), t(3, 2), 1e-6f);
			Assert::AreEqual(m(1, 0), t(0, 1), 1e-6f);
		}

		TEST_METHOD(TestMatrixMultiplication)
		{
			Mat4<float> a(
				1, 0, 0, 0,
				0, 2, 0, 0,
				0, 0, 3, 0,
				0, 0, 0, 1
			);

			Mat4<float> b(
				1, 2, 3, 4,
				5, 6, 7, 8,
				9, 10, 11, 12,
				13, 14, 15, 16
			);

			Mat4<float> result = a * b;

			Assert::AreEqual(1.0f, result(0, 0), 1e-6f);
			Assert::AreEqual(12.0f, result(1, 1), 1e-6f);
			Assert::AreEqual(33.0f, result(2, 2), 1e-6f);
			Assert::AreEqual(13.0f, result(3, 0), 1e-6f);
		}

		TEST_METHOD(TestDeterminantAndInverse)
		{
			Mat4<float> m(
				1, 0, 0, 0,
				0, 2, 0, 0,
				0, 0, 3, 0,
				0, 0, 0, 1
			);

			float det = m.Determinant();
			Assert::AreEqual(6.0f, det, 1e-6f);

			Mat4<float> inv = m.Inverse();

			Mat4<float> expected(
				1, 0, 0, 0,
				0, 0.5f, 0, 0,
				0, 0, 1.0f / 3.0f, 0,
				0, 0, 0, 1
			);

			AssertMat4Equals(expected, inv);
		}

		// --------------------------------------------------------------------
		// TRS and component extraction
		// --------------------------------------------------------------------

		TEST_METHOD(TestTRS_And_Extraction)
		{
			Vector3<float> position(1.0f, 2.0f, 3.0f);
			Quaternion rotation = Quaternion::FromAxisAngle({ 0.0f, 1.0f, 0.0f }, std::numbers::pi_v<float> / 2.0f);
			Vector3<float> scale(2.0f, 2.0f, 2.0f);

			Mat4<float> m = Mat4<float>::TRS(position, rotation, scale);

			Vector3<float> extractedPos = m.ExtractPosition();
			Vector3<float> extractedScale = m.ExtractScale();
			Quaternion extractedRot = m.ExtractRotation();

			Assert::IsTrue(FloatEquals(extractedPos.x, 1.0f));
			Assert::IsTrue(FloatEquals(extractedPos.y, 2.0f));
			Assert::IsTrue(FloatEquals(extractedPos.z, 3.0f));

			Assert::IsTrue(FloatEquals(extractedScale.x, 2.0f));
			Assert::IsTrue(FloatEquals(extractedScale.y, 2.0f));
			Assert::IsTrue(FloatEquals(extractedScale.z, 2.0f));

			Assert::IsTrue(FloatEquals(extractedRot.Magnitude(), 1.0f));
		}

		// --------------------------------------------------------------------
		// MultiplyPoint and MultiplyVector
		// --------------------------------------------------------------------

		TEST_METHOD(TestMultiplyPoint)
		{
			Mat4<float> m = Mat4<float>::Translate({ 1.0f, 2.0f, 3.0f });
			Vector3<float> p(1.0f, 1.0f, 1.0f);
			Vector3<float> result = m.MultiplyPoint(p);

			Assert::AreEqual(2.0f, result.x, 1e-6f);
			Assert::AreEqual(3.0f, result.y, 1e-6f);
			Assert::AreEqual(4.0f, result.z, 1e-6f);
		}

		TEST_METHOD(TestMultiplyVector)
		{
			Mat4<float> m = Mat4<float>::Scale({ 2.0f, 3.0f, 4.0f });
			Vector3<float> v(1.0f, 1.0f, 1.0f);
			Vector3<float> result = m.MultiplyVector(v);

			Assert::AreEqual(2.0f, result.x, 1e-6f);
			Assert::AreEqual(3.0f, result.y, 1e-6f);
			Assert::AreEqual(4.0f, result.z, 1e-6f);
		}

		// --------------------------------------------------------------------
		// Projection and view matrices
		// --------------------------------------------------------------------

		TEST_METHOD(TestPerspectiveMatrix)
		{
			Mat4<float> m = Mat4<float>::Perspective(90.0f * std::numbers::pi_v<float> / 180.0f, 1.0f, 0.1f, 100.0f);
			Assert::IsTrue(FloatEquals(m(3, 2), -1.0f) || FloatEquals(m(2, 3), -1.0f));
		}

		TEST_METHOD(TestOrthoMatrix)
		{
			Mat4<float> m = Mat4<float>::Ortho(-1, 1, -1, 1, 0.1f, 10.0f);
			Assert::AreEqual(1.0f, m(0, 0), 1e-6f);
			Assert::AreEqual(1.0f, m(1, 1), 1e-6f);
		}

		TEST_METHOD(TestLookAtMatrix)
		{
			Vector3<float> eye(0.0f, 0.0f, 0.0f);
			Vector3<float> target(0.0f, 0.0f, -1.0f);
			Vector3<float> up(0.0f, 1.0f, 0.0f);

			Mat4<float> m = Mat4<float>::LookAt(eye, target, up);

			// Forward axis should point towards -Z
			Assert::IsTrue(FloatEquals(m(2, 2), 1.0f) || FloatEquals(m(2, 2), -1.0f));
		}

		// --------------------------------------------------------------------
		// Equality operators and access
		// --------------------------------------------------------------------

		TEST_METHOD(TestEqualityOperators)
		{
			Mat4<float> a = Mat4<float>::Identity();
			Mat4<float> b = Mat4<float>::Identity();
			Assert::IsTrue(a == b);
			b(0, 0) = 2.0f;
			Assert::IsTrue(a != b);
		}

		TEST_METHOD(TestGetRowAndColumn)
		{
			Mat4<float> m(
				1, 2, 3, 4,
				5, 6, 7, 8,
				9, 10, 11, 12,
				13, 14, 15, 16
			);

			Vector3<float> row = m.GetRow(1);

			Vector3<float> col = m.GetColumn(2);

			Assert::AreEqual(5.0f, row.x, 1e-6f);
			Assert::AreEqual(7.0f, row.z, 1e-6f);

			Assert::AreEqual(3.0f, col.x, 1e-6f);
			Assert::AreEqual(7.0f, col.y, 1e-6f);
		}

		TEST_METHOD(TestValidTRS)
		{
			Mat4<float> m = Mat4<float>::TRS({ 0, 0, 0 }, Quaternion::Identity(), { 1, 1, 1 });
			Assert::IsTrue(m.ValidTRS());
		}
	};
}
