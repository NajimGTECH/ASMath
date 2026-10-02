#include "Game.h"
#include <optional>
#include <string>

Game::Game()
	: m_window(sf::VideoMode({ 800, 600 }), "Asteroids"), m_gameOverText(m_font), m_livesText(m_font), m_scoreText(m_font)
{
    m_window.setFramerateLimit(60);

    std::srand(static_cast<unsigned>(std::time(nullptr)));

    if(!m_font.openFromFile("assets/fonts/Dengb.otf"))
        std::cerr << "Error: could not load font 'Dengb.otf'\n";

	// Game Over Text
    m_gameOverText.setCharacterSize(40);
    m_gameOverText.setFillColor(sf::Color::Red);
    m_gameOverText.setString("GAME OVER");
    m_gameOverText.setPosition(util::SFMLConverter<float>::Convert(math::Vector2<float>(220.f, 250.f)));

	// Lives Text
    m_livesText.setCharacterSize(30);
    m_livesText.setFillColor(sf::Color::White);
    m_livesText.setString("Lives: _");
    m_livesText.setPosition(util::SFMLConverter<float>::Convert(math::Vector2<float>(0.f, 0.f)));


	// Score Text
    m_scoreText.setCharacterSize(30);
    m_scoreText.setFillColor(sf::Color::White);
    m_scoreText.setString("Score: _");
    m_scoreText.setPosition(util::SFMLConverter<float>::Convert(math::Vector2<float>(0.f, 35.f)));

    SpawnAsteroids(5);
}

void Game::Run()
{
    while (m_window.isOpen())
    {
        PollEvents();
        m_deltaTime = m_clock.restart().asSeconds();

        if (m_state == State::InGame)
            Update();

        Render();
    }
}

void Game::PollEvents()
{
    while (const std::optional event = m_window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
            m_window.close();
    }

    if (m_state == State::GameOver)
    {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R))
            ResetGame();
    }
}

void Game::Update()
{
    m_player.Update(m_deltaTime, m_window);

    math::Vector2<float> windowSize(
        static_cast<float>(m_window.getSize().x),
        static_cast<float>(m_window.getSize().y));

    // Asteroid spawning timer
    m_asteroidSpawnTimer += m_deltaTime;
    if (m_asteroidSpawnTimer >= m_ASTEROID_SPAWN_INTERVAL)
    {
        m_asteroidSpawnTimer = 0.f;

        math::Vector2<float> windowSize(
            static_cast<float>(m_window.getSize().x),
            static_cast<float>(m_window.getSize().y));

        m_asteroids.push_back(std::make_unique<Asteroid>(windowSize));
    }

    for (auto& asteroid : m_asteroids)
        asteroid->Update(m_deltaTime, windowSize);

    HandleCollisions();

    m_gameOverText.setString("GAME OVER\nFinal Score: " + std::to_string(m_score) + "\nPress R to Restart");
    m_livesText.setString("Lives: " + std::to_string(m_player.GetLife()));
    m_scoreText.setString("Score: " + std::to_string(m_score));

    if (!m_player.IsAlive())
        m_state = State::GameOver;
}

void Game::Render()
{
    m_window.clear(sf::Color::Black);

    if (m_player.IsAlive())
        m_player.Draw(m_window);

    if (m_state == State::InGame)
    {
        for (auto& asteroid : m_asteroids)
            asteroid->Draw(m_window);

        for (auto& bullet : m_player.GetBullets())
            bullet->Draw(m_window);

        m_window.draw(m_livesText);
        m_window.draw(m_scoreText);
    }

    if (m_state == State::GameOver)
        m_window.draw(m_gameOverText);

    m_window.display();
}

void Game::HandleCollisions()
{
    // Bullet <-> Asteroid
    auto& bullets = m_player.GetBullets();

    std::vector<std::unique_ptr<Asteroid>> newAsteroids;
    std::vector<size_t> asteroidsToRemove;
    std::vector<size_t> bulletsToRemove;

    for (size_t b = 0; b < bullets.size(); ++b)
    {
        for (size_t a = 0; a < m_asteroids.size(); ++a)
        {
            float dist = (bullets[b]->GetPosition() - m_asteroids[a]->GetPosition()).Length();
            float collisionRadius = m_asteroids[a]->GetSize();

            if (dist < collisionRadius)
            {
                int size = m_asteroids[a]->GetSizeIndex();

                if (size > 1)
                {
                    // Split asteroid into smaller ones
                    for (int i = 0; i < 2; ++i)
                        newAsteroids.push_back(std::make_unique<Asteroid>(m_asteroids[a]->GetPosition(), size - 1));
                }
                else
                {
                    m_score += 100;
                }

                asteroidsToRemove.push_back(a);
                bulletsToRemove.push_back(b);
                break;
            }
        }
    }

    // Apply removals AFTER all checks
    std::sort(asteroidsToRemove.rbegin(), asteroidsToRemove.rend());
    for (size_t idx : asteroidsToRemove)
        m_asteroids.erase(m_asteroids.begin() + idx);

    std::sort(bulletsToRemove.rbegin(), bulletsToRemove.rend());
    for (size_t idx : bulletsToRemove)
        bullets.erase(bullets.begin() + idx);

    // Add new fragments now
    for (auto& newAsteroid : newAsteroids)
        m_asteroids.push_back(std::move(newAsteroid));

    // Player <-> Asteroid
    const float playerRadius = 10.f;

    for (auto asteroidIt = m_asteroids.begin(); asteroidIt != m_asteroids.end();)
    {
        float dist = (m_player.GetPosition() - (*asteroidIt)->GetPosition()).Length();
        float collisionRadius = (*asteroidIt)->GetSize() + playerRadius;

        if (dist < collisionRadius)
        {
            m_player.OnHit();

            // Remove asteroid
            asteroidIt = m_asteroids.erase(asteroidIt);
            break;
        }
        else
        {
            ++asteroidIt;
        }
    }
}

void Game::SpawnAsteroids(int count)
{
    math::Vector2<float> windowSize(
        static_cast<float>(m_window.getSize().x),
        static_cast<float>(m_window.getSize().y));

    m_asteroids.clear();
    m_asteroids.reserve(count);

    for (int i = 0; i < count; ++i)
        m_asteroids.push_back(std::make_unique<Asteroid>(windowSize));
}

void Game::ResetGame()
{
	m_score = 0;         // Reset score
	m_asteroids.clear(); // Reset asteroids
    m_player = Player(); // Reset player

    SpawnAsteroids(5);
    m_state = State::InGame;
}