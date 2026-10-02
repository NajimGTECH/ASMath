#pragma once
#include "Asteroid.h"
#include "Bullet.h"
#include "Player.h"

#include <SFML/Graphics.hpp>

#include <memory>
#include <vector>

class Game
{
public:

	Game();

	void Run();

private:

	// Core
	sf::RenderWindow m_window;
	sf::Clock m_clock;
	float m_deltaTime = 0.f;

	// Game Elements
	int m_score = 0;
	float m_asteroidSpawnTimer = 0.f;
	const float m_ASTEROID_SPAWN_INTERVAL = 6.f;

	// Entities
	Player m_player;
	std::vector<std::unique_ptr<Asteroid>> m_asteroids;

	// UIs
	sf::Font m_font;
	sf::Text m_gameOverText;
	sf::Text m_livesText;
	sf::Text m_scoreText;

	// Game State
	enum class State {InGame, GameOver};
	State m_state = State::InGame;

private:

	void PollEvents();
	void Update();
	void Render();
	void HandleCollisions();
	void SpawnAsteroids(int count);
	void ResetGame();
};