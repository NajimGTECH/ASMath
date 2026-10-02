#pragma once
#include "Vector2.h"
#include "Mat3.h"

#include "../utils/MathsToSFML.h"

#include <SFML/Graphics.hpp>

class Entity
{
public:

    Entity();
    virtual ~Entity() = default;

    virtual void Update(float deltaTime);
    virtual void Draw(sf::RenderWindow& window) const;
    void Move(const math::Vector2<float>& delta);
   
	// Setters
    void SetRotation(float angle);
    void SetPosition(const math::Vector2<float>& position);

	// Getters
    float GetRotation() const;
    math::Vector2<float> GetPosition() const;

protected:

    // Visual
    sf::Shape* m_shape;    

	// World Transform
    math::Vector2<float> m_position;
    math::Vector2<float> m_velocity;
    math::Mat3<float> m_transform;
	float m_rotation = 0.f;             // In degrees
};

