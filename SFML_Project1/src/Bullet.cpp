#include "Bullet.h"
#include <SFML/Graphics.hpp>
#include <cmath>

Bullet::Bullet(const math::Vector2<float>& position, float rotation, const math::Vector2<float>& windowSize)
    : m_speed(500.f), m_alive(true)
{
	// Circle representing the bullet
    auto shape = new sf::CircleShape(3.f);
    shape->setFillColor(sf::Color::White);
    m_shape = shape;

	// Initial position and rotation
    m_position = position;
    m_rotation = rotation;

	// Calculating direction vector based on rotation
    float rad = (m_rotation - 90.f) * std::numbers::pi_v<float> / 180.f;
    m_velocity = math::Vector2<float>(std::cos(rad), std::sin(rad)) * m_speed;
}

void Bullet::Update(float deltaTime, const math::Vector2<float>& windowSize)
{
    m_position += m_velocity * deltaTime;

    if (m_shape)
        m_shape->setPosition(util::SFMLConverter<float>::Convert(m_position));
}

void Bullet::Draw(sf::RenderWindow& window) const
{
    if (m_alive && m_shape)
        window.draw(*m_shape);
}

