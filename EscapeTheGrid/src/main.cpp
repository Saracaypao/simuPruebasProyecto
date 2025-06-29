#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <iostream>
#include <cmath>
#include <memory>
#include <algorithm>

using namespace std;

enum class CellType { Empty, Wall, Start, Goal, Crystal };
enum class GameState { TitleScreen, Menu, Playing, Solved };

struct Cell { 
    CellType type; 
    bool visited = false; 
    bool isOnPath = false;
    bool hasBeenTraversed = false;
};

struct Button {
    sf::RectangleShape shape;
    sf::Text text;
    bool isHovered = false;
    
    Button(float x, float y, float width, float height, const string& label, sf::Font& font, sf::Color color) {
        shape = sf::RectangleShape(sf::Vector2f(width, height));
        shape.setPosition(x, y);
        shape.setFillColor(color);
        shape.setOutlineThickness(2.f);
        shape.setOutlineColor(sf::Color(255, 255, 255, 100));
        
        text = sf::Text(label, font, 12);
        text.setFillColor(sf::Color::White);
        
        sf::FloatRect textBounds = text.getLocalBounds();
        text.setPosition(
            x + (width - textBounds.width) / 2.f,
            y + (height - textBounds.height) / 2.f - 2.f
        );
    }
    
    bool contains(float x, float y) const {
        return shape.getGlobalBounds().contains(x, y);
    }
    
    void updateHover(float mouseX, float mouseY) {
        bool wasHovered = isHovered;
        isHovered = contains(mouseX, mouseY);
        
        if (isHovered && !wasHovered) {
            shape.setFillColor(sf::Color(
                shape.getFillColor().r + 30,
                shape.getFillColor().g + 30,
                shape.getFillColor().b + 30,
                shape.getFillColor().a
            ));
        } else if (!isHovered && wasHovered) {
            shape.setFillColor(sf::Color(
                shape.getFillColor().r - 30,
                shape.getFillColor().g - 30,
                shape.getFillColor().b - 30,
                shape.getFillColor().a
            ));
        }
    }
    
    void draw(sf::RenderWindow& window) {
        window.draw(shape);
        window.draw(text);
    }
};

class TitleScreen {
private:
    sf::Font font;
    sf::Text titleText;
    sf::Text subtitleText;
    sf::Text instructionText;
    sf::Text creditText;
    sf::Text playerText;
    sf::Clock animationClock;
    sf::Clock blinkClock;
    vector<sf::CircleShape> particles;
    vector<sf::Vector2f> particleVelocities;
    sf::RectangleShape backgroundGradient;
    sf::RectangleShape startButton;
    sf::Text startButtonText;
    bool buttonHovered = false;
    
    void initParticles(sf::Vector2u windowSize = sf::Vector2u(1200, 800)) {
        particles.clear();
        particleVelocities.clear();
        
        int particleCount = min(150, max(50, (int)((windowSize.x * windowSize.y) / 15000)));
        
        for (int i = 0; i < particleCount; i++) {
            sf::CircleShape particle(1.f + rand() % 3);
            particle.setFillColor(sf::Color(
                150 + rand() % 106,
                200 + rand() % 56,
                255,
                80 + rand() % 176
            ));
            particle.setPosition(rand() % windowSize.x, rand() % windowSize.y);
            particles.push_back(particle);
            
            particleVelocities.push_back(sf::Vector2f(
                (rand() % 20 - 10) / 15.f,
                (rand() % 20 - 10) / 15.f
            ));
        }
    }
    
public:
    TitleScreen(sf::Font& gameFont) {
        font = gameFont;
        
        backgroundGradient.setSize(sf::Vector2f(1200, 800));
        backgroundGradient.setFillColor(sf::Color(5, 5, 15));
        
        titleText.setString("ESCAPE THE GRID");
        titleText.setFont(font);
        titleText.setCharacterSize(48);
        titleText.setFillColor(sf::Color(180, 220, 255));
        titleText.setStyle(sf::Text::Bold);
        
        subtitleText.setString("CRYSTAL HAZE ADVENTURE");
        subtitleText.setFont(font);
        subtitleText.setCharacterSize(24);
        subtitleText.setFillColor(sf::Color(100, 255, 255));
        
        playerText.setString("1P\n00");
        playerText.setFont(font);
        playerText.setCharacterSize(12);
        playerText.setFillColor(sf::Color(255, 100, 100));
        
        startButton.setSize(sf::Vector2f(400, 70));
        startButton.setFillColor(sf::Color(50, 50, 150, 200));
        startButton.setOutlineThickness(3.f);
        startButton.setOutlineColor(sf::Color(255, 255, 100, 180));
        
        startButtonText.setString("CLICK TO CONTINUE");
        startButtonText.setFont(font);
        startButtonText.setCharacterSize(24);
        startButtonText.setFillColor(sf::Color(255, 255, 255));
        startButtonText.setStyle(sf::Text::Bold);
        
        instructionText.setString("Navigate the maze - Crystals' reflect reality.\nAnd the green exit 'Beware' the goal moves!");
        instructionText.setFont(font);
        instructionText.setCharacterSize(16);
        instructionText.setFillColor(sf::Color(150, 200, 255, 180));
        instructionText.setLineSpacing(1.5f);
        
        creditText.setString("TGC - 2025");
        creditText.setFont(font);
        creditText.setCharacterSize(12);
        creditText.setFillColor(sf::Color(255, 100, 255));
        
        initParticles();
    }
    
    void onWindowResize(sf::Vector2u newSize) {
        initParticles(newSize);
    }
    
    void update(sf::Vector2u windowSize) {
        float deltaTime = animationClock.restart().asSeconds();
        
        for (size_t i = 0; i < particles.size(); i++) {
            particles[i].move(particleVelocities[i] * deltaTime * 30.f);
            
            sf::Vector2f pos = particles[i].getPosition();
            if (pos.x < -10) particles[i].setPosition(windowSize.x + 10, pos.y);
            if (pos.x > windowSize.x + 10) particles[i].setPosition(-10, pos.y);
            if (pos.y < -10) particles[i].setPosition(pos.x, windowSize.y + 10);
            if (pos.y > windowSize.y + 10) particles[i].setPosition(pos.x, -10);
            
            float time = blinkClock.getElapsedTime().asSeconds();
            sf::Color baseColor = particles[i].getFillColor();
            float alpha = 80 + 120 * sin(time * 3 + i * 0.8f);
            particles[i].setFillColor(sf::Color(baseColor.r, baseColor.g, baseColor.b, (sf::Uint8)alpha));
        }
        
        float time = blinkClock.getElapsedTime().asSeconds();
        float pulseScale = 1.0f + 0.04f * sin(time * 2.5f);
        if (windowSize.x >= 1920) {
            pulseScale = 1.0f + 0.02f * sin(time * 2.5f);
        }
        titleText.setScale(pulseScale, pulseScale);
        
        float buttonAlpha = 180 + 75 * sin(time * 4);
        startButtonText.setFillColor(sf::Color(255, 255, 255, (sf::Uint8)buttonAlpha));
    }
    
    void updateButtonHover(float mouseX, float mouseY) {
        sf::FloatRect buttonBounds = startButton.getGlobalBounds();
        bool wasHovered = buttonHovered;
        buttonHovered = buttonBounds.contains(mouseX, mouseY);
        
        if (buttonHovered && !wasHovered) {
            startButton.setFillColor(sf::Color(100, 100, 200, 240));
            startButton.setScale(1.02f, 1.02f);
        } else if (!buttonHovered && wasHovered) {
            startButton.setFillColor(sf::Color(50, 50, 150, 200));
            startButton.setScale(1.0f, 1.0f);
        }
    }
    
