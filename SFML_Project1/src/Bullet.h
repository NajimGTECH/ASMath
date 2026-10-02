#pragma once
#include "Vector2.h"

#include "Entity.h"

#include "../utils/MathsToSFML.h"

class Bullet : public Entity
{
public:

    Bullet(const math::Vector2<float>& position, float rotation, const math::Vector2<float>& windowSize);

    void Update(float deltaTime, const math::Vector2<float>& windowSize) ;
    void Draw(sf::RenderWindow& window) const override;

    bool IsAlive() const { return m_alive; }

private:

    float m_speed = 0.f;
    bool m_alive = true;
};
