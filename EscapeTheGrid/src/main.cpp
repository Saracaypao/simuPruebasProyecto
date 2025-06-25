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
enum class GameState { Menu, Playing, Solved };

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
        
        text = sf::Text(label, font, 16);
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

// ==================== VARIABLES GLOBALES ====================
int turnCount = 0;
const int TURNS_PER_EVENT = 5;
const int TURNS_TO_MOVE_GOAL = 10;
int turnsSinceLastGoalMove = 0;
int W, H;
vector<Cell> grid;
int startX, startY, goalX, goalY;
vector<pair<int,int>> path;
GameState gameState = GameState::Menu;

float cellSize = 35.f;
float menuWidth = 300.f;
sf::Vector2f gameOffset(0, 0);
sf::View gameView, menuView;

// AGREGAR variables globales para tracking de dirección (línea ~75):
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
    cout << "¡Cristal activado en (" << crystalX << "," << crystalY << ") - Dirección: (" << directionX << "," << directionY << ")\n";
    
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
        cout << "¡La salida se ha movido a (" << goalX << ", " << goalY << ")!\n";
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
    
    // ASEGURAR que la celda de inicio se marca como recorrida desde el principio
    grid[startY*W + startX].hasBeenTraversed = true;
    
    // Resetear tracking de posición
    lastX = startX;
    lastY = startY;
    
    verifyGoal(goal);
}

void triggerMapEvent() {
    static vector<int> validIndices; // Reutilizar vector
    validIndices.clear();
    
    // Una sola pasada por el grid
    for (int i = 0; i < W * H; ++i) {
        Cell& c = grid[i];
        if ((c.type == CellType::Empty && !c.hasBeenTraversed) || c.type == CellType::Wall) {
            validIndices.push_back(i);
        }
    }
    
    if (!validIndices.empty()) {
        int idx = validIndices[rand() % validIndices.size()];
        Cell& c = grid[idx];
        
        if (c.type == CellType::Empty) {
            c.type = CellType::Wall;
            c.hasBeenTraversed = false;
            cout << "Evento: aparece muro en (" << (idx % W) << "," << (idx / W) << ")\n";
        } else {
            c.type = CellType::Empty;
            cout << "Evento: desaparece muro en (" << (idx % W) << "," << (idx / W) << ")\n";
        }
    }
}

// REEMPLAZAR la función bfsSolve() completamente (línea ~342):
void bfsSolve() {
    // Cache optimization: reutilizar caminos válidos
    static vector<pair<int,int>> lastPath;
    static int lastStartX = -1, lastStartY = -1, lastGoalX = -1, lastGoalY = -1;
    static int lastTurnCount = -1;
    
    // Verificar si podemos reutilizar el camino anterior
    if (lastStartX == startX && lastStartY == startY && lastGoalX == goalX && 
        lastGoalY == goalY && lastTurnCount == turnCount && !lastPath.empty()) {
        path = lastPath;
        return;
    }
    
    path.clear();
    
    // Verificar que tenemos un grid válido
    if (W <= 0 || H <= 0 || grid.empty()) {
        cout << "Error: Grid inválido\n";
        return;
    }
    
    // Usar arrays planos para mejor cache locality
    vector<int> cost(W * H, INT_MAX);
    vector<int> parentX(W * H, -1);
    vector<int> parentY(W * H, -1);
    queue<int> q;
    
    int startIdx = startY * W + startX;
    int goalIdx = goalY * W + goalX;
    
    // Verificar índices válidos
    if (startIdx < 0 || startIdx >= W * H || goalIdx < 0 || goalIdx >= W * H) {
        cout << "Error: Índices fuera de rango\n";
        return;
    }
    
    cost[startIdx] = 0;
    q.push(startIdx);
    
    // Direcciones: arriba, abajo, izquierda, derecha
    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};
    bool found = false;

    while (!q.empty() && !found) {
        int idx = q.front(); 
        q.pop();
        
        if (idx == goalIdx) { 
            found = true; 
            break; 
        }
        
        int y = idx / W;
        int x = idx % W;
        
        // Explorar las 4 direcciones
        for (int i = 0; i < 4; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            
            // Verificar límites
            if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;
            
            int newIdx = ny * W + nx;
            
            // Verificar que el índice sea válido
            if (newIdx < 0 || newIdx >= W * H) continue;
            
            // Verificar que no sea muro
            if (grid[newIdx].type == CellType::Wall) continue;
            
            // Calcular costo del movimiento
            int moveCost = 1;
            if (grid[newIdx].type == CellType::Crystal) moveCost = 3;
            else if (grid[newIdx].hasBeenTraversed) moveCost = 1;
            
            int newCost = cost[idx] + moveCost;
            
            if (newCost < cost[newIdx]) {
                cost[newIdx] = newCost;
                parentX[newIdx] = x;
                parentY[newIdx] = y;
                q.push(newIdx);
            }
        }
    }

    // Reconstruir camino si se encontró
    if (found) {
        path.reserve(W + H); // Estimación conservadora
        
        int cx = goalX, cy = goalY;
        while (cx != -1 && cy != -1) {
            int idx = cy * W + cx;
            if (idx < 0 || idx >= W * H) break; // Seguridad adicional
            
            path.emplace_back(cy, cx);
            int px = parentX[idx];
            int py = parentY[idx];
            
            // Prevenir loops infinitos
            if (px == cx && py == cy) break;
            
            cx = px; 
            cy = py;
        }
        reverse(path.begin(), path.end());
        
        // Guardar en cache
        lastPath = path;
        lastStartX = startX; lastStartY = startY;
        lastGoalX = goalX; lastGoalY = goalY;
        lastTurnCount = turnCount;
        
        cout << "Camino encontrado con " << path.size() << " pasos\n";
    } else {
        cout << "No se encontró camino hacia la meta\n";
    }
}