    bool isStartButtonClicked(float mouseX, float mouseY) {
        return startButton.getGlobalBounds().contains(mouseX, mouseY);
    }
    
    void draw(sf::RenderWindow& window) {
    sf::Vector2u windowSize = window.getSize();
    float scale = min(
        windowSize.x / 1200.0f,
        windowSize.y / 800.0f
    );
    scale = max(0.5f, min(scale, 2.0f));

    sf::Vector2f center(windowSize.x / 2.0f, windowSize.y / 2.0f);

    backgroundGradient.setSize(sf::Vector2f(windowSize.x, windowSize.y));
    window.draw(backgroundGradient);

    for (auto& particle : particles) {
        window.draw(particle);
    }

    titleText.setCharacterSize(static_cast<unsigned int>(48 * scale));
    sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setOrigin(titleBounds.width/2, titleBounds.height/2);
    titleText.setPosition(center.x, center.y - 100);
    window.draw(titleText);

    subtitleText.setCharacterSize(static_cast<unsigned int>(24 * scale));
    sf::FloatRect subtitleBounds = subtitleText.getLocalBounds();
    subtitleText.setOrigin(subtitleBounds.width/2, subtitleBounds.height/2);
    subtitleText.setPosition(center.x, center.y - 40);
    window.draw(subtitleText);

    startButton.setSize(sf::Vector2f(400 * scale, 70 * scale));
    startButton.setOrigin(startButton.getSize().x/2, startButton.getSize().y/2);
    startButton.setPosition(center.x, center.y + 50);
    window.draw(startButton);

    startButtonText.setCharacterSize(static_cast<unsigned int>(24 * scale));
    sf::FloatRect buttonTextBounds = startButtonText.getLocalBounds();
    startButtonText.setOrigin(buttonTextBounds.width/2, buttonTextBounds.height/2);
    startButtonText.setPosition(center.x, center.y + 50);
    window.draw(startButtonText);

    instructionText.setCharacterSize(static_cast<unsigned int>(16 * scale));
    sf::FloatRect instrBounds = instructionText.getLocalBounds();
    instructionText.setOrigin(instrBounds.width/2, instrBounds.height/2);
    instructionText.setPosition(center.x, center.y + 150);
    window.draw(instructionText);

    creditText.setCharacterSize(static_cast<unsigned int>(12 * scale));
    sf::FloatRect creditBounds = creditText.getLocalBounds();
    creditText.setOrigin(creditBounds.width/2, creditBounds.height/2);
    creditText.setPosition(center.x, windowSize.y - 40 * scale);
    window.draw(creditText);
}
};

// Pantalla de victoria con efectos visuales y botón de reinicio
class VictoryScreen {
private:
    sf::Font font;
    sf::Text victoryText;
    sf::Text timeText;
    sf::Text movesText;
    sf::Clock animationClock;
    sf::Clock blinkClock;
    vector<sf::CircleShape> particles;
    vector<sf::Vector2f> particleVelocities;
    sf::RectangleShape backgroundGradient;
    sf::RectangleShape playAgainButton;
    sf::Text playAgainButtonText;
    bool buttonHovered = false;
    
    // Datos de la partida completada
    int finalMoves;
    float finalTime;
    
    void initParticles(sf::Vector2u windowSize = sf::Vector2u(1200, 800)) {
        particles.clear();
        particleVelocities.clear();
        
        // Ajustar la cantidad de partículas según el tamaño de pantalla
        int particleCount = min(200, max(50, (int)((windowSize.x * windowSize.y) / 12000)));
        
        // Reducir partículas en pantallas pequeñas para mejor rendimiento
        if (windowSize.x < 800) particleCount = min(120, particleCount);
        if (windowSize.x < 600) particleCount = min(80, particleCount);
        
        for (int i = 0; i < particleCount; i++) {
            // Ajustar el tamaño de las partículas según la pantalla
            float particleSize = 1.f + rand() % 4;
            if (windowSize.x < 800) particleSize = 1.f + rand() % 3;
            if (windowSize.x < 600) particleSize = 1.f + rand() % 2;
            
            sf::CircleShape particle(particleSize);
            particle.setFillColor(sf::Color(
                255,
                200 + rand() % 56,
                50 + rand() % 106,
                100 + rand() % 156
            ));
            particle.setPosition(rand() % windowSize.x, rand() % windowSize.y);
            particles.push_back(particle);
            
            // Ajustar la velocidad de las partículas según el tamaño de pantalla
            float velocityScale = 1.0f;
            if (windowSize.x < 800) velocityScale = 0.8f;
            if (windowSize.x < 600) velocityScale = 0.6f;
            
            particleVelocities.push_back(sf::Vector2f(
                ((rand() % 30 - 15) / 10.f) * velocityScale,
                ((rand() % 30 - 15) / 10.f) * velocityScale
            ));
        }
    }
    
    // Función para dibujar el trofeo dorado pixelado con diseño responsive
    void drawTrophy(sf::RenderWindow& window, sf::Vector2f center, float scale) {
        sf::Vector2u windowSize = window.getSize();
        
        // Ajustar el tamaño del pixel según el tamaño de ventana
        float pixelSize = 8.f * scale;
        if (windowSize.x < 800) pixelSize = 6.f * scale;
        if (windowSize.x < 600) pixelSize = 5.f * scale;
        
        // Ajustar el tamaño general del trofeo en pantallas pequeñas
        float trophyScale = 1.0f;
        if (windowSize.y < 600) trophyScale = 0.8f;
        if (windowSize.x < 600) trophyScale = 0.7f;
        
        pixelSize *= trophyScale;
        
        // Base del trofeo
        sf::RectangleShape base(sf::Vector2f(pixelSize * 8, pixelSize * 2));
        base.setFillColor(sf::Color(255, 215, 0)); // Dorado
        base.setPosition(center.x - pixelSize * 4, center.y + pixelSize * 4);
        window.draw(base);
        
        // Soporte del trofeo
        sf::RectangleShape support(sf::Vector2f(pixelSize * 2, pixelSize * 3));
        support.setFillColor(sf::Color(255, 215, 0));
        support.setPosition(center.x - pixelSize, center.y + pixelSize);
        window.draw(support);
        
        // Copa principal
        sf::RectangleShape cup(sf::Vector2f(pixelSize * 6, pixelSize * 4));
        cup.setFillColor(sf::Color(255, 215, 0));
        cup.setPosition(center.x - pixelSize * 3, center.y - pixelSize * 3);
        window.draw(cup);
        
        // Manijas del trofeo
        sf::RectangleShape leftHandle(sf::Vector2f(pixelSize, pixelSize * 2));
        leftHandle.setFillColor(sf::Color(255, 215, 0));
        leftHandle.setPosition(center.x - pixelSize * 4, center.y - pixelSize * 2);
        window.draw(leftHandle);
        
        sf::RectangleShape rightHandle(sf::Vector2f(pixelSize, pixelSize * 2));
        rightHandle.setFillColor(sf::Color(255, 215, 0));
        rightHandle.setPosition(center.x + pixelSize * 3, center.y - pixelSize * 2);
        window.draw(rightHandle);
        
        // Detalles brillantes adaptativos
        if (pixelSize > 4.0f) { // Solo mostrar detalles si hay suficiente espacio
            sf::RectangleShape shine1(sf::Vector2f(pixelSize, pixelSize));
            shine1.setFillColor(sf::Color(255, 255, 255, 200));
            shine1.setPosition(center.x - pixelSize * 2, center.y - pixelSize * 2);
            window.draw(shine1);
            
            sf::RectangleShape shine2(sf::Vector2f(pixelSize, pixelSize));
            shine2.setFillColor(sf::Color(255, 255, 255, 150));
            shine2.setPosition(center.x, center.y - pixelSize);
            window.draw(shine2);
        }
    }
    
public:
    VictoryScreen(sf::Font& gameFont) {
        font = gameFont;
        finalMoves = 0;
        finalTime = 0.0f;
        
        backgroundGradient.setSize(sf::Vector2f(1200, 800));
        backgroundGradient.setFillColor(sf::Color(10, 5, 20));
        
        victoryText.setString("YOU WIN!");
        victoryText.setFont(font);
        victoryText.setCharacterSize(64);
        victoryText.setFillColor(sf::Color(255, 215, 0)); // Dorado
        victoryText.setStyle(sf::Text::Bold);
        
        timeText.setString("");
        timeText.setFont(font);
        timeText.setCharacterSize(16);
        timeText.setFillColor(sf::Color(255, 255, 255));
        
        movesText.setString("");
        movesText.setFont(font);
        movesText.setCharacterSize(16);
        movesText.setFillColor(sf::Color(255, 255, 255));
        
        playAgainButton.setSize(sf::Vector2f(300, 60));
        playAgainButton.setFillColor(sf::Color(50, 150, 50, 200));
        playAgainButton.setOutlineThickness(3.f);
        playAgainButton.setOutlineColor(sf::Color(255, 255, 100, 180));
        
        playAgainButtonText.setString("PLAY AGAIN");
        playAgainButtonText.setFont(font);
        playAgainButtonText.setCharacterSize(20);
        playAgainButtonText.setFillColor(sf::Color(255, 255, 255));
        playAgainButtonText.setStyle(sf::Text::Bold);
        
        initParticles();
    }
    
