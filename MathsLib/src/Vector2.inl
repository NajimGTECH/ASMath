
namespace math
{
    // ======================
    // Constructors
    // ======================

    template<typename T>
    constexpr Vector2<T>::Vector2()
        : x(static_cast<T>(0)),
        y(static_cast<T>(0))
    {
    }

    template<typename T>
    constexpr Vector2<T>::Vector2(T x_, T y_)
        : x(x_),
        y(y_)
    {
    }

    template<typename T>
    template<typename U>
    constexpr Vector2<T>::Vector2(const Vector2<U>& other)
        : x(static_cast<T>(other.x)),
        y(static_cast<T>(other.y))
    {
    }


    // ======================
    // Arithmetic Operators
    // ======================

    template<typename T>
    constexpr Vector2<T> Vector2<T>::operator+(const Vector2& rhs) const
    {
        return { x + rhs.x, y + rhs.y };
    }

    template<typename T>
    constexpr Vector2<T> Vector2<T>::operator-(const Vector2& rhs) const
    {
        return { x - rhs.x, y - rhs.y };
    }

    template<typename T>
    constexpr Vector2<T>& Vector2<T>::operator+=(const Vector2& rhs)
    {
        x += rhs.x;
        y += rhs.y;

        return *this;
    }

    template<typename T>
    constexpr Vector2<T>& Vector2<T>::operator-=(const Vector2& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;

        return *this;
    }

    template<typename T>
    constexpr Vector2<T> Vector2<T>::operator*(T scalar) const
    {
        return { x * scalar, y * scalar };
    }

    template<typename T>
    Vector2<T> Vector2<T>::operator/(T scalar) const
    {
        if (scalar == static_cast<T>(0))
        {
            throw std::domain_error("Vector2 division by zero");
        }

        return { x / scalar, y / scalar };
    }

    template<typename T>
    constexpr Vector2<T>& Vector2<T>::operator*=(T scalar)
    {
        x *= scalar;
        y *= scalar;

        return *this;
    }

    template<typename T>
    Vector2<T>& Vector2<T>::operator/=(T scalar)
    {
        if (scalar == static_cast<T>(0))
        {
            throw std::domain_error("Vector2 division by zero");
        }

        x /= scalar;
        y /= scalar;

        return *this;
    }


    // ======================
    // Comparison
    // ======================

    template<typename T>
    bool Vector2<T>::Equals(
        const Vector2& rhs,
        T absoluteTolerance,
        T relativeTolerance
    ) const
    {
        if (absoluteTolerance < static_cast<T>(0) ||
            relativeTolerance < static_cast<T>(0))
        {
            throw std::invalid_argument("Tolerance cannot be negative");
        }

        const auto IsClose = [&](T a, T b)
            {
                const T difference = std::fabs(a - b);

                const T scale = std::max(
                    std::fabs(a),
                    std::fabs(b)
                );

                return difference <=
                    absoluteTolerance + relativeTolerance * scale;
            };

        return IsClose(x, rhs.x) && IsClose(y, rhs.y);
    }

    template<typename T>
    constexpr bool Vector2<T>::operator==(const Vector2& rhs) const
    {
        return x == rhs.x && y == rhs.y;
    }

    template<typename T>
    constexpr bool Vector2<T>::operator!=(const Vector2& rhs) const
    {
        return !(*this == rhs);
    }


    // ======================
    // Vector Math
    // ======================

    template<typename T>
    constexpr T Vector2<T>::Dot(const Vector2& rhs) const
    {
        return x * rhs.x + y * rhs.y;
    }

    template<typename T>
    T Vector2<T>::Length() const
    {
        return static_cast<T>(std::hypot(x, y));
    }

    template<typename T>
    constexpr T Vector2<T>::LengthSquared() const
    {
        return x * x + y * y;
    }

    template<typename T>
    Vector2<T> Vector2<T>::Normalized() const
    {
        const T length = Length();

        if (length == static_cast<T>(0))
        {
            return { static_cast<T>(0), static_cast<T>(0) };
        }

        return *this / length;
    }

    template<typename T>
    void Vector2<T>::Normalize()
    {
        const T length = Length();

        if (length == static_cast<T>(0))
        {
            return;
        }

        x /= length;
        y /= length;
    }


    // ======================
    // Distance and Interpolation
    // ======================

    template<typename T>
    T Vector2<T>::Distance(const Vector2& a, const Vector2& b)
    {
        return (a - b).Length();
    }

    template<typename T>
    Vector2<T> Vector2<T>::Lerp(
        const Vector2& a,
        const Vector2& b,
        T t
    )
    {
        return a + (b - a) * t;
    }


    // ======================
    // Angle
    // ======================

    template<typename T>
    T Vector2<T>::Angle(const Vector2& a, const Vector2& b)
    {
        const T magnitudeProduct = a.Length() * b.Length();

        if (magnitudeProduct == static_cast<T>(0))
        {
            return static_cast<T>(0);
        }

        T cosine = a.Dot(b) / magnitudeProduct;

        cosine = Clamp(
            cosine,
            static_cast<T>(-1),
            static_cast<T>(1)
        );

        const T radians = static_cast<T>(std::acos(cosine));

        return radians * static_cast<T>(
            180.0 / std::numbers::pi_v<double>
            );
    }


    // ======================
    // Geometric Operations
    // ======================

    template<typename T>
    Vector2<T> Vector2<T>::Perpendicular() const
    {
        return { -y, x };
    }

    template<typename T>
    Vector2<T> Vector2<T>::Scale(
        const Vector2& a,
        const Vector2& b
    )
    {
        return { a.x * b.x, a.y * b.y };
    }

    template<typename T>
    Vector2<T> Vector2<T>::Min(
        const Vector2& a,
        const Vector2& b
    )
    {
        return {
            std::min(a.x, b.x),
            std::min(a.y, b.y)
        };
    }

    template<typename T>
    Vector2<T> Vector2<T>::Max(
        const Vector2& a,
        const Vector2& b
    )
    {
        return {
            std::max(a.x, b.x),
            std::max(a.y, b.y)
        };
    }


    // ======================
    // Move Towards
    // ======================

    template<typename T>
    Vector2<T> Vector2<T>::MoveTowards(
        const Vector2& current,
        const Vector2& target,
        T maxDistanceDelta
    )
    {
        if (maxDistanceDelta < static_cast<T>(0))
        {
            throw std::invalid_argument(
                "maxDistanceDelta cannot be negative"
            );
        }

        const Vector2<T> delta = target - current;
        const T distance = delta.Length();

        if (distance == static_cast<T>(0) ||
            distance <= maxDistanceDelta)
        {
            return target;
        }

        return current + delta / distance * maxDistanceDelta;
    }


    // ======================
    // Private Functions
    // ======================

    template<typename T>
    T Vector2<T>::Clamp(T value, T minValue, T maxValue)
    {
        return std::max(minValue, std::min(value, maxValue));
    }
}