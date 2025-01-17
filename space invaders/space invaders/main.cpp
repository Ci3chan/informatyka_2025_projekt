#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <memory>
#include <cstdlib>
#include <ctime>

// Forward declarations
class MainMenu;
class HowToPlay;
class Life;
class Game;

// Life class implementation
class Life {
private:
    int lives;
    sf::Font font;
    sf::Text lifeText;

    void updateText() {
        lifeText.setString("Lives: " + std::to_string(lives));
    }

public:
    Life() : lives(3) {
        if (!font.loadFromFile("arial.ttf")) {
            throw std::runtime_error("Failed to load font arial.ttf");
        }
        lifeText.setFont(font);
        lifeText.setCharacterSize(20);
        lifeText.setFillColor(sf::Color::White);
        lifeText.setPosition(10.0f, 40.0f);
        updateText();
    }

    void loseLife() {
        if (lives > 0) {
            lives--;
            updateText();
        }
    }

    int getLives() const { return lives; }
    void draw(sf::RenderWindow& window) { window.draw(lifeText); }
};

// Game class implementation
class Game {
private:
    struct GameState {
        bool isGameOver;
        bool isPaused;
        bool isBackgroundRed;
        int score;
        
        GameState() : isGameOver(false), isPaused(false), 
                     isBackgroundRed(false), score(0) {}
    };

    sf::RenderWindow& window;
    GameState state;
    
    // Game objects
    sf::Sprite player;
    std::vector<sf::Sprite> bullets;
    std::vector<sf::Sprite> enemies;
    std::vector<sf::Sprite> enemyBullets;
    Life life;

    // Resources
    sf::Texture playerTexture;
    sf::Texture enemyTexture;
    sf::Texture bulletTexture;
    sf::Font font;

    // UI elements
    sf::Text scoreText;
    sf::Text gameOverText;
    sf::Text finalScoreText;
    sf::Text returnToMenuText;

    // Timers
    sf::Clock shootClock;
    sf::Clock enemyShootClock;
    sf::Clock enemySpawnClock;
    sf::Clock gameClock;
    sf::Clock redBackgroundClock;

    void initializeResources() {
        if (!playerTexture.loadFromFile("player.png") ||
            !enemyTexture.loadFromFile("enemy.png") ||
            !bulletTexture.loadFromFile("bullet.png") ||
            !font.loadFromFile("arial.ttf")) {
            throw std::runtime_error("Failed to load resources");
        }
    }

    void initializePlayer() {
        player.setTexture(playerTexture);
        player.setScale(0.08f, 0.08f);
        player.setPosition(375.0f, 550.0f);
    }

    void initializeUI() {
        // Score text
        scoreText.setFont(font);
        scoreText.setCharacterSize(20);
        scoreText.setFillColor(sf::Color::White);
        scoreText.setPosition(10.0f, 10.0f);
        updateScoreText();

        // Game over text
        gameOverText.setFont(font);
        gameOverText.setString("GAME OVER");
        gameOverText.setCharacterSize(50);
        gameOverText.setFillColor(sf::Color::Red);
        gameOverText.setPosition(300.0f, 200.0f);

        // Final score text
        finalScoreText.setFont(font);
        finalScoreText.setCharacterSize(30);
        finalScoreText.setFillColor(sf::Color::White);
        finalScoreText.setPosition(300.0f, 280.0f);

        // Return to menu text
        returnToMenuText.setFont(font);
        returnToMenuText.setString("Press Enter to return to menu");
        returnToMenuText.setCharacterSize(20);
        returnToMenuText.setFillColor(sf::Color::White);
        returnToMenuText.setPosition(280.0f, 350.0f);
    }

    void updateScoreText() {
        scoreText.setString("Score: " + std::to_string(state.score));
    }

    void handleGameOver() {
        state.isGameOver = true;
        state.isPaused = true;
        finalScoreText.setString("Final Score: " + std::to_string(state.score));
    }

public:
    Game(sf::RenderWindow& win) : window(win) {
        initializeResources();
        initializePlayer();
        initializeUI();
    }

