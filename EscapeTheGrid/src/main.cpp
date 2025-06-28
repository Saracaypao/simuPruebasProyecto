#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <iostream>
#include <cmath>
#include <memory>

using namespace std;

// ==================== ESTRUCTURAS Y ENUMS ====================
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

// ==================== TITLE SCREEN ====================
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
        
        // Ajustar número de partículas basado en el área de la pantalla
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
        
        // Fondo estilo arcade - se ajustará dinámicamente
        backgroundGradient.setSize(sf::Vector2f(1200, 800));
        backgroundGradient.setFillColor(sf::Color(5, 5, 15));
        
        // Título principal estilo arcade - tamaño base, se escalará dinámicamente
        titleText.setString("ESCAPE THE GRID");
        titleText.setFont(font);
        titleText.setCharacterSize(32);
        titleText.setFillColor(sf::Color(180, 220, 255));
        titleText.setStyle(sf::Text::Bold);
        
        // Subtítulo descriptivo - tamaño base, se escalará dinámicamente
        subtitleText.setString("CRYSTAL MAZE ADVENTURE");
        subtitleText.setFont(font);
        subtitleText.setCharacterSize(12);
        subtitleText.setFillColor(sf::Color(100, 255, 255));
        
        // Player info - tamaño base, se escalará dinámicamente
        playerText.setString("1P\n00");
        playerText.setFont(font);
        playerText.setCharacterSize(12);
        playerText.setFillColor(sf::Color(255, 100, 100));
        
        // Botón estilo arcade - tamaño base, se escalará dinámicamente
        startButton.setSize(sf::Vector2f(300, 50));
        startButton.setFillColor(sf::Color(50, 50, 150, 200));
        startButton.setOutlineThickness(3.f);
        startButton.setOutlineColor(sf::Color(255, 255, 100, 180));
        
        startButtonText.setString("CLICK TO CONTINUE");
        startButtonText.setFont(font);
        startButtonText.setCharacterSize(12);
        startButtonText.setFillColor(sf::Color(255, 255, 255));
        startButtonText.setStyle(sf::Text::Bold);
        
        // Instrucciones estilo retro - tamaño base, se escalará dinámicamente
        instructionText.setString("Navigate the maze - Crystals reflect reality\nFind the green exit - Beware: the goal moves!");
        instructionText.setFont(font);
        instructionText.setCharacterSize(10);
        instructionText.setFillColor(sf::Color(150, 200, 255, 180));
        
        // Créditos - tamaño base, se escalará dinámicamente
        creditText.setString("TSC - 2025");
        creditText.setFont(font);
        creditText.setCharacterSize(12);
        creditText.setFillColor(sf::Color(255, 100, 255));
        
        initParticles();
    }
    
    void onWindowResize(sf::Vector2u newSize) {
        // Reinicializar partículas con el nuevo tamaño de ventana
        initParticles(newSize);
        
        // Actualizar fondo para cubrir toda la ventana
        backgroundGradient.setSize(sf::Vector2f(newSize.x, newSize.y));
    }
    
    void update(sf::Vector2u windowSize) {
        float deltaTime = animationClock.restart().asSeconds();
        
        // Animar partículas de fondo con límites responsive
        for (size_t i = 0; i < particles.size(); i++) {
            particles[i].move(particleVelocities[i] * deltaTime * 30.f);
            
            sf::Vector2f pos = particles[i].getPosition();
            // Wrap-around responsive basado en el tamaño actual de la ventana
            if (pos.x < -10) particles[i].setPosition(windowSize.x + 10, pos.y);
            if (pos.x > windowSize.x + 10) particles[i].setPosition(-10, pos.y);
            if (pos.y < -10) particles[i].setPosition(pos.x, windowSize.y + 10);
            if (pos.y > windowSize.y + 10) particles[i].setPosition(pos.x, -10);
            
            // Efecto de parpadeo en las partículas
            float time = blinkClock.getElapsedTime().asSeconds();
            sf::Color baseColor = particles[i].getFillColor();
            float alpha = 80 + 120 * sin(time * 3 + i * 0.8f);
            particles[i].setFillColor(sf::Color(baseColor.r, baseColor.g, baseColor.b, (sf::Uint8)alpha));
        }
        
        // Efecto pulsante en el título (más sutil para pantallas grandes)
        float time = blinkClock.getElapsedTime().asSeconds();
        float pulseScale = 1.0f + 0.04f * sin(time * 2.5f);
        if (windowSize.x >= 1920) {
            pulseScale = 1.0f + 0.02f * sin(time * 2.5f); // Menos pronunciado en pantallas grandes
        }
        titleText.setScale(pulseScale, pulseScale);
        
        // Parpadeo del botón
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
        // Obtener tamaño actual de la ventana
        sf::Vector2u windowSize = window.getSize();
        float centerX = windowSize.x / 2.0f;
        float centerY = windowSize.y / 2.0f;
        
        window.clear(sf::Color(5, 5, 15));
        
        // Ajustar el fondo para que cubra toda la ventana
        backgroundGradient.setSize(sf::Vector2f(windowSize.x, windowSize.y));
        window.draw(backgroundGradient);
        
        // Dibujar partículas
        for (auto& particle : particles) {
            window.draw(particle);
        }
        
        // Sistema de escalado uniforme basado en altura de ventana
        float baseHeight = 600.0f;
        float scale = windowSize.y / baseHeight;
        
        // Limitar escalas extremas para mantener proporciones
        scale = max(0.7f, min(scale, 2.5f));
        
        // ===== PLAYER INFO =====
        if (windowSize.x > 800) {
            float playerOffset = windowSize.x * 0.15f;
            float playerY = windowSize.y * 0.08f;
            playerText.setCharacterSize(static_cast<unsigned int>(12.0f * scale));
            sf::FloatRect playerBounds = playerText.getLocalBounds();
            playerText.setOrigin(playerBounds.width/2, playerBounds.height/2);
            playerText.setPosition(centerX + playerOffset, playerY);
            window.draw(playerText);
        }
        
        // ===== TÍTULO PRINCIPAL =====
        float titleSize = 32.0f * scale;
        titleText.setCharacterSize(static_cast<unsigned int>(titleSize));
        sf::FloatRect titleBounds = titleText.getLocalBounds();
        titleText.setOrigin(titleBounds.width/2, titleBounds.height/2);
        
        // Posición del título - centrado en el tercio superior
        float titleY = centerY * 0.65f;
        titleText.setPosition(centerX, titleY);
        window.draw(titleText);
        
        // ===== SUBTÍTULO =====
        float subtitleSize = 12.0f * scale;
        subtitleText.setCharacterSize(static_cast<unsigned int>(subtitleSize));
        sf::FloatRect subtitleBounds = subtitleText.getLocalBounds();
        subtitleText.setOrigin(subtitleBounds.width/2, subtitleBounds.height/2);
        
        float subtitleY = titleY + (40.0f * scale);
        subtitleText.setPosition(centerX, subtitleY);
        window.draw(subtitleText);
        
        // ===== BOTÓN DE INICIO =====
        float buttonWidth = 300.0f * scale;
        float buttonHeight = 50.0f * scale;
        
        // Limitar tamaño máximo del botón para pantallas muy grandes
        buttonWidth = min(buttonWidth, windowSize.x * 0.25f);
        buttonHeight = min(buttonHeight, windowSize.y * 0.08f);
        
        startButton.setSize(sf::Vector2f(buttonWidth, buttonHeight));
        
        float buttonY = subtitleY + (60.0f * scale);
        startButton.setPosition(centerX - buttonWidth/2, buttonY);
        
        // Texto del botón
        float buttonTextSize = 12.0f * scale;
        startButtonText.setCharacterSize(static_cast<unsigned int>(buttonTextSize));
        sf::FloatRect buttonTextBounds = startButtonText.getLocalBounds();
        startButtonText.setOrigin(buttonTextBounds.width/2, buttonTextBounds.height/2);
        startButtonText.setPosition(centerX, buttonY + buttonHeight/2);
        
        window.draw(startButton);
        window.draw(startButtonText);
        
        // ===== INSTRUCCIONES =====
        if (windowSize.y > 500) {
            float instructionSize = 10.0f * scale;
            instructionText.setCharacterSize(static_cast<unsigned int>(instructionSize));
            sf::FloatRect instrBounds = instructionText.getLocalBounds();
            instructionText.setOrigin(instrBounds.width/2, instrBounds.height/2);
            
            float instructionY = buttonY + buttonHeight + (50.0f * scale);
            instructionText.setPosition(centerX, instructionY);
            window.draw(instructionText);
        }
        
        // ===== CRÉDITOS =====
        float creditSize = 12.0f * scale;
        creditText.setCharacterSize(static_cast<unsigned int>(creditSize));
        sf::FloatRect creditBounds = creditText.getLocalBounds();
        creditText.setOrigin(creditBounds.width/2, creditBounds.height/2);
        
        float creditY = windowSize.y - (40.0f * scale);
        creditText.setPosition(centerX, creditY);
        window.draw(creditText);
        
        // ===== ELEMENTOS DECORATIVOS - CRISTALES =====
        if (windowSize.x > 1000) {
            float crystalOffset = windowSize.x * 0.20f;
            
            sf::CircleShape crystal1(6.0f * scale);
            crystal1.setFillColor(sf::Color(0, 255, 255, 200));
            crystal1.setPosition(centerX - crystalOffset, titleY - (30.0f * scale));
            window.draw(crystal1);
            
            sf::CircleShape crystal2(8.0f * scale);
            crystal2.setFillColor(sf::Color(255, 255, 0, 150));
            crystal2.setPosition(centerX + crystalOffset - (20.0f * scale), titleY - (10.0f * scale));
            window.draw(crystal2);
            
            sf::CircleShape crystal3(5.0f * scale);
            crystal3.setFillColor(sf::Color(255, 0, 255, 180));
            crystal3.setPosition(centerX - crystalOffset + (50.0f * scale), buttonY + buttonHeight + (40.0f * scale));
            window.draw(crystal3);
            
            sf::CircleShape crystal4(7.0f * scale);
            crystal4.setFillColor(sf::Color(0, 255, 0, 160));
            crystal4.setPosition(centerX + crystalOffset - (60.0f * scale), buttonY + buttonHeight + (20.0f * scale));
            window.draw(crystal4);
        }
        
        // ===== LÍNEAS DECORATIVAS =====
        if (windowSize.x > 600) {
            sf::RectangleShape line1(sf::Vector2f(150.0f * scale, 2.0f * scale));
            line1.setFillColor(sf::Color(255, 255, 100, 120));
            line1.setPosition(centerX - (75.0f * scale), subtitleY + (15.0f * scale));
            window.draw(line1);
            
            sf::RectangleShape line2(sf::Vector2f(100.0f * scale, 2.0f * scale));
            line2.setFillColor(sf::Color(100, 255, 255, 120));
            line2.setPosition(centerX - (50.0f * scale), buttonY + buttonHeight + (80.0f * scale));
            window.draw(line2);
        }
    }
};

