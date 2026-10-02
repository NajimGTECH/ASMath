#pragma once
#include "Bullet.h"
#include "Entity.h"

#include "../utils/MathsToSFML.h"

#include <memory>

class Player : public Entity
{
public:

    Player();
    
    void Update(float deltaTime, const sf::RenderWindow& window);
    void Draw(sf::RenderWindow& window);

    void OnHit();

	// Getters
    bool IsAlive() const { return m_isAlive; }
    int GetLife() const { return m_life; };
    const std::vector<std::unique_ptr<Bullet>>& GetBullets() const { return m_bullets; }
    std::vector<std::unique_ptr<Bullet>>& GetBullets() { return m_bullets; }

private:

    // Visuals
    sf::ConvexShape m_shape;
	sf::ConvexShape m_thrusterShape;

    // Timer
	float m_fireCooldown = 0.f;         // For shooting
	float m_thrusterTimer = 0.f;        // For the thruster animation

    // Movement
    math::Vector2<float> m_velocity;
	float m_acceleration;               // Pixels * s^-2
    float m_maxSpeed;
	float m_rotationSpeed;              // Degrees per second

    // States
    int m_life = 3;
    bool m_isAlive = true;
    bool m_isMoving = false;

	// Bullets
    std::vector<std::unique_ptr<Bullet>> m_bullets;

private:

    void WrapAround(const sf::RenderWindow& window);
};