    void setGameStats(int moves, float time) {
        finalMoves = moves;
        finalTime = time;
        
        int minutes = (int)finalTime / 60;
        int seconds = (int)finalTime % 60;
        timeText.setString("Time: " + to_string(minutes) + ":" + (seconds < 10 ? "0" : "") + to_string(seconds));
        movesText.setString("Moves: " + to_string(finalMoves));
    }
    
    void onWindowResize(sf::Vector2u newSize) {
        initParticles(newSize);
    }
    
    void update(sf::Vector2u windowSize) {
        float deltaTime = animationClock.restart().asSeconds();
        
        // Ajustar la velocidad de animación según el tamaño de pantalla
        float animationSpeed = 40.f;
        if (windowSize.x < 800) animationSpeed = 30.f;
        if (windowSize.x < 600) animationSpeed = 25.f;
        
        // Actualizar partículas doradas
        for (size_t i = 0; i < particles.size(); i++) {
            particles[i].move(particleVelocities[i] * deltaTime * animationSpeed);
            
            sf::Vector2f pos = particles[i].getPosition();
            if (pos.x < -10) particles[i].setPosition(windowSize.x + 10, pos.y);
            if (pos.x > windowSize.x + 10) particles[i].setPosition(-10, pos.y);
            if (pos.y < -10) particles[i].setPosition(pos.x, windowSize.y + 10);
            if (pos.y > windowSize.y + 10) particles[i].setPosition(pos.x, -10);
            
            float time = blinkClock.getElapsedTime().asSeconds();
            sf::Color baseColor = particles[i].getFillColor();
            float alpha = 100 + 155 * sin(time * 2 + i * 0.5f);
            particles[i].setFillColor(sf::Color(baseColor.r, baseColor.g, baseColor.b, (sf::Uint8)alpha));
        }
        
        // Animación pulsante del texto adaptativa
        float time = blinkClock.getElapsedTime().asSeconds();
        float pulseScale = 1.0f + 0.1f * sin(time * 3.0f);
        
        // Reducir la pulsación en pantallas muy pequeñas para mejor legibilidad
        if (windowSize.x < 600) {
            pulseScale = 1.0f + 0.05f * sin(time * 3.0f);
        }
        
        victoryText.setScale(pulseScale, pulseScale);
        
        // Animación del botón
        float buttonAlpha = 200 + 55 * sin(time * 5);
        playAgainButtonText.setFillColor(sf::Color(255, 255, 255, (sf::Uint8)buttonAlpha));
    }
    
    void updateButtonHover(float mouseX, float mouseY) {
        sf::FloatRect buttonBounds = playAgainButton.getGlobalBounds();
        bool wasHovered = buttonHovered;
        buttonHovered = buttonBounds.contains(mouseX, mouseY);
        
        if (buttonHovered && !wasHovered) {
            playAgainButton.setFillColor(sf::Color(80, 200, 80, 240));
            playAgainButton.setScale(1.05f, 1.05f);
        } else if (!buttonHovered && wasHovered) {
            playAgainButton.setFillColor(sf::Color(50, 150, 50, 200));
            playAgainButton.setScale(1.0f, 1.0f);
        }
    }
    
    bool isPlayAgainButtonClicked(float mouseX, float mouseY) {
        return playAgainButton.getGlobalBounds().contains(mouseX, mouseY);
    }
    
    void draw(sf::RenderWindow& window) {
        sf::Vector2u windowSize = window.getSize();
        float scale = min(
            windowSize.x / 1200.0f,
            windowSize.y / 800.0f
        );
        scale = max(0.5f, min(scale, 2.0f));

        sf::Vector2f center(windowSize.x / 2.0f, windowSize.y / 2.0f);

        // Fondo oscuro responsive
        backgroundGradient.setSize(sf::Vector2f(windowSize.x, windowSize.y));
        window.draw(backgroundGradient);

        // Partículas doradas
        for (auto& particle : particles) {
            window.draw(particle);
        }

        // Trofeo con mejor posicionamiento responsive
        float trophyOffset = 120 * scale;
        if (windowSize.y < 600) trophyOffset = 80 * scale;
        drawTrophy(window, sf::Vector2f(center.x, center.y - trophyOffset), scale);

        // Texto de victoria con escala adaptativa
        float victoryTextSize = 64 * scale;
        if (windowSize.x < 800) victoryTextSize = 48 * scale;
        if (windowSize.x < 600) victoryTextSize = 36 * scale;
        
        victoryText.setCharacterSize(static_cast<unsigned int>(victoryTextSize));
        sf::FloatRect victoryBounds = victoryText.getLocalBounds();
        victoryText.setOrigin(victoryBounds.width/2, victoryBounds.height/2);
        
        float victoryYOffset = -20 * scale;
        if (windowSize.y < 600) victoryYOffset = 10 * scale;
        victoryText.setPosition(center.x, center.y + victoryYOffset);
        window.draw(victoryText);

        // Estadísticas con mejor espaciado responsive
        float statsTextSize = 16 * scale;
        if (windowSize.x < 800) statsTextSize = 14 * scale;
        if (windowSize.x < 600) statsTextSize = 12 * scale;
        
        float statsStartY = center.y + 40 * scale;
        if (windowSize.y < 600) statsStartY = center.y + 60 * scale;
        
        timeText.setCharacterSize(static_cast<unsigned int>(statsTextSize));
        sf::FloatRect timeBounds = timeText.getLocalBounds();
        timeText.setOrigin(timeBounds.width/2, timeBounds.height/2);
        timeText.setPosition(center.x, statsStartY);
        window.draw(timeText);

        movesText.setCharacterSize(static_cast<unsigned int>(statsTextSize));
        sf::FloatRect movesBounds = movesText.getLocalBounds();
        movesText.setOrigin(movesBounds.width/2, movesBounds.height/2);
        movesText.setPosition(center.x, statsStartY + 30 * scale);
        window.draw(movesText);

        // Botón Play Again con dimensiones adaptativas
        float buttonWidth = 300 * scale;
        float buttonHeight = 60 * scale;
        float buttonYOffset = 130 * scale;
        
        if (windowSize.x < 800) {
            buttonWidth = 250 * scale;
            buttonHeight = 50 * scale;
        }
        if (windowSize.x < 600) {
            buttonWidth = 200 * scale;
            buttonHeight = 40 * scale;
        }
        if (windowSize.y < 600) {
            buttonYOffset = 150 * scale;
        }
        
        playAgainButton.setSize(sf::Vector2f(buttonWidth, buttonHeight));
        playAgainButton.setOrigin(playAgainButton.getSize().x/2, playAgainButton.getSize().y/2);
        playAgainButton.setPosition(center.x, center.y + buttonYOffset);
        window.draw(playAgainButton);

        float buttonTextSize = 20 * scale;
        if (windowSize.x < 800) buttonTextSize = 18 * scale;
        if (windowSize.x < 600) buttonTextSize = 16 * scale;
        
        playAgainButtonText.setCharacterSize(static_cast<unsigned int>(buttonTextSize));
        sf::FloatRect buttonTextBounds = playAgainButtonText.getLocalBounds();
        playAgainButtonText.setOrigin(buttonTextBounds.width/2, buttonTextBounds.height/2);
        playAgainButtonText.setPosition(center.x, center.y + buttonYOffset);
        window.draw(playAgainButtonText);
    }
};