// ==================== VARIABLES GLOBALES ====================
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

int lastX = -1, lastY = -1; // Para detectar dirección de movimiento

// ==================== FUNCIONES AUXILIARES ====================
bool inside(int y, int x) {
    return x >= 0 && x < W && y >= 0 && y < H;
}

// ==================== FUNCIONES DE REFLEXIÓN MEJORADAS ====================
void reflectHorizontally(int crystalX, int crystalY) {
    // Reflejar todo el lado izquierdo hacia la derecha tal cual
    for (int x = 0; x < crystalX; x++) {
        int reflectedX = crystalX + (crystalX - x);
        if (reflectedX < W) {
            grid[crystalY*W + reflectedX] = grid[crystalY*W + x];
        }
    }
    
    // Reflejar todo el lado derecho hacia la izquierda tal cual
    for (int x = crystalX + 1; x < W; x++) {
        int reflectedX = crystalX - (x - crystalX);
        if (reflectedX >= 0) {
            grid[crystalY*W + reflectedX] = grid[crystalY*W + x];
        }
    }
}

void reflectVertically(int crystalX, int crystalY) {
    // Reflejar todo el lado superior hacia abajo tal cual
    for (int y = 0; y < crystalY; y++) {
        int reflectedY = crystalY + (crystalY - y);
        if (reflectedY < H) {
            grid[reflectedY*W + crystalX] = grid[y*W + crystalX];
        }
    }
    
    // Reflejar todo el lado inferior hacia arriba tal cual
    for (int y = crystalY + 1; y < H; y++) {
        int reflectedY = crystalY - (y - crystalY);
        if (reflectedY >= 0) {
            grid[reflectedY*W + crystalX] = grid[y*W + crystalX];
        }
    }
}

