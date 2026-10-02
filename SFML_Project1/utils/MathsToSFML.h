#pragma once
#include <SFML/System/Vector2.hpp>
#include "Vector2.h"

namespace util
{
    template<typename T>
    struct SFMLConverter
    {
        static sf::Vector2<T> Convert(const math::Vector2<T>& vec)
        {
            return sf::Vector2<T>(vec.x, vec.y);
        }
    };
}