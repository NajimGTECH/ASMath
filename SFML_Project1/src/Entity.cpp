#include "Entity.h"

Entity::Entity()
    : m_shape(nullptr),
    m_position(0.f, 0.f),
    m_velocity(0.f, 0.f),
    m_rotation(0.f)
{}

/*
* Updates the position based on velocity and delta time.
*/
void Entity::Update(float deltaTime)
{
    m_position += m_velocity * deltaTime;

    if (m_shape)
    {
        // Implicit conversion from math::Vector2<float> to sf::Vector2f
        m_shape->setPosition(util::SFMLConverter<float>::Convert(m_position));
        m_shape->setRotation(sf::degrees(m_rotation));
    }
}

/*
* Draws the entity on the window if it has a valid SFML shape.
*/
void Entity::Draw(sf::RenderWindow& window) const
{
    if (m_shape)
        window.draw(*m_shape);
}

void Entity::SetPosition(const math::Vector2<float>& position)
{
    m_position = position;
    if (m_shape)
        m_shape->setPosition(util::SFMLConverter<float>::Convert(m_position));
}

void Entity::Move(const math::Vector2<float>& delta)
{
    m_position += delta;
    if (m_shape)
        m_shape->setPosition(util::SFMLConverter<float>::Convert(m_position));
}

math::Vector2<float> Entity::GetPosition() const
{
    return m_position;
}

void Entity::SetRotation(float angle)
{
    m_rotation = angle;
    if (m_shape)
        m_shape->setRotation(sf::degrees(angle));
}

float Entity::GetRotation() const
{
    return m_rotation;
}