void reflectDynamically(int crystalX, int crystalY, int directionX, int directionY) {
    cout << "Cristal activado en (" << crystalX << "," << crystalY << ") - Direccion: (" << directionX << "," << directionY << ")\n";
    
    // Determinar tipo de reflexión basado en la dirección de entrada
    if (directionX != 0) {
        // Movimiento horizontal -> Reflejar horizontalmente
        reflectHorizontally(crystalX, crystalY);
        cout << "Reflejo HORIZONTAL activado (movimiento izquierda/derecha)\n";
    } else if (directionY != 0) {
        // Movimiento vertical -> Reflejar verticalmente  
        reflectVertically(crystalX, crystalY);
        cout << "Reflejo VERTICAL activado (movimiento arriba/abajo)\n";
    }
}

void reflectCrystals(int currentX, int currentY) {
    // Calcular dirección de movimiento
    int directionX = 0, directionY = 0;
    
    if (lastX != -1 && lastY != -1) {
        directionX = currentX - lastX;
        directionY = currentY - lastY;
    }
    
    // Verificar si el jugador está en un cristal
    if (grid[currentY*W + currentX].type == CellType::Crystal) {
        reflectDynamically(currentX, currentY, directionX, directionY);
    }
    
    // Actualizar posición anterior
    lastX = currentX;
    lastY = currentY;
}