int turnCount = 0;
const int TURNS_PER_EVENT = 5;
const int TURNS_TO_MOVE_GOAL = 10;
int turnsSinceLastGoalMove = 0;
int W, H;
vector<Cell> grid;
int startX, startY, goalX, goalY;
vector<pair<int,int>> path;
GameState gameState = GameState::TitleScreen;

float cellSize = 35.f;
float menuWidth = 300.f;
sf::Vector2f gameOffset(0, 0);
sf::View gameView, menuView;

int lastX = -1, lastY = -1;

// Función de validación de límites del laberinto
// Verifica que las coordenadas se encuentren dentro de los límites válidos
bool inside(int y, int x) {
    return x >= 0 && x < W && y >= 0 && y < H;
}

// Sistema de reflexión horizontal para cristales
// Implementa la mecánica de espejo horizontal cuando se activa un cristal
void reflectHorizontally(int crystalX, int crystalY) {
    for (int x = 0; x < crystalX; x++) {
        int reflectedX = crystalX + (crystalX - x);
        if (reflectedX < W) {
            grid[crystalY*W + reflectedX] = grid[crystalY*W + x];
        }
    }
    
    for (int x = crystalX + 1; x < W; x++) {
        int reflectedX = crystalX - (x - crystalX);
        if (reflectedX >= 0) {
            grid[crystalY*W + reflectedX] = grid[crystalY*W + x];
        }
    }
}

// Sistema de reflexión vertical para cristales
// Implementa la mecánica de espejo vertical cuando se activa un cristal
void reflectVertically(int crystalX, int crystalY) {
    for (int y = 0; y < crystalY; y++) {
        int reflectedY = crystalY + (crystalY - y);
        if (reflectedY < H) {
            grid[reflectedY*W + crystalX] = grid[y*W + crystalX];
        }
    }
    
    for (int y = crystalY + 1; y < H; y++) {
        int reflectedY = crystalY - (y - crystalY);
        if (reflectedY >= 0) {
            grid[reflectedY*W + crystalX] = grid[y*W + crystalX];
        }
    }
}

// Controlador dinámico del sistema de reflexión de cristales
// Determina el tipo de reflexión basado en la dirección del movimiento del jugador
void reflectDynamically(int crystalX, int crystalY, int directionX, int directionY) {
    cout << "Cristal activado en (" << crystalX << "," << crystalY << ") - Direccion: (" << directionX << "," << directionY << ")\n";
    
    if (directionX != 0) {
        reflectHorizontally(crystalX, crystalY);
        cout << "Reflejo HORIZONTAL activado (movimiento izquierda/derecha)\n";
    } else if (directionY != 0) {
        reflectVertically(crystalX, crystalY);
        cout << "Reflejo VERTICAL activado (movimiento arriba/abajo)\n";
    }
}

// Función de activación de cristales basada en movimiento
void reflectCrystals(int currentX, int currentY) {
    int directionX = 0, directionY = 0;
    
    if (lastX != -1 && lastY != -1) {
        directionX = currentX - lastX;
        directionY = currentY - lastY;
    }
    
    if (grid[currentY*W + currentX].type == CellType::Crystal) {
        reflectDynamically(currentX, currentY, directionX, directionY);
    }
    
    lastX = currentX;
    lastY = currentY;
}

// Sistema de verificación y actualización del objetivo
void verifyGoal(sf::CircleShape& goal) {
    if (grid[goalY*W + goalX].type != CellType::Goal) {
        grid[goalY*W + goalX].type = CellType::Goal;
    }
    
    goal.setFillColor(sf::Color(0, 255, 0, 255));
    goal.setOutlineColor(sf::Color::White);
    goal.setOutlineThickness(3.f);
    goal.setPosition(
        goalX * cellSize + cellSize/2 - cellSize/5,
        goalY * cellSize + cellSize/2 - cellSize/5
    );
}

// Algoritmo de reubicación dinámica del objetivo
void moveGoal(int currentX, int currentY, sf::CircleShape& goal) {
    int oldGoalX = goalX;
    int oldGoalY = goalY;
    
    vector<pair<int, int>> emptyCells;
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            if (grid[y*W + x].type == CellType::Empty && 
                !(x == currentX && y == currentY)) {
                emptyCells.emplace_back(y, x);
            }
        }
    }
    
    if (!emptyCells.empty()) {
        int index = rand() % emptyCells.size();
        auto [newY, newX] = emptyCells[index];
        
        grid[oldGoalY*W + oldGoalX].type = CellType::Empty;
        goalX = newX;
        goalY = newY;
        grid[goalY*W + goalX].type = CellType::Goal;
        
        verifyGoal(goal);
        cout << "La salida se ha movido a (" << goalX << ", " << goalY << ")!\n";
    }
    
    turnsSinceLastGoalMove = 0;
}

// Función para cargar el laberinto desde archivo - Gestiona la lectura e inicialización de la estructura del laberinto desde archivos externos
bool loadMaze(const string& path) {
    // Intento de apertura del archivo de configuración del laberinto
    ifstream file(path);
    if (!file.is_open()) {
        cout << "No se pudo abrir: " << path << endl;
        return false;
    }
    
    if (!(file >> W >> H >> startY >> startX >> goalY >> goalX)) {
        cout << "Error leyendo dimensiones del laberinto" << endl;
        return false;
    }
    
    // Cálculo del espacio de memoria requerido y optimización de recursos
    int totalCells = W * H;
    int memoryCapacity = min(totalCells, 2500);
    
    if (grid.size() != totalCells) {
        grid.clear();
        grid.reserve(memoryCapacity);
        grid.resize(totalCells);
    }
    
    string line;
    getline(file, line);
    
    for (int y = 0; y < H; y++) {
        if (!getline(file, line)) {
            cout << "Error leyendo linea " << y << endl;
            return false;
        }
        istringstream iss(line);
        for (int x = 0; x < W; x++) {
            string t;
            if (!(iss >> t)) {
                cout << "Error leyendo celda [" << x << "," << y << "]" << endl;
                return false;
            }
            if (t == "#") grid[y*W + x] = {CellType::Wall};
            else if (t == "S") grid[y*W + x] = {CellType::Start};
            else if (t == "G") grid[y*W + x] = {CellType::Goal};
            else if (t == "C" || t == "K") grid[y*W + x] = {CellType::Crystal};
            else grid[y*W + x] = {CellType::Empty};
        }
    }

    return true;
}

