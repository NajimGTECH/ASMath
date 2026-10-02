#pragma once
#include "Entity.h"

#include "../utils/MathsToSFML.h"

#include <SFML/Graphics.hpp>

#include <cmath>
#include <numbers>

class Asteroid : public Entity
{
public:

    Asteroid(const math::Vector2<float>& windowSize);
    Asteroid(const math::Vector2<float>& position, int size);

    void Update(float deltaTime, const math::Vector2<float>& windowSize) ;
    void Draw(sf::RenderWindow& window) const override;

    // Getters
    float GetSize() const;
    int GetSizeIndex() const;

private:

    float m_rotationSpeed = 0.f;
    int m_size; // 1 = small, 2 = medium, 3 = large

private:

    void InitializeShape();
    void WrapAround(const math::Vector2<float>& windowSize);
};