    void handleEvents() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (state.isGameOver && event.type == sf::Event::KeyPressed && 
                event.key.code == sf::Keyboard::Enter) {
                return; // Return to menu
            }
        }
    }

    void update() {
        if (state.isGameOver || state.isPaused) return;

        float deltaTime = gameClock.restart().asSeconds();

        // Player movement
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) && player.getPosition().x > 0) {
            player.move(-300.0f * deltaTime, 0.0f);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) && 
            player.getPosition().x + player.getGlobalBounds().width < 800) {
            player.move(300.0f * deltaTime, 0.0f);
        }

        // Shooting
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && 
            shootClock.getElapsedTime().asSeconds() >= 0.5f) {
            sf::Sprite bullet;
            bullet.setTexture(bulletTexture);
            bullet.setScale(0.2f, 0.5f);
            bullet.setPosition(
                player.getPosition().x + player.getGlobalBounds().width / 2 - 2.5f,
                player.getPosition().y
            );
            bullets.push_back(bullet);
            shootClock.restart();
        }

        // Enemy spawning
        if (enemySpawnClock.getElapsedTime().asSeconds() >= 1.0f && enemies.size() < 5) {
            spawnEnemy();
            enemySpawnClock.restart();
        }

        // Enemy shooting
        if (enemyShootClock.getElapsedTime().asSeconds() >= 2.5f) {
            for (auto& enemy : enemies) {
                spawnEnemyBullet(enemy);
            }
            enemyShootClock.restart();
        }

        // Update projectiles and enemies
        updateProjectiles(deltaTime);
        updateEnemies(deltaTime);
        handleCollisions();

        // Check for game over
        if (life.getLives() <= 0) {
            handleGameOver();
        }
    }

    void updateProjectiles(float deltaTime) {
        // Update player bullets
        for (auto& bullet : bullets) {
            bullet.move(0.0f, -300.0f * deltaTime);
        }
        bullets.erase(
            std::remove_if(bullets.begin(), bullets.end(),
                [](const sf::Sprite& b) { return b.getPosition().y < 0; }
            ),
            bullets.end()
        );

        // Update enemy bullets
        for (auto& enemyBullet : enemyBullets) {
            enemyBullet.move(0.0f, 200.0f * deltaTime);
        }
        enemyBullets.erase(
            std::remove_if(enemyBullets.begin(), enemyBullets.end(),
                [](const sf::Sprite& eb) { return eb.getPosition().y > 600; }
            ),
            enemyBullets.end()
        );
    }

    void updateEnemies(float deltaTime) {
        for (auto& enemy : enemies) {
            enemy.move(0.0f, 100.0f * deltaTime);
        }
        enemies.erase(
            std::remove_if(enemies.begin(), enemies.end(),
                [](const sf::Sprite& e) { return e.getPosition().y > 600; }
            ),
            enemies.end()
        );
    }

    void handleCollisions() {
        // Player bullets hitting enemies
        for (auto& bullet : bullets) {
            for (auto& enemy : enemies) {
                if (bullet.getGlobalBounds().intersects(enemy.getGlobalBounds())) {
                    bullet.setPosition(-10.0f, -10.0f);
                    enemy.setPosition(-10.0f, -10.0f);
                    state.score++;
                    updateScoreText();
                }
            }
        }

        // Enemy bullets hitting player
        for (auto& enemyBullet : enemyBullets) {
            if (enemyBullet.getGlobalBounds().intersects(player.getGlobalBounds())) {
                enemyBullet.setPosition(-10.0f, -10.0f);
                life.loseLife();
                state.isBackgroundRed = true;
                redBackgroundClock.restart();
            }
        }
    }

    void render() {
        if (state.isBackgroundRed && redBackgroundClock.getElapsedTime().asSeconds() < 0.1f) {
            window.clear(sf::Color::Red);
        } else {
            state.isBackgroundRed = false;
            window.clear(sf::Color::Black);
        }

        if (state.isGameOver) {
            window.draw(gameOverText);
            window.draw(finalScoreText);
            window.draw(returnToMenuText);
        } else {
            window.draw(player);
            for (const auto& bullet : bullets) window.draw(bullet);
            for (const auto& enemy : enemies) window.draw(enemy);
            for (const auto& enemyBullet : enemyBullets) window.draw(enemyBullet);
            window.draw(scoreText);
            life.draw(window);
        }

        window.display();
    }

    void spawnEnemy() {
        sf::Sprite enemy;
        enemy.setTexture(enemyTexture);
        enemy.setScale(0.08f, 0.08f);
        enemy.setPosition(static_cast<float>(rand() % 760), 0.0f);
        enemies.push_back(enemy);
    }

    void spawnEnemyBullet(const sf::Sprite& enemy) {
        sf::Sprite enemyBullet;
        enemyBullet.setTexture(bulletTexture);
        enemyBullet.setScale(0.2f, 0.5f);
        enemyBullet.setPosition(
            enemy.getPosition().x + enemy.getGlobalBounds().width / 2 - 2.5f,
            enemy.getPosition().y + enemy.getGlobalBounds().height
        );
        enemyBullets.push_back(enemyBullet);
    }

    void run() {
        while (window.isOpen()) {
            handleEvents();
            if (state.isGameOver && 
                sf::Keyboard::isKeyPressed(sf::Keyboard::Enter)) {
                return; // Return to menu
            }
            update();
            render();
        }
    }
};