// Generador de laberinto por defecto
void createDefaultMaze() {
    W = 12; H = 10;
    startX = 1; startY = 1;
    goalX = 10; goalY = 8;
    
    grid.resize(W * H);

    for (auto& cell : grid) {
        cell = {CellType::Empty};
    }

    vector<pair<int,int>> walls = {
        {0,0},{0,1},{0,2},{0,3},{0,4},{0,5},{0,6},{0,7},{0,8},{0,9},
        {11,0},{11,1},{11,2},{11,3},{11,4},{11,5},{11,6},{11,7},{11,8},{11,9},
        {1,0},{2,0},{3,0},{4,0},{5,0},{6,0},{7,0},{8,0},{9,0},{10,0},
        {1,9},{2,9},{3,9},{4,9},{5,9},{6,9},{7,9},{8,9},{9,9},{10,9},
        {2,2},{2,3},{2,4},{4,2},{4,4},{4,5},{4,6},
        {6,1},{6,3},{6,5},{6,7},{8,2},{8,4},{8,6},{8,7},
        {3,7},{5,7},{7,7},{9,3},{9,5}
    };

    for (auto [x,y] : walls) {
        grid[y*W + x] = {CellType::Wall};
    }

    grid[3*W + 6] = {CellType::Crystal};
    grid[6*W + 9] = {CellType::Crystal};

    grid[startY*W + startX] = {CellType::Start};
    grid[goalY*W + goalX] = {CellType::Goal};
}

// Sistema de reinicialización del estado del juego
void resetGame(sf::CircleShape& goal) {
    gameState = GameState::Menu;
    turnCount = 0;
    turnsSinceLastGoalMove = 0;
    path.clear();

    if (!loadMaze("../assets/maze.txt") && !loadMaze("assets/maze.txt") && !loadMaze("maze.txt")) {
        createDefaultMaze();
    }

    for (auto& cell : grid) {
        cell.visited = false;
        cell.isOnPath = false;
        cell.hasBeenTraversed = false;
    }
    
    grid[startY*W + startX].hasBeenTraversed = true;
    
    lastX = startX;
    lastY = startY;
    
    verifyGoal(goal);
}

// Generador de eventos aleatorios del mapa
// Introduce cambios dinámicos en la estructura del laberinto durante el juego
void triggerMapEvent() {
    int rx = rand() % W;
    int ry = rand() % H;
    Cell& c = grid[ry*W + rx];
    if (c.type == CellType::Empty) {
        c.type = CellType::Wall;
        // Limpiar estados cuando se convierte en muro
        c.visited = false;
        c.isOnPath = false;
        c.hasBeenTraversed = false;
        cout << "Evento: aparece muro en (" << rx << "," << ry << ")\n";
    }
    else if (c.type == CellType::Wall) {
        c.type = CellType::Empty;
        // Limpiar estados cuando se convierte en celda vacía
        c.visited = false;
        c.isOnPath = false;
        c.hasBeenTraversed = false;
        cout << "Evento: desaparece muro en (" << rx << "," << ry << ")\n";
    }
}

// Algoritmo BFS para resolver el laberinto - implementación de búsqueda en anchura
void bfsSolve() {
    // Variables estáticas para memoización y optimización de cálculos repetitivos
    static vector<pair<int,int>> lastPath;
    static int lastStartX = -1, lastStartY = -1, lastGoalX = -1, lastGoalY = -1;
    
    // Verificación de validez del camino previamente calculado para evitar recálculos innecesarios
    if (lastStartX == startX && lastStartY == startY && lastGoalX == goalX && lastGoalY == goalY && !lastPath.empty()) {
        bool pathValid = true;
        // Validación de integridad del camino almacenado
        for (auto [y, x] : lastPath) {
            if (grid[y*W + x].type == CellType::Wall) {
                pathValid = false;
                break;
            }
        }
        
        if (pathValid) {
            path = lastPath;
            return;
        }
    }
    
    int tempGoalX = goalX;
    int tempGoalY = goalY;
    
    for (auto& cell : grid) {
        cell.visited = false;
        cell.isOnPath = false;
    }
    
    grid[tempGoalY*W + tempGoalX].type = CellType::Goal;
    goalX = tempGoalX;
    goalY = tempGoalY;
    
    path.clear();
    
    // Inicialización de estructuras de datos para el algoritmo BFS
    vector<vector<int>> cost(H, vector<int>(W, INT_MAX));
    vector<vector<pair<int,int>>> parent(H, vector<pair<int,int>>(W, {-1,-1}));
    queue<pair<int,int>> q;
    
    cost[startY][startX] = 0;
    q.push({startY, startX});
    
    // Vectores direccionales para exploración de adyacencias: arriba, abajo, derecha, izquierda
    int dirs[4][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}};
    bool found = false;

    // Implementación del algoritmo BFS - exploración sistemática por niveles
    while (!q.empty()) {
        auto [y,x] = q.front(); q.pop();
        
        if (y == goalY && x == goalX) { 
            found = true; 
            break; 
        }
        
        // Exploración de todas las direcciones factibles desde la posición actual
        for (auto& d : dirs) {
            int ny = y + d[0];
            int nx = x + d[1];
            
            // Validaciones de límites y obstáculos
            if (!inside(ny,nx)) continue;
            if (grid[ny*W+nx].type == CellType::Wall) continue;
            
            // Sistema de costos ponderados basado en tipos de celda
            int moveCost = 1;
            if (grid[ny*W+nx].type == CellType::Crystal) moveCost = 3;
            if (grid[ny*W+nx].hasBeenTraversed) moveCost = 1;
            
            int newCost = cost[y][x] + moveCost;
            
            if (newCost < cost[ny][nx]) {
                cost[ny][nx] = newCost;
                parent[ny][nx] = {y,x};
                q.push({ny,nx});
            }
        }
    }

    if (found) {
        // Algoritmo de retroceso para generar la secuencia completa de movimientos
        for (int cy = goalY, cx = goalX; cy != -1; ) {
            path.push_back({cy,cx});
            auto p = parent[cy][cx];
            cy = p.first; 
            cx = p.second;
        }
        reverse(path.begin(), path.end());
        
        // Almacenamiento del resultado para optimización de consultas futuras
        lastPath = path;
        lastStartX = startX; lastStartY = startY;
        lastGoalX = tempGoalX; lastGoalY = tempGoalY;
    }
}

// Sistema de coloración diferencial de celdas
sf::Color getCellColor(CellType type, int x, int y, bool visited, bool isOnPath, bool hasBeenTraversed) {
    // Los muros siempre deben tener su color específico, sin importar otros estados
    if (type == CellType::Wall) return sf::Color(40, 40, 40);
    
    if (type == CellType::Crystal) return sf::Color(0, 255, 255, 180);
    
    if (type == CellType::Goal) return sf::Color(0, 200, 0, 150);
    
    if (hasBeenTraversed) return sf::Color(192, 192, 192, 200);
    
    switch (type) {
        case CellType::Start: return sf::Color(100, 255, 100, 200);
        case CellType::Empty:
        default: 
            return ((x + y) % 2 == 0) ? sf::Color(120, 120, 200, 120) : sf::Color(100, 100, 180, 120);
    }
}

// Controlador de movimiento del jugador con validaciones
bool tryMovePlayer(int newX, int newY, int& currentX, int& currentY, sf::CircleShape& player, sf::CircleShape& goal) {
    if (inside(newY, newX) && grid[newY*W + newX].type != CellType::Wall) {
        currentX = newX;
        currentY = newY;
        sf::Vector2f newPos(
            newX * cellSize + cellSize/2 - cellSize/5,
            newY * cellSize + cellSize/2 - cellSize/5
        );
        player.setPosition(newPos);

        grid[currentY*W + currentX].hasBeenTraversed = true;
        
        if (grid[currentY*W + currentX].type == CellType::Crystal) {
            reflectCrystals(currentX, currentY);
        } else {
            lastX = currentX;
            lastY = currentY;
        }

        turnCount++;
        turnsSinceLastGoalMove++;
        
        if (turnsSinceLastGoalMove >= TURNS_TO_MOVE_GOAL) {
            moveGoal(currentX, currentY, goal);
        }
        
        if (turnCount % TURNS_PER_EVENT == 0) {
            triggerMapEvent();
        }

        return true;
    }
    return false;
}

