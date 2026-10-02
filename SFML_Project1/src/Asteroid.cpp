#include "Asteroid.h"

Asteroid::Asteroid(const math::Vector2<float>& windowSize)
{
	// Random size
    m_size = 1 + rand() % 3;

    // Init
    InitializeShape();

	// Spawn on random side
    int side = rand() % 4;
    switch (side)
    {
    case 0: m_position = math::Vector2<float>(0, static_cast<float>(rand() % static_cast<int>(windowSize.y))); break;            // Left
    case 1: m_position = math::Vector2<float>(windowSize.x, static_cast<float>(rand() % static_cast<int>(windowSize.y))); break; // Right
    case 2: m_position = math::Vector2<float>(static_cast<float>(rand() % static_cast<int>(windowSize.x)), 0); break;            // Up
    case 3: m_position = math::Vector2<float>(static_cast<float>(rand() % static_cast<int>(windowSize.x)), windowSize.y); break; // Down
    }

    // Random speed and direction
    float speed = 50.f + static_cast<float>(rand() % 100);
    float angle = static_cast<float>(rand()) / RAND_MAX * 2.f * std::numbers::pi_v<float>;
    m_velocity = math::Vector2<float>(std::cos(angle), std::sin(angle)) * speed;

    m_rotation = 0.f;
    m_rotationSpeed = static_cast<float>((rand() % 40) - 20.f); // -20 à +20 deg/s
}

Asteroid::Asteroid(const math::Vector2<float>& position, int size)
{
    m_size = size;
    m_position = position;

    InitializeShape();

    // Slight random direction for fragments
    float angle = static_cast<float>(rand()) / RAND_MAX * 2.f * std::numbers::pi_v<float>;
    float speed = 70.f + static_cast<float>(rand() % 80);
    m_velocity = math::Vector2<float>(std::cos(angle), std::sin(angle)) * speed;

    m_rotation = 0.f;
    m_rotationSpeed = static_cast<float>((rand() % 40) - 20.f);
}

void Asteroid::InitializeShape()
{
    auto shape = new sf::ConvexShape();
    shape->setPointCount(6);
    float radius = m_size * 15.f;

    for (int i = 0; i < 6; ++i)
    {
        float angle = i * 2.f * std::numbers::pi_v<float> / 6.f + ((rand() % 20) * std::numbers::pi_v<float> / 180.f);
        float r = radius * (0.8f + static_cast<float>(rand() % 40) / 100.f);
        shape->setPoint(i, util::SFMLConverter<float>::Convert(math::Vector2(std::cos(angle) * r, std::sin(angle) * r)));
    }

    shape->setFillColor(sf::Color::Transparent);
    shape->setOutlineColor(sf::Color::White);
    shape->setOutlineThickness(2.f);

    m_shape = shape; // Using sf::Shape* from Entity
}

void Asteroid::Update(float deltaTime, const math::Vector2<float>& windowSize)
{
    // Mouvement
    m_position += m_velocity * deltaTime;
    m_rotation += m_rotationSpeed * deltaTime;

    // Wrap-around
    WrapAround(windowSize);

	// Constructing the transformation matrix
    float rad = m_rotation * std::numbers::pi_v<float> / 180.f;

    // Rotation matrix around Z
    math::Mat3<float> rotation = math::Mat3<float>::RotationZ(rad);

	// Translation matrix
    math::Mat3<float> translation = math::Mat3<float>::Identity();
    translation(0, 2) = m_position.x;
    translation(1, 2) = m_position.y;

    m_transform = translation * rotation;
}

void Asteroid::WrapAround(const math::Vector2<float>& windowSize)
{
    if (m_position.x < 0) m_position.x += windowSize.x;
    else if (m_position.x > windowSize.x) m_position.x -= windowSize.x;

    if (m_position.y < 0) m_position.y += windowSize.y;
    else if (m_position.y > windowSize.y) m_position.y -= windowSize.y;
}

void Asteroid::Draw(sf::RenderWindow& window) const
{
    if (!m_shape) return;

    sf::Transform sfTransform(
        m_transform(0, 0), m_transform(0, 1), m_transform(0, 2),
        m_transform(1, 0), m_transform(1, 1), m_transform(1, 2),
        m_transform(2, 0), m_transform(2, 1), m_transform(2, 2)
    );

    window.draw(*m_shape, sfTransform);
}

float Asteroid::GetSize() const
{
    return static_cast<float>(m_size) * 15.f;
}

int Asteroid::GetSizeIndex() const
{
    return m_size;
}