// Klasa Menu G³ównego
class MainMenu {
private:
    sf::RenderWindow& window;
    sf::Font font;
    sf::Text title;
    sf::Text playOption;
    sf::Text howToPlayOption;
    sf::Text exitOption;
    int selectedOption;

    void render() {
        window.clear();
        window.draw(title);
        window.draw(playOption);
        window.draw(howToPlayOption);
        window.draw(exitOption);
        window.display();
    }

public:
    MainMenu(sf::RenderWindow& win) : window(win), selectedOption(0) {
        if (!font.loadFromFile("arial.ttf")) {
            throw std::runtime_error("Failed to load font arial.ttf");
        }

        // Inicjalizacja tytu³u
        title.setFont(font);
        title.setString("Space Invaders");
        title.setCharacterSize(50);
        title.setFillColor(sf::Color::White);
        title.setPosition(200.0f, 50.0f);

        // Inicjalizacja opcji gry
        playOption.setFont(font);
        playOption.setString("Play");
        playOption.setCharacterSize(30);
        playOption.setFillColor(sf::Color::Yellow);
        playOption.setPosition(350.0f, 200.0f);

        // Inicjalizacja opcji instrukcji
        howToPlayOption.setFont(font);
        howToPlayOption.setString("How to Play");
        howToPlayOption.setCharacterSize(30);
        howToPlayOption.setFillColor(sf::Color::White);
        howToPlayOption.setPosition(320.0f, 300.0f);

        // Inicjalizacja opcji wyjœcia
        exitOption.setFont(font);
        exitOption.setString("Exit");
        exitOption.setCharacterSize(30);
        exitOption.setFillColor(sf::Color::White);
        exitOption.setPosition(350.0f, 400.0f);
    }

    int run() {
        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    return 2; // Wyjœcie
                }

                if (event.type == sf::Event::KeyPressed) {
                    if (event.key.code == sf::Keyboard::Up) {
                        selectedOption = (selectedOption + 2) % 3;
                    }
                    else if (event.key.code == sf::Keyboard::Down) {
                        selectedOption = (selectedOption + 1) % 3;
                    }
                    else if (event.key.code == sf::Keyboard::Enter) {
                        return selectedOption;
                    }
                }
            }

            // Aktualizacja kolorów opcji
            playOption.setFillColor(selectedOption == 0 ? sf::Color::Yellow : sf::Color::White);
            howToPlayOption.setFillColor(selectedOption == 1 ? sf::Color::Yellow : sf::Color::White);
            exitOption.setFillColor(selectedOption == 2 ? sf::Color::Yellow : sf::Color::White);

            render();
        }
        return 2; // Domyœlne wyjœcie
    }
};

// Klasa Instrukcji
class HowToPlay {
private:
    sf::RenderWindow& window;
    sf::Font font;
    sf::Text instructions;
    sf::Text backOption;

    void render() {
        window.clear();
        window.draw(instructions);
        window.draw(backOption);
        window.display();
    }

public:
    HowToPlay(sf::RenderWindow& win) : window(win) {
        if (!font.loadFromFile("arial.ttf")) {
            throw std::runtime_error("Failed to load font arial.ttf");
        }

        // Inicjalizacja tekstu instrukcji
        instructions.setFont(font);
        instructions.setString(
            "How to Play:\n\n"
            "- Use Left/Right Arrow keys to move\n"
            "- Press Space to shoot\n"
            "- Avoid enemy bullets\n"
            "- Destroy enemies to score points\n"
            "- You have 3 lives\n\n"
            "Press Enter to return to menu"
        );
        instructions.setCharacterSize(25);
        instructions.setFillColor(sf::Color::White);
        instructions.setPosition(50.0f, 100.0f);

        // Inicjalizacja opcji powrotu
        backOption.setFont(font);
        backOption.setString("Back to Menu");
        backOption.setCharacterSize(20);
        backOption.setFillColor(sf::Color::Yellow);
        backOption.setPosition(350.0f, 500.0f);
    }

    void run() {
        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                }

                if (event.type == sf::Event::KeyPressed &&
                    event.key.code == sf::Keyboard::Enter) {
                    return;
                }
            }

            render();
        }
    }
};

int main() {
    srand(static_cast<unsigned>(time(0)));
    sf::RenderWindow window(sf::VideoMode(800, 600), "Space Invaders");
    window.setFramerateLimit(60);

    MainMenu mainMenu(window);
    HowToPlay howToPlay(window);

    while (window.isOpen()) {
        int option = mainMenu.run();

        if (option == 0) {
            Game game(window);
            game.run();
        }
        else if (option == 1) {
            howToPlay.run();
        }
        else if (option == 2) {
            window.close();
        }
    }

    return 0;
}