// Generador de formas triangulares para representación visual
// Crea elementos gráficos triangulares con patrón alternante para las celdas
sf::ConvexShape makeTri(int x, int y) {
    sf::ConvexShape tri;
    tri.setPointCount(3);
    
    const float verticalGap = 8.f;
    const float horizontalGap = 3.f;
    bool up = (x + y) % 2 == 0;

    float px = x * cellSize + horizontalGap/2;
    float py = y * cellSize + verticalGap/2;
    float triWidth = cellSize - horizontalGap;
    float triHeight = cellSize - verticalGap;
    
    const float horizontalStretch = 10.f;
    const float verticalStretch = 0.f;
    
    float left = px - horizontalStretch/2;
    float right = px + triWidth + horizontalStretch/2;
    float centerX = px + triWidth / 2.f;
    
    float top = py;
    float bottom = py + triHeight + verticalStretch;

    if (up) {
        tri.setPoint(0, sf::Vector2f(centerX, top));
        tri.setPoint(1, sf::Vector2f(left, bottom));
        tri.setPoint(2, sf::Vector2f(right, bottom));
    } else {
        tri.setPoint(0, sf::Vector2f(centerX, bottom));
        tri.setPoint(1, sf::Vector2f(left, top));
        tri.setPoint(2, sf::Vector2f(right, top));
    }

    tri.setOutlineThickness(0.5f);
    tri.setOutlineColor(sf::Color(100, 100, 100, 100));
    return tri;
}


sf::Text createStyledText(const string& text, sf::Font& font, int size, sf::Color color, float x, float y) {
    sf::Text styledText(text, font, size);
    styledText.setFillColor(color);
    styledText.setPosition(x, y);
    return styledText;
}


sf::Vector2f windowToGameCoords(sf::Vector2i windowPos, const sf::RenderWindow& window) {
    return window.mapPixelToCoords(windowPos, gameView);
}


sf::Vector2f windowToMenuCoords(sf::Vector2i windowPos, const sf::RenderWindow& window) {
    return window.mapPixelToCoords(windowPos, menuView);
}


void updateViews(sf::RenderWindow& window) {
    sf::Vector2u windowSize = window.getSize();
    
    float gameWidth = W * cellSize;
    float gameHeight = H * cellSize;
    
    float gameViewWidth = windowSize.x - menuWidth;
    gameView.reset(sf::FloatRect(0, 0, gameWidth, gameHeight));
    gameView.setViewport(sf::FloatRect(0, 0, gameViewWidth / windowSize.x, 1.0f));
    
    menuView.reset(sf::FloatRect(0, 0, menuWidth, windowSize.y));
    menuView.setViewport(sf::FloatRect(gameViewWidth / windowSize.x, 0, menuWidth / windowSize.x, 1.0f));
}