// REEMPLAZAR getCellColor() con versión optimizada (línea ~433):
sf::Color getCellColor(CellType type, int x, int y, bool visited, bool isOnPath, bool hasBeenTraversed) {
    // Lookup table estática para colores base
    static const sf::Color wallColor(40, 40, 40);
    static const sf::Color crystalColor(0, 255, 255, 180);
    static const sf::Color goalColor(0, 200, 0, 150);
    static const sf::Color traversedColor(192, 192, 192, 200);
    static const sf::Color normalColor1(120, 120, 200, 120);
    static const sf::Color normalColor2(100, 100, 180, 120);
    
    // Switch optimizado en lugar de ifs secuenciales
    switch (type) {
        case CellType::Wall: return wallColor;
        case CellType::Crystal: return crystalColor;
        case CellType::Goal: return goalColor;
        case CellType::Empty:
        case CellType::Start:
            return hasBeenTraversed ? traversedColor : 
                   ((x + y) & 1) ? normalColor2 : normalColor1; // Bitwise AND más rápido
        default:
            return ((x + y) & 1) ? normalColor2 : normalColor1;
    }
}

bool tryMovePlayer(int newX, int newY, int& currentX, int& currentY, sf::CircleShape& player, sf::CircleShape& goal) {
    // Verificaciones de seguridad
    if (newX < 0 || newX >= W || newY < 0 || newY >= H) return false;
    if (grid.empty() || W <= 0 || H <= 0) return false;
    
    int newIdx = newY * W + newX;
    int currentIdx = currentY * W + currentX;
    
    if (newIdx < 0 || newIdx >= grid.size()) return false;
    if (currentIdx < 0 || currentIdx >= grid.size()) return false;
    
    if (grid[newIdx].type != CellType::Wall) {
        // MARCAR la celda actual ANTES de moverse
        grid[currentIdx].hasBeenTraversed = true;
        
        // Actualizar posición
        currentX = newX;
        currentY = newY;
        sf::Vector2f newPos(
            newX * cellSize + cellSize/2 - cellSize/5,
            newY * cellSize + cellSize/2 - cellSize/5
        );
        player.setPosition(newPos);

        // MARCAR la nueva celda también
        grid[newIdx].hasBeenTraversed = true;
        
        // Solo reflejar cuando el jugador pasa por un cristal
        if (grid[newIdx].type == CellType::Crystal) {
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

// REEMPLAZAR makeTri() con versión optimizada (línea ~516):
sf::ConvexShape makeTri(int x, int y) {
    static sf::ConvexShape tri; // Reutilizar objeto
    tri.setPointCount(3);
    
    // Precálculos constantes
    static const float verticalGap = 8.f;
    static const float horizontalGap = 3.f;
    static const float triWidthBase = cellSize - horizontalGap;
    static const float triHeightBase = cellSize - verticalGap;
    static const float horizontalStretch = 10.f;
    static const sf::Color outlineColor(100, 100, 100, 100);
    
    bool up = (x + y) & 1; // Bitwise AND más rápido

    float px = x * cellSize + horizontalGap * 0.5f;
    float py = y * cellSize + verticalGap * 0.5f;
    
    float left = px - horizontalStretch * 0.5f;
    float right = px + triWidthBase + horizontalStretch * 0.5f;
    float centerX = px + triWidthBase * 0.5f;
    
    float top = py;
    float bottom = py + triHeightBase;

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
    tri.setOutlineColor(outlineColor);
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
    if (!font.loadFromFile("../assets/arial.ttf") && !font.loadFromFile("assets/arial.ttf")) {
        cerr << "Error cargando fuente" << endl;
        return 1;
    }

    sf::RectangleShape menuBackground(sf::Vector2f(menuWidth, 2000));
    menuBackground.setPosition(0, 0);
    menuBackground.setFillColor(sf::Color(25, 25, 35, 240));
    menuBackground.setOutlineThickness(2.f);
    menuBackground.setOutlineColor(sf::Color(100, 100, 150, 150));

    sf::Text titleText("ESCAPE THE GRID", font, 24);
    titleText.setFillColor(sf::Color(150, 255, 255));
    sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setPosition((menuWidth - titleBounds.width) / 2.f, 20);

    vector<unique_ptr<Button>> buttons;
    buttons.push_back(make_unique<Button>(30, 100, 240, 40, "JUGAR", font, sf::Color(50, 150, 50, 200)));
    buttons.push_back(make_unique<Button>(30, 150, 240, 40, "AUTOCOMPLETAR", font, sf::Color(150, 100, 50, 200)));
    buttons.push_back(make_unique<Button>(30, 200, 240, 40, "REINICIAR", font, sf::Color(150, 50, 50, 200)));

    float column1X = 30, column2X = 160, columnsStartY = 260;
    sf::Text infoTitle = createStyledText("INFORMACION", font, 16, sf::Color(255, 255, 150), column1X, columnsStartY);
    vector<sf::Text> infoTexts = {
        createStyledText("- Jugador (azul)", font, 14, sf::Color(100, 150, 255), column1X, columnsStartY + 30),
        createStyledText("- Meta (verde)", font, 14, sf::Color(100, 255, 100), column1X, columnsStartY + 60),
        createStyledText("- Muro (gris)", font, 14, sf::Color(150, 150, 150), column1X, columnsStartY + 90),
        createStyledText("- Cristal (cyan)", font, 14, sf::Color(0, 255, 255), column1X, columnsStartY + 120)
    };

    sf::Text controlsTitle = createStyledText("CONTROLES", font, 16, sf::Color(255, 255, 150), column2X, columnsStartY);
    vector<sf::Text> controlTexts = {
        createStyledText("- Flechas: Mover", font, 14, sf::Color(200, 200, 200), column2X, columnsStartY + 30),
        createStyledText("- ENTER: Resolver", font, 14, sf::Color(200, 200, 200), column2X, columnsStartY + 60),
        createStyledText("- R: Reiniciar", font, 14, sf::Color(200, 200, 200), column2X, columnsStartY + 90),
        createStyledText("- Click: Mover", font, 14, sf::Color(200, 200, 200), column2X, columnsStartY + 120)
    };

    sf::Text statusText("", font, 14);
    sf::Text movesText("Movimientos: 0", font, 14);
    sf::Text timeText("Tiempo: 0:00", font, 14);

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

    // ASEGURAR que la celda de inicio SIEMPRE esté marcada
    grid[startY*W + startX].hasBeenTraversed = true;

    while (window.isOpen()) {
        float dt = moveClock.restart().asSeconds();
        sf::Event e;
        
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) 
                window.close();
            
            if (e.type == sf::Event::Resized) {
                updateViews(window);
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
                            case 0: // JUGAR
                                gameState = GameState::Playing;
                                autoMode = false;
                                solved = false;
                                // ASEGURAR que la celda actual esté marcada al empezar a jugar
                                grid[currentY*W + currentX].hasBeenTraversed = true;
                                gameClock.restart();
                                break;
                            case 1: // AUTOCOMPLETAR
                                if (gameState == GameState::Menu || gameState == GameState::Playing) {
                                    cout << "Iniciando autocompletado desde (" << currentX << "," << currentY << ")\n";
                                    cout << "Meta en (" << goalX << "," << goalY << ")\n";
                                    cout << "Grid size: " << W << "x" << H << " = " << grid.size() << " celdas\n";
                                    
                                    gameState = GameState::Playing;
                                    solved = false;
                                    startY = currentY;
                                    startX = currentX;
                                    
                                    bfsSolve();
                                    
                                    if (!path.empty()) {
                                        autoMode = true;
                                        step = 0;
                                        currentPos = player.getPosition();
                                        moveCount = 0;
                                        gameClock.restart();
                                        cout << "Autocompletado iniciado con " << path.size() << " pasos\n";
                                    } else {
                                        cout << "Error: No se pudo generar camino para autocompletado\n";
                                        autoMode = false;
                                    }
                                }
                                break;
                            case 2: // REINICIAR
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
                                
                                // ASEGURAR que la celda de inicio esté marcada después del reinicio
                                grid[startY*W + startX].hasBeenTraversed = true;
                                lastX = startX;
                                lastY = startY;
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
                sf::Vector2i mousePos(e.mouseMove.x, e.mouseMove.y);
                sf::Vector2f menuCoords = windowToMenuCoords(mousePos, window);
                
                for (auto& button : buttons) {
                    button->updateHover(menuCoords.x, menuCoords.y);
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
                // MARCAR la celda anterior ANTES de moverse
                if (step > 0) { // Solo si no es el primer paso
                    auto [prevY, prevX] = path[step - 1];
                    grid[prevY*W + prevX].hasBeenTraversed = true;
                }
                
                currentPos = nextPos;
                player.setPosition(currentPos);
                currentX = x;
                currentY = y;

                // MARCAR la nueva celda
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
        
        movesText.setString("Movimientos: " + to_string(moveCount));
        timeText.setString("Tiempo: " + to_string(minutes) + ":" + (seconds < 10 ? "0" : "") + to_string(seconds));
        
        switch (gameState) {
            case GameState::Menu:
                statusText.setString("Bienvenido!");
                statusText.setFillColor(sf::Color(150, 255, 150));
                break;
            case GameState::Playing:
                statusText.setString(autoMode ? "Resolviendo..." : "Jugando!");
                statusText.setFillColor(autoMode ? sf::Color(255, 255, 150) : sf::Color(150, 150, 255));
                break;
            case GameState::Solved:
                statusText.setString("Completado!");
                statusText.setFillColor(sf::Color(150, 255, 150));
                break;
        }
        
        sf::FloatRect statusBounds = statusText.getLocalBounds();
        statusText.setPosition((menuWidth - statusBounds.width) / 2.f, windowSize.y - 60);
        
        sf::FloatRect movesBounds = movesText.getLocalBounds();
        movesText.setPosition((menuWidth - movesBounds.width) / 2.f, windowSize.y - 40);
        
        sf::FloatRect timeBounds = timeText.getLocalBounds();
        timeText.setPosition((menuWidth - timeBounds.width) / 2.f, windowSize.y - 20);

        window.clear(sf::Color(15, 15, 25));
        
        window.setView(gameView);

        // Verificar que tengamos un grid válido antes de renderizar
        if (W > 0 && H > 0 && !grid.empty()) {
            for (int y = 0; y < H; ++y) {
                for (int x = 0; x < W; ++x) {
                    int idx = y * W + x;
                    if (idx >= 0 && idx < grid.size()) { // Verificación de seguridad
                        Cell& cell = grid[idx];
                        sf::ConvexShape tri = makeTri(x, y);
                        tri.setFillColor(getCellColor(cell.type, x, y, cell.visited, cell.isOnPath, cell.hasBeenTraversed));
                        window.draw(tri);
                        
                        // Efecto especial para cristales
                        if (cell.type == CellType::Crystal) {
                            sf::ConvexShape crystalTri = makeTri(x, y);
                            crystalTri.setFillColor(sf::Color(255, 255, 255, 50));
                            window.draw(crystalTri);
                        }
                    }
                }
            }
        }

        verifyGoal(goal);
        window.draw(goal);
        window.draw(player);
        
        window.setView(menuView);
        
        window.draw(menuBackground);
        window.draw(titleText);
        
        for (auto& button : buttons) {
            button->draw(window);
        }
        
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