#include "Player.h"
#include <cmath>

Player::Player()
    : m_velocity(0.f, 0.f),
    m_acceleration(300.f), m_maxSpeed(400.f), m_rotationSpeed(180.f)
{
	// Triangle ship shape
    m_shape.setPointCount(3);
    m_shape.setPoint(0, util::SFMLConverter<float>::Convert(math::Vector2(0.f, -20.f)));
    m_shape.setPoint(1, util::SFMLConverter<float>::Convert(math::Vector2(10.f, 10.f)));
    m_shape.setPoint(2, util::SFMLConverter<float>::Convert(math::Vector2(-10.f, 10.f)));

    // Thruster shape
    m_thrusterShape.setPointCount(3);
    m_thrusterShape.setPoint(0, util::SFMLConverter<float>::Convert(math::Vector2(-8.f, 10.f)));
    m_thrusterShape.setPoint(1, util::SFMLConverter<float>::Convert(math::Vector2(8.f, 10.f)));
    m_thrusterShape.setPoint(2, util::SFMLConverter<float>::Convert(math::Vector2(0.f, 26.f)));

    m_thrusterShape.setFillColor(sf::Color(255, 140, 0)); // Orange
    m_thrusterShape.setOrigin(util::SFMLConverter<float>::Convert(math::Vector2(0.f, 0.f)));
    m_thrusterShape.setScale((util::SFMLConverter<float>::Convert(math::Vector2(1.f, 0.f))));

	// Initial position at center of window
    m_position = math::Vector2<float>(400.f, 300.f);
}

void Player::Update(float deltaTime, const sf::RenderWindow& window)
{
    if (!m_isAlive) return;

	// Rotation
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q))
        m_rotation -= m_rotationSpeed * deltaTime;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
        m_rotation += m_rotationSpeed * deltaTime;

	// Acceleration
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Z))
    {
        m_isMoving = true;
        math::Vector3<float> forward3 = m_transform.GetColumn(1);
        math::Vector2<float> forward(-forward3.x, -forward3.y);
        m_velocity += forward * m_acceleration * deltaTime;

		// Limit speed
        if (m_velocity.Length() > m_maxSpeed)
            m_velocity = m_velocity.Normalized() * m_maxSpeed;

        // Thruster animation using accumulated time
        m_thrusterTimer += deltaTime;
        float flicker = 0.8f + 0.2f * std::sin(10.0f * m_thrusterTimer);
        flicker = std::clamp(flicker, 0.0f, 2.0f);

        // Scale
        m_thrusterShape.setScale((util::SFMLConverter<float>::Convert(math::Vector2(1.f, flicker))));
    }
    else
    {
        m_isMoving = false;
        m_thrusterTimer = 0.0f;
        m_thrusterShape.setScale((util::SFMLConverter<float>::Convert(math::Vector2(1.f, 0.f))));
    }

	// Shooting
    m_fireCooldown -= deltaTime;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) && m_fireCooldown <= 0.f)
    {
        m_fireCooldown = 0.3f;
        m_bullets.push_back(std::make_unique<Bullet>(m_position, m_rotation,
            math::Vector2<float>(window.getSize().x, window.getSize().y)));
    }

	// Movement
    m_position += m_velocity * deltaTime;

    // Wrap-around
    WrapAround(window);

	// Transformation matrix update
    float rad = m_rotation * std::numbers::pi_v<float> / 180.f;

	// Rotation matrix around Z
    math::Mat3<float> rotation = math::Mat3<float>::RotationZ(rad);

	// Translation matrix
    math::Mat3<float> translation = math::Mat3<float>::Identity();
    translation(0, 2) = m_position.x;
    translation(1, 2) = m_position.y;

    m_transform = translation * rotation;
      
    // Update bullets
    math::Vector2<float> windowSize(window.getSize().x, window.getSize().y);
    for (auto& bullet : m_bullets)
        bullet->Update(deltaTime, windowSize);
}

void Player::OnHit()
{
    m_life--;
    if (m_life <= 0)
        m_isAlive = false;
}

void Player::Draw(sf::RenderWindow& window)
{
    sf::Transform sfTransform(
        m_transform(0, 0), m_transform(0, 1), m_transform(0, 2),
        m_transform(1, 0), m_transform(1, 1), m_transform(1, 2),
        m_transform(2, 0), m_transform(2, 1), m_transform(2, 2)
    );

    if(m_isMoving)
		window.draw(m_thrusterShape, sfTransform);

    window.draw(m_shape, sfTransform);
}

void Player::WrapAround(const sf::RenderWindow& window)
{
    if (m_position.x < 0) m_position.x += window.getSize().x;
    else if (m_position.x > window.getSize().x) m_position.x -= window.getSize().x;

    if (m_position.y < 0) m_position.y += window.getSize().y;
    else if (m_position.y > window.getSize().y) m_position.y -= window.getSize().y;
}