// Función principal del programa - punto de entrada y controlador central del sistema
int main() {
    // Proceso de carga del laberinto con múltiples rutas de búsqueda
    if (!loadMaze("../assets/maze.txt") && !loadMaze("assets/maze.txt") && !loadMaze("maze.txt")) {
        createDefaultMaze();
    }

    float initialGameWidth = W * cellSize;
    float initialGameHeight = H * cellSize;
    float initialTotalWidth = initialGameWidth + menuWidth;
    
    // Inicialización del sistema de renderizado con configuraciones específicas
    sf::RenderWindow window(sf::VideoMode(initialTotalWidth, initialGameHeight), "Escape the Grid", sf::Style::Default);
    window.setFramerateLimit(60);
    
    updateViews(window);
    
    sf::Font font;
    if (!font.loadFromFile("../assets/PRESSSTART2P-REGULAR.TTF") && 
        !font.loadFromFile("assets/PRESSSTART2P-REGULAR.TTF") &&
        !font.loadFromFile("../assets/arial.ttf") && 
        !font.loadFromFile("assets/arial.ttf")) {
        cerr << "Error cargando fuente" << endl;
        return 1;
    }

    TitleScreen titleScreen(font);
    VictoryScreen victoryScreen(font);

    sf::RectangleShape menuBackground(sf::Vector2f(menuWidth, 2000));
    menuBackground.setPosition(0, 0);
    menuBackground.setFillColor(sf::Color(15, 15, 25, 250));
    
    sf::RectangleShape menuPattern(sf::Vector2f(menuWidth, 2000));
    menuPattern.setPosition(0, 0);
    menuPattern.setFillColor(sf::Color(30, 30, 45, 100));
    
    sf::RectangleShape menuBorder(sf::Vector2f(menuWidth - 4, 2000));
    menuBorder.setPosition(2, 0);
    menuBorder.setFillColor(sf::Color::Transparent);
    menuBorder.setOutlineThickness(2.f);
    menuBorder.setOutlineColor(sf::Color(100, 150, 255, 180));

    sf::Text titleText("ESCAPE THE GRID", font, 16);
    titleText.setFillColor(sf::Color(100, 200, 255));
    titleText.setStyle(sf::Text::Bold);
    sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setPosition((menuWidth - titleBounds.width) / 2.f, 15);
    
    sf::Text subtitleText("Find the maze exit", font, 8);
    subtitleText.setFillColor(sf::Color(180, 180, 220));
    sf::FloatRect subtitleBounds = subtitleText.getLocalBounds();
    subtitleText.setPosition((menuWidth - subtitleBounds.width) / 2.f, 50);
    
    sf::RectangleShape decorLine(sf::Vector2f(200, 2));
    decorLine.setPosition((menuWidth - 200) / 2.f, 75);
    decorLine.setFillColor(sf::Color(100, 150, 255, 150));

    vector<unique_ptr<Button>> buttons;
    buttons.push_back(make_unique<Button>(30, 110, 240, 45, "PLAY", font, sf::Color(50, 180, 50, 220)));
    buttons.push_back(make_unique<Button>(30, 165, 240, 45, "AUTO-SOLVE", font, sf::Color(180, 120, 50, 220)));
    buttons.push_back(make_unique<Button>(30, 220, 240, 45, "RESTART", font, sf::Color(180, 50, 50, 220)));

    float column1X = 30, column2X = 160, columnsStartY = 290;
    
    sf::RectangleShape infoSection(sf::Vector2f(240, 180));
    infoSection.setPosition(30, columnsStartY - 10);
    infoSection.setFillColor(sf::Color(20, 25, 35, 180));
    infoSection.setOutlineThickness(1.f);
    infoSection.setOutlineColor(sf::Color(80, 120, 180, 120));
    
    sf::Text infoTitle = createStyledText("INFORMATION", font, 12, sf::Color(150, 200, 255), column1X + 10, columnsStartY);
    infoTitle.setStyle(sf::Text::Bold);
    
    vector<sf::Text> infoTexts = {
        createStyledText("- Player (blue)", font, 8, sf::Color(100, 150, 255), column1X + 15, columnsStartY + 25),
        createStyledText("- Goal (green)", font, 8, sf::Color(100, 255, 100), column1X + 15, columnsStartY + 45),
        createStyledText("- Wall (gray)", font, 8, sf::Color(150, 150, 150), column1X + 15, columnsStartY + 65),
        createStyledText("- Crystal (cyan)", font, 8, sf::Color(0, 255, 255), column1X + 15, columnsStartY + 85)
    };

    sf::Text controlsTitle = createStyledText("CONTROLS", font, 12, sf::Color(255, 200, 150), column1X + 10, columnsStartY + 110);
    controlsTitle.setStyle(sf::Text::Bold);
    
    vector<sf::Text> controlTexts = {
        createStyledText("- Arrows: Move", font, 8, sf::Color(200, 200, 200), column1X + 15, columnsStartY + 135),
        createStyledText("- ENTER: Auto-solve", font, 8, sf::Color(200, 200, 200), column1X + 15, columnsStartY + 155)
    };

    sf::Text statusText("", font, 10);
    sf::Text movesText("Moves: 0", font, 10);
    sf::Text timeText("Time: 0:00", font, 10);

    sf::CircleShape player(cellSize/5), goal(cellSize/5);
    player.setFillColor(sf::Color(50, 150, 255));
    player.setOutlineThickness(2.f);
    player.setOutlineColor(sf::Color::White);
    goal.setFillColor(sf::Color(0, 255, 0, 255));
    goal.setOutlineThickness(3.f);
    goal.setOutlineColor(sf::Color::White);

    player.setPosition(startX * cellSize + cellSize/2 - cellSize/5, startY * cellSize + cellSize/2 - cellSize/5);
    verifyGoal(goal);

    bool autoMode = false;
    bool solved = false;
    size_t step = 0;
    sf::Vector2f currentPos = player.getPosition();
    sf::Clock gameClock, moveClock;
    int currentX = startX, currentY = startY;
    int moveCount = 0;
    
    lastX = startX;
    lastY = startY;

    grid[startY*W + startX].hasBeenTraversed = true;

    // Gestiona eventos de usuario, lógica de juego y renderizado de frames
    while (window.isOpen()) {
        float dt = moveClock.restart().asSeconds();
        sf::Event e;
        
        // Sistema de procesamiento de eventos del sistema operativo
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) 
                window.close();
            
            if (e.type == sf::Event::Resized) {
                if (gameState == GameState::TitleScreen) {
                    sf::FloatRect visibleArea(0, 0, e.size.width, e.size.height);
                    window.setView(sf::View(visibleArea));
                    titleScreen.onWindowResize(sf::Vector2u(e.size.width, e.size.height));
                }
                else if (gameState == GameState::Solved) {
                    // Handle victory screen resize similar to title screen
                    sf::FloatRect visibleArea(0, 0, e.size.width, e.size.height);
                    window.setView(sf::View(visibleArea));
                    victoryScreen.onWindowResize(sf::Vector2u(e.size.width, e.size.height));
                }
                else {
                    updateViews(window);
                }
            }
            
            if (gameState == GameState::TitleScreen) {
                if (e.type == sf::Event::MouseButtonPressed) {
                    sf::Vector2i mousePos(e.mouseButton.x, e.mouseButton.y);
                    if (titleScreen.isStartButtonClicked(mousePos.x, mousePos.y)) {
                        gameState = GameState::Menu;
                    }
                }
                else if (e.type == sf::Event::MouseMoved) {
                    titleScreen.updateButtonHover(e.mouseMove.x, e.mouseMove.y);
                }
                continue;
            }
                
            if (e.type == sf::Event::MouseButtonPressed) {
                sf::Vector2i mousePos(e.mouseButton.x, e.mouseButton.y);                                sf::Vector2f menuCoords = windowToMenuCoords(mousePos, window);
                                sf::Vector2f gameCoords = windowToGameCoords(mousePos, window);
                                
                                bool buttonClicked = false;
                                for (size_t i = 0; i < buttons.size(); i++) {
                                    if (buttons[i]->contains(menuCoords.x, menuCoords.y)) {
                                        buttonClicked = true;
                                        switch (i) {
                                            case 0:
                                                gameState = GameState::Playing;
                                autoMode = false;                                                solved = false;
                                                gameClock.restart();
                                                break;
                                            case 1:
                                                if (gameState == GameState::Menu || gameState == GameState::Playing) {
                                    gameState = GameState::Playing;
                                    solved = false;
                                    startY = currentY;
                                    startX = currentX;
                                    bfsSolve();
                                    autoMode = true;
                                    step = 0;
                                    currentPos = player.getPosition();
                                    moveCount = 0;                                                    gameClock.restart();
                                                }
                                                break;
                                            case 2:
                                                resetGame(goal);
                                currentX = startX;
                                currentY = startY;
                                player.setPosition(
                                    startX * cellSize + cellSize/2 - cellSize/5,
                                    startY * cellSize + cellSize/2 - cellSize/5
                                );
                                currentPos = player.getPosition();
                                moveCount = 0;
                                solved = false;
                                autoMode = false;
                                gameState = GameState::Menu;
                                break;
                        }
                        break;
                    }
                }
                
                if (e.mouseButton.button == sf::Mouse::Left && !autoMode && !solved && gameState == GameState::Playing) {
                    if (gameCoords.x >= 0 && gameCoords.y >= 0) {
                        int clickX = gameCoords.x / cellSize;
                        int clickY = gameCoords.y / cellSize;
                        
                        if (clickX >= 0 && clickX < W && clickY >= 0 && clickY < H) {
                            int dx = abs(clickX - currentX);
                            int dy = abs(clickY - currentY);
                            
                            if ((dx == 1 && dy == 0) || (dx == 0 && dy == 1)) {
                                if (tryMovePlayer(clickX, clickY, currentX, currentY, player, goal)) {
                                    moveCount++;
                                    if (currentX == goalX && currentY == goalY) {
                                        solved = true;
                                        gameState = GameState::Solved;
                                        victoryScreen.setGameStats(moveCount, gameClock.getElapsedTime().asSeconds());
                                    }
                                }
                            }
                        }
                    }
                }
                
                if (e.mouseButton.button == sf::Mouse::Left && gameState == GameState::Solved) {
                    sf::Vector2i mousePos(e.mouseButton.x, e.mouseButton.y);
                    if (victoryScreen.isPlayAgainButtonClicked(mousePos.x, mousePos.y)) {
                        // Reset the game completely
                        resetGame(goal);
                        currentX = startX;
                        currentY = startY;
                        player.setPosition(
                            startX * cellSize + cellSize/2 - cellSize/5,
                            startY * cellSize + cellSize/2 - cellSize/5
                        );
                        currentPos = player.getPosition();
                        moveCount = 0;
                        solved = false;
                        autoMode = false;
                        gameState = GameState::Menu;
                        gameClock.restart();
                    }
                }
            }
            
            if (e.type == sf::Event::MouseMoved && gameState != GameState::TitleScreen) {
                sf::Vector2i mousePos(e.mouseMove.x, e.mouseMove.y);
                
                if (gameState == GameState::Solved) {
                    // Handle victory screen mouse hover directly
                    victoryScreen.updateButtonHover(mousePos.x, mousePos.y);
                } else {
                    // Handle menu buttons for other states
                    sf::Vector2f menuCoords = windowToMenuCoords(mousePos, window);
                    for (auto& button : buttons) {
                        button->updateHover(menuCoords.x, menuCoords.y);
                    }
                }
            }
                
            if (e.type == sf::Event::KeyPressed) {
                if (!autoMode && !solved && gameState == GameState::Playing) {
                    bool moved = false;
                    
                    if (e.key.code == sf::Keyboard::Up) {
                        moved = tryMovePlayer(currentX, currentY - 1, currentX, currentY, player, goal);
                    }
                    else if (e.key.code == sf::Keyboard::Down) {
                        moved = tryMovePlayer(currentX, currentY + 1, currentX, currentY, player, goal);
                    }
                    else if (e.key.code == sf::Keyboard::Left) {
                        moved = tryMovePlayer(currentX - 1, currentY, currentX, currentY, player, goal);
                    }
                    else if (e.key.code == sf::Keyboard::Right) {
                        moved = tryMovePlayer(currentX + 1, currentY, currentX, currentY, player, goal);
                    }
                    
                    if (moved) {
                        moveCount++;
                        if (currentX == goalX && currentY == goalY) {
                            solved = true;
                            gameState = GameState::Solved;
                            victoryScreen.setGameStats(moveCount, gameClock.getElapsedTime().asSeconds());
                        }
                    }
                }
                
                if (e.key.code == sf::Keyboard::Enter && !autoMode && gameState == GameState::Playing) {
                    startY = currentY;
                    startX = currentX;
                    bfsSolve();
                    autoMode = true;
                    step = 0;
                    currentPos = player.getPosition();
                }
                
                if (e.key.code == sf::Keyboard::R) {
                    resetGame(goal);
                    currentX = startX;
                    currentY = startY;
                    player.setPosition(
                        startX * cellSize + cellSize/2 - cellSize/5,
                        startY * cellSize + cellSize/2 - cellSize/5
                    );
                    currentPos = player.getPosition();
                    moveCount = 0;
                    solved = false;
                    autoMode = false;
                    gameState = GameState::Menu;
                }
                
                // Debug key to test victory screen - press 'V' to win instantly
                if (e.key.code == sf::Keyboard::V && gameState == GameState::Playing) {
                    solved = true;
                    gameState = GameState::Solved;
                    victoryScreen.setGameStats(moveCount, gameClock.getElapsedTime().asSeconds());
                }
            }
        }

        // Gestión especializada para el estado de pantalla de título
        if (gameState == GameState::TitleScreen) {
            titleScreen.update(window.getSize());
            window.clear();
            titleScreen.draw(window);
            window.display();
            continue;
        }

        // Sistema de resolución automática mediante inteligencia artificial
        if (autoMode && step < path.size()) {
            auto [y, x] = path[step];
            sf::Vector2f nextPos(
                x * cellSize + cellSize/2 - cellSize/5,
                y * cellSize + cellSize/2 - cellSize/5
            );

            sf::Vector2f direction = nextPos - currentPos;
            float distance = sqrt(direction.x*direction.x + direction.y*direction.y);
            
            // Algoritmo de optimización dinámica para suavizado de animaciones
            vector<int> framePrices = {1, 3, 4, 5};
            int totalFrames = max(1, (int)(distance / 2.0f));
            
            vector<int> dp(totalFrames + 1, 0);
            for (int i = 1; i <= totalFrames; i++) {
                for (int j = 1; j <= min(i, (int)framePrices.size()); j++) {
                    dp[i] = max(dp[i], framePrices[j-1] + dp[i-j]);
                }
            }
            
            float optimalSpeed = 2.0f + (dp[totalFrames] * 0.1f);
            float animationSpeed = min(optimalSpeed, 6.0f);

            if (distance > 1.f) {
                direction /= distance;
                currentPos += direction * animationSpeed * dt * cellSize;
                player.setPosition(currentPos);
            } else {
                currentPos = nextPos;
                player.setPosition(currentPos);
                currentX = x;
                currentY = y;

                grid[y*W + x].hasBeenTraversed = true;
                
                if (grid[y*W + x].type == CellType::Crystal) {
                    reflectCrystals(currentX, currentY);
                } else {
                    lastX = currentX;
                    lastY = currentY;
                }

                turnCount++;
                turnsSinceLastGoalMove++;
                
                if (turnsSinceLastGoalMove >= TURNS_TO_MOVE_GOAL) {
                    moveGoal(currentX, currentY, goal);
                }
                
                if (turnCount % TURNS_PER_EVENT == 0) {
                    triggerMapEvent();
                    startY = currentY;
                    startX = currentX;

                    int savedGoalX = goalX;
                    int savedGoalY = goalY;

                    bfsSolve();

                    goalX = savedGoalX;
                    goalY = savedGoalY;
                    grid[goalY*W + goalX].type = CellType::Goal;
                    verifyGoal(goal);

                    step = 0;
                    if (path.empty()) {                    autoMode = false;
                    break;
                }
                continue;
            }

                step++;
                moveCount++;

                if (currentX == goalX && currentY == goalY) {
                    solved = true;
                    gameState = GameState::Solved;
                    victoryScreen.setGameStats(moveCount, gameClock.getElapsedTime().asSeconds());
                    autoMode = false;
                }
            }
            
            if (step % 5 == 0) {
                verifyGoal(goal);
            }
        }

        float elapsedTime = gameClock.getElapsedTime().asSeconds();
        int minutes = (int)elapsedTime / 60;
        int seconds = (int)elapsedTime % 60;
        
        sf::Vector2u windowSize = window.getSize();
        
        movesText.setString("Moves: " + to_string(moveCount));
        timeText.setString("Time: " + to_string(minutes) + ":" + (seconds < 10 ? "0" : "") + to_string(seconds));
        
        switch (gameState) {
            case GameState::Menu:
                statusText.setString("Welcome!");
                statusText.setFillColor(sf::Color(150, 255, 150));
                break;
            case GameState::Playing:
                statusText.setString(autoMode ? "Auto-solving..." : "Playing!");
                statusText.setFillColor(autoMode ? sf::Color(255, 255, 150) : sf::Color(150, 150, 255));
                break;
            case GameState::Solved:
                statusText.setString("Completed!");
                statusText.setFillColor(sf::Color(150, 255, 150));
                break;
            default: break;
        }
        
        sf::FloatRect statusBounds = statusText.getLocalBounds();
        statusText.setPosition((menuWidth - statusBounds.width) / 2.f, windowSize.y - 60);
        
        sf::FloatRect movesBounds = movesText.getLocalBounds();
        movesText.setPosition((menuWidth - movesBounds.width) / 2.f, windowSize.y - 40);
        
        sf::FloatRect timeBounds = timeText.getLocalBounds();
        timeText.setPosition((menuWidth - timeBounds.width) / 2.f, windowSize.y - 20);

        
        // Update victory screen if in solved state
        if (gameState == GameState::Solved) {
            victoryScreen.update(windowSize);
        }
        
        window.clear(sf::Color(15, 15, 25));
        
        window.setView(gameView);

        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                Cell& cell = grid[y*W + x];
                sf::ConvexShape tri = makeTri(x, y);
                tri.setFillColor(getCellColor(cell.type, x, y, cell.visited, cell.isOnPath, cell.hasBeenTraversed));
                window.draw(tri);
                
                if (cell.type == CellType::Crystal) {
                    sf::ConvexShape crystalTri = makeTri(x, y);
                    crystalTri.setFillColor(sf::Color(255, 255, 255, 50));
                    window.draw(crystalTri);
                }
            }
        }

        verifyGoal(goal);
        window.draw(goal);
        window.draw(player);

        window.setView(menuView);
        
        window.draw(menuBackground);
        window.draw(menuPattern);
        window.draw(menuBorder);
        window.draw(titleText);
        window.draw(subtitleText);
        window.draw(decorLine);
        
        for (auto& button : buttons) {
            button->draw(window);
        }
        
        window.draw(infoSection);
        window.draw(infoTitle);
        for (auto& text : infoTexts) window.draw(text);
        
        window.draw(controlsTitle);
        for (auto& text : controlTexts) window.draw(text);
        
        window.draw(statusText);
        window.draw(movesText);
        window.draw(timeText);
        
        // Draw victory screen on top if game is solved
        if (gameState == GameState::Solved) {
            // Use a full window view for the victory screen, similar to title screen
            sf::View fullWindowView(sf::FloatRect(0, 0, windowSize.x, windowSize.y));
            window.setView(fullWindowView);
            victoryScreen.draw(window);
        }
        
        window.display();
    }

    return 0;
}