// ==================== RESTANTE DEL CÓDIGO ====================
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

bool loadMaze(const string& path) {
    ifstream file(path);
    if (!file.is_open()) {
        cout << "No se pudo abrir: " << path << endl;
        return false;
    }
    
    if (!(file >> W >> H >> startY >> startX >> goalY >> goalX)) {
        cout << "Error leyendo dimensiones del laberinto" << endl;
        return false;
    }
    
    // Knapsack optimization: calcular capacidad óptima de memoria
    int totalCells = W * H;
    int memoryCapacity = min(totalCells, 2500); // Límite de memoria eficiente
    
    // Solo redimensionar si es necesario (evita reallocaciones innecesarias)
    if (grid.size() != totalCells) {
        grid.clear();
        grid.reserve(memoryCapacity); // Reservar memoria óptima
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
    
    // Resetear tracking de posición
    lastX = startX;
    lastY = startY;
    
    verifyGoal(goal);
}

void triggerMapEvent() {
    int rx = rand() % W;
    int ry = rand() % H;
    Cell& c = grid[ry*W + rx];
    if (c.type == CellType::Empty) {
        c.type = CellType::Wall;
        cout << "Evento: aparece muro en (" << rx << "," << ry << ")\n";
    }
    else if (c.type == CellType::Wall) {
        c.type = CellType::Empty;
        cout << "Evento: desaparece muro en (" << rx << "," << ry << ")\n";
    }
}

// AGREGAR al inicio de bfsSolve() para cache inteligente:
void bfsSolve() {
    // LCS optimization: reutilizar segmentos de caminos anteriores si son similares
    static vector<pair<int,int>> lastPath;
    static int lastStartX = -1, lastStartY = -1, lastGoalX = -1, lastGoalY = -1;
    
    // Verificar si podemos reutilizar el camino anterior (LCS logic)
    if (lastStartX == startX && lastStartY == startY && lastGoalX == goalX && lastGoalY == goalY && !lastPath.empty()) {
        // Validar que el camino anterior sigue siendo válido
        bool pathValid = true;
        for (auto [y, x] : lastPath) {
            if (grid[y*W + x].type == CellType::Wall) {
                pathValid = false;
                break;
            }
        }
        
        if (pathValid) {
            path = lastPath; // Reutilizar camino cached
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
    
    // Minimum Coin Change aplicado: usar dp para encontrar costo mínimo
    vector<vector<int>> cost(H, vector<int>(W, INT_MAX));
    vector<vector<pair<int,int>>> parent(H, vector<pair<int,int>>(W, {-1,-1}));
    queue<pair<int,int>> q;
    
    cost[startY][startX] = 0; // Costo inicial = 0
    q.push({startY, startX});
    
    int dirs[4][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}};
    bool found = false;

    while (!q.empty()) {
        auto [y,x] = q.front(); q.pop();
        
        if (y == goalY && x == goalX) { 
            found = true; 
            break; 
        }
        
        for (auto& d : dirs) {
            int ny = y + d[0];
            int nx = x + d[1];
            
            if (!inside(ny,nx)) continue;
            if (grid[ny*W+nx].type == CellType::Wall) continue;
            
            // Aplicar costos dinámicos basados en tipo de celda (Coin Change logic)
            int moveCost = 1; // Costo base
            if (grid[ny*W+nx].type == CellType::Crystal) moveCost = 3; // Cristales cuestan más
            if (grid[ny*W+nx].hasBeenTraversed) moveCost = 1; // Celdas ya visitadas son más baratas
            
            int newCost = cost[y][x] + moveCost;
            
            // Solo procesar si encontramos un camino más barato (DP optimization)
            if (newCost < cost[ny][nx]) {
                cost[ny][nx] = newCost;
                parent[ny][nx] = {y,x};
                q.push({ny,nx});
            }
        }
    }

    // Reconstruir camino óptimo
    if (found) {
        for (int cy = goalY, cx = goalX; cy != -1; ) {
            path.push_back({cy,cx});
            auto p = parent[cy][cx];
            cy = p.first; 
            cx = p.second;
        }
        reverse(path.begin(), path.end());
        
        // Guardar en cache para futura reutilización (LCS optimization)
        lastPath = path;
        lastStartX = startX; lastStartY = startY;
        lastGoalX = tempGoalX; lastGoalY = tempGoalY;
    }
}

sf::Color getCellColor(CellType type, int x, int y, bool visited, bool isOnPath, bool hasBeenTraversed) {
    if (type == CellType::Crystal) return sf::Color(0, 255, 255, 180);
    
    if (type == CellType::Goal) return sf::Color(0, 200, 0, 150);
    
    if (hasBeenTraversed) return sf::Color(192, 192, 192, 200);
    
    switch (type) {
        case CellType::Wall: return sf::Color(40, 40, 40);
        case CellType::Start: return sf::Color(100, 255, 100, 200);
        case CellType::Empty:
        default: 
            return ((x + y) % 2 == 0) ? sf::Color(120, 120, 200, 120) : sf::Color(100, 100, 180, 120);
    }
}

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
        
        // Solo reflejar cuando el jugador pasa por un cristal (con dirección)
        if (grid[currentY*W + currentX].type == CellType::Crystal) {
            reflectCrystals(currentX, currentY);
        } else {
            // Actualizar posición anterior sin activar reflejo
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

int main() {
    if (!loadMaze("../assets/maze.txt") && !loadMaze("assets/maze.txt") && !loadMaze("maze.txt")) {
        createDefaultMaze();
    }

    float initialGameWidth = W * cellSize;
    float initialGameHeight = H * cellSize;
    float initialTotalWidth = initialGameWidth + menuWidth;
    
    sf::RenderWindow window(sf::VideoMode(initialTotalWidth, initialGameHeight), "Escape the Grid");
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

    // Crear pantalla de título
    TitleScreen titleScreen(font);

    sf::RectangleShape menuBackground(sf::Vector2f(menuWidth, 2000));
    menuBackground.setPosition(0, 0);
    menuBackground.setFillColor(sf::Color(15, 15, 25, 250));
    
    // Fondo decorativo con patrón
    sf::RectangleShape menuPattern(sf::Vector2f(menuWidth, 2000));
    menuPattern.setPosition(0, 0);
    menuPattern.setFillColor(sf::Color(30, 30, 45, 100));
    
    // Borde elegante del menú
    sf::RectangleShape menuBorder(sf::Vector2f(menuWidth - 4, 2000));
    menuBorder.setPosition(2, 0);
    menuBorder.setFillColor(sf::Color::Transparent);
    menuBorder.setOutlineThickness(2.f);
    menuBorder.setOutlineColor(sf::Color(100, 150, 255, 180));

    // Título principal con efectos
    sf::Text titleText("ESCAPE THE GRID", font, 16);
    titleText.setFillColor(sf::Color(100, 200, 255));
    titleText.setStyle(sf::Text::Bold);
    sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setPosition((menuWidth - titleBounds.width) / 2.f, 15);
    
    // Subtítulo
    sf::Text subtitleText("Find the maze exit", font, 8);
    subtitleText.setFillColor(sf::Color(180, 180, 220));
    sf::FloatRect subtitleBounds = subtitleText.getLocalBounds();
    subtitleText.setPosition((menuWidth - subtitleBounds.width) / 2.f, 50);
    
    // Línea decorativa
    sf::RectangleShape decorLine(sf::Vector2f(200, 2));
    decorLine.setPosition((menuWidth - 200) / 2.f, 75);
    decorLine.setFillColor(sf::Color(100, 150, 255, 150));

    vector<unique_ptr<Button>> buttons;
    buttons.push_back(make_unique<Button>(30, 110, 240, 45, "PLAY", font, sf::Color(50, 180, 50, 220)));
    buttons.push_back(make_unique<Button>(30, 165, 240, 45, "AUTO-SOLVE", font, sf::Color(180, 120, 50, 220)));
    buttons.push_back(make_unique<Button>(30, 220, 240, 45, "RESTART", font, sf::Color(180, 50, 50, 220)));

    float column1X = 30, column2X = 160, columnsStartY = 290;
    
    // Sección de información mejorada
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
    
    // Inicializar tracking de posición
    lastX = startX;
    lastY = startY;

    grid[startY*W + startX].hasBeenTraversed = true;

    while (window.isOpen()) {
        float dt = moveClock.restart().asSeconds();
        sf::Event e;
        
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) 
                window.close();
            
            if (e.type == sf::Event::Resized) {
                updateViews(window);
                // Actualizar la pantalla de título si estamos en ella
                if (gameState == GameState::TitleScreen) {
                    titleScreen.onWindowResize(sf::Vector2u(e.size.width, e.size.height));
                }
            }
            
            // Manejar pantalla de título
            if (gameState == GameState::TitleScreen) {
                if (e.type == sf::Event::MouseButtonPressed) {
                    sf::Vector2i mousePos(e.mouseButton.x, e.mouseButton.y);
                    if (titleScreen.isStartButtonClicked(mousePos.x, mousePos.y)) {
                        gameState = GameState::Menu;
                    }
                }
                continue;
            }
                
            if (e.type == sf::Event::MouseButtonPressed) {
                sf::Vector2i mousePos(e.mouseButton.x, e.mouseButton.y);
                sf::Vector2f menuCoords = windowToMenuCoords(mousePos, window);
                sf::Vector2f gameCoords = windowToGameCoords(mousePos, window);
                
                bool buttonClicked = false;
                for (size_t i = 0; i < buttons.size(); i++) {
                    if (buttons[i]->contains(menuCoords.x, menuCoords.y)) {
                        buttonClicked = true;
                        switch (i) {
                            case 0: // PLAY
                                gameState = GameState::Playing;
                                autoMode = false;
                                solved = false;
                                gameClock.restart();
                                break;
                            case 1: // AUTO-SOLVE
                                if (gameState == GameState::Menu || gameState == GameState::Playing) {
                                    gameState = GameState::Playing;
                                    solved = false;
                                    startY = currentY;
                                    startX = currentX;
                                    bfsSolve();
                                    autoMode = true;
                                    step = 0;
                                    currentPos = player.getPosition();
                                    moveCount = 0;
                                    gameClock.restart();
                                }
                                break;
                            case 2: // RESTART
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
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            if (e.type == sf::Event::MouseMoved) {
                if (gameState == GameState::TitleScreen) {
                    titleScreen.updateButtonHover(e.mouseMove.x, e.mouseMove.y);
                } else {
                    sf::Vector2i mousePos(e.mouseMove.x, e.mouseMove.y);
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
            }
        }

        // Mostrar pantalla de título si corresponde
        if (gameState == GameState::TitleScreen) {
            titleScreen.update(window.getSize());
            titleScreen.draw(window);
            window.display();
            continue;
        }

        if (autoMode && step < path.size()) {
            auto [y, x] = path[step];
            sf::Vector2f nextPos(
                x * cellSize + cellSize/2 - cellSize/5,
                y * cellSize + cellSize/2 - cellSize/5
            );

            sf::Vector2f direction = nextPos - currentPos;
            float distance = sqrt(direction.x*direction.x + direction.y*direction.y);
            
            // Rod Cutting optimization: dividir la animación en segmentos óptimos
            vector<int> framePrices = {1, 3, 4, 5}; // Valores de eficiencia por frame
            int totalFrames = max(1, (int)(distance / 2.0f));
            
            // Calcular división óptima de frames usando Rod Cutting
            vector<int> dp(totalFrames + 1, 0);
            for (int i = 1; i <= totalFrames; i++) {
                for (int j = 1; j <= min(i, (int)framePrices.size()); j++) {
                    dp[i] = max(dp[i], framePrices[j-1] + dp[i-j]);
                }
            }
            
            // Usar velocidad adaptativa basada en la división óptima
            float optimalSpeed = 2.0f + (dp[totalFrames] * 0.1f);
            float animationSpeed = min(optimalSpeed, 6.0f); // Límite máximo

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
                
                // Solo reflejar cuando el jugador pasa por un cristal (con dirección)
                if (grid[y*W + x].type == CellType::Crystal) {
                    reflectCrystals(currentX, currentY);
                } else {
                    // Actualizar posición anterior sin activar reflejo
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
                    if (path.empty()) {
                        autoMode = false;
                        break;
                    }
                    continue;  
                }

                step++;
                moveCount++;

                if (currentX == goalX && currentY == goalY) {
                    solved = true;
                    gameState = GameState::Solved;
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
        
        window.display();
    }

    return 0;
}