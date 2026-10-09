
#pragma once

#include <cmath>
#include <numbers>
#include <stdexcept>
#include <algorithm>

namespace math
{
    /**
    * @class Vector2
    * @brief Represents a 2D mathematical vector.
    *
    * Provides common arithmetic operations and vector mathematics.
    *
    * @tparam T Numeric type, preferably float or double.
    *
    * Zero vector behavior:
    * - Normalized() returns (0, 0).
    * - Normalize() leaves the vector unchanged.
    * - Angle() returns 0 degrees if either vector is zero.
    */
    template<typename T>
    class Vector2
    {
    public:

        // ======================
        // Public Constructors
        // ======================

        constexpr Vector2();

        constexpr Vector2(T x_, T y_);

        template<typename U>
        constexpr Vector2(const Vector2<U>& other);


        // ======================
        // Public Arithmetic Operators
        // ======================

        constexpr Vector2 operator+(const Vector2& rhs) const;

        constexpr Vector2 operator-(const Vector2& rhs) const;

        constexpr Vector2& operator+=(const Vector2& rhs);

        constexpr Vector2& operator-=(const Vector2& rhs);

        constexpr Vector2 operator*(T scalar) const;

        Vector2 operator/(T scalar) const;

        constexpr Vector2& operator*=(T scalar);

        Vector2& operator/=(T scalar);


        // ======================
        // Public Comparison
        // ======================

        bool Equals(
            const Vector2& rhs,
            T absoluteTolerance = static_cast<T>(1e-6),
            T relativeTolerance = static_cast<T>(1e-5)
        ) const;

        constexpr bool operator==(const Vector2& rhs) const;
        constexpr bool operator!=(const Vector2& rhs) const;


        // ======================
        // Public Vector Math
        // ======================

        constexpr T Dot(const Vector2& rhs) const;

        T Length() const;

        constexpr T LengthSquared() const;

        Vector2 Normalized() const;

        void Normalize();

        static T Distance(const Vector2& a, const Vector2& b);

        static Vector2 Lerp(const Vector2& a, const Vector2& b, T t);

        static T Angle(const Vector2& a, const Vector2& b);

        Vector2 Perpendicular() const;

        static Vector2 Scale(const Vector2& a, const Vector2& b);

        static Vector2 Min(const Vector2& a, const Vector2& b);

        static Vector2 Max(const Vector2& a, const Vector2& b);

        static Vector2 MoveTowards(
            const Vector2& current,
            const Vector2& target,
            T maxDistanceDelta
        );


        // ======================
        // Public Members
        // ======================

        T x;
        T y;


    private:

        // ======================
        // Private Functions
        // ======================

        static T Clamp(T value, T minValue, T maxValue);


        // ======================
        // Private Members
        // ======================

    };
}

#include "../src/Vector2.inl"