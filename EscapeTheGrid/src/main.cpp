#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <iostream>
#include <cmath>
#include <memory>
#include <unordered_map>

using namespace std;

// Tipos de celdas del laberinto
enum class CellType { Empty, Wall, Start, Goal, Crystal };
// Estados del juego
enum class GameState { Menu, Playing, Solved };

// Estructura que representa cada celda del laberinto
struct Cell {
    CellType type;
    bool visited = false;
    bool isOnPath = false;
    bool hasBeenTraversed = false;
    bool isReflected = false;
    bool isDirty = false;
};

// Clase para crear botones interactivos del menú
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
        
         // Centrar el texto en el botón
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

// Variables globales del juego
int turnCount = 0;
const int TURNS_PER_EVENT = 5;      // Cada 5 turnos ocurre un evento
const int TURNS_TO_MOVE_GOAL = 10; // Cada 10 turnos se mueve la meta
int turnsSinceLastGoalMove = 0;
int W, H;                          // Ancho y alto del laberinto
vector<Cell> grid;                 // Grid del laberinto
int startX, startY, goalX, goalY;  // Posiciones de inicio y meta
vector<pair<int,int>> path;        // Camino calculado por BFS
GameState gameState = GameState::Menu;

// Configuración de la interfaz
float cellSize = 35.f;
float menuWidth = 300.f;
sf::Vector2f gameOffset(0, 0);
sf::View gameView, menuView;

// Verifica que la meta esté en la posición correcta
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

// Mueve la meta a una nueva posición aleatoria
void moveGoal(int currentX, int currentY, sf::CircleShape& goal) {
    int oldGoalX = goalX;
    int oldGoalY = goalY;
    
     // Buscar celdas vacías disponibles
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
        // Algoritmo híbrido: 70% aleatorio, 30% considerando distancia
        int selectedIndex;
        
        if (rand() % 100 < 70) {
            
            selectedIndex = rand() % emptyCells.size();
        } else {
            // Preferir posiciones a distancia media del jugador
            vector<pair<int, int>> balancedCells;
            
            for (size_t i = 0; i < emptyCells.size(); i++) {
                auto [y, x] = emptyCells[i];
                int distance = abs(x - currentX) + abs(y - currentY);
              
                if (distance >= 3 && distance <= 8) {
                    balancedCells.push_back({y, x});
                }
            }
            
            if (!balancedCells.empty()) {

                int balancedIndex = rand() % balancedCells.size();
                auto [selectedY, selectedX] = balancedCells[balancedIndex];
                
                // Encontrar el índice en emptyCells
                for (size_t i = 0; i < emptyCells.size(); i++) {
                    if (emptyCells[i].first == selectedY && emptyCells[i].second == selectedX) {
                        selectedIndex = i;
                        break;
                    }
                }
            } else {

                selectedIndex = rand() % emptyCells.size();
            }
        }
        
        auto [newY, newX] = emptyCells[selectedIndex];
        grid[oldGoalY*W + oldGoalX].type = CellType::Empty;
        goalX = newX;
        goalY = newY;
        grid[goalY*W + goalX].type = CellType::Goal;
        
        verifyGoal(goal);
        cout << "¡La salida se ha movido a (" << goalX << ", " << goalY << ")!\n";
    }
    
    turnsSinceLastGoalMove = 0;
}

// Carga el laberinto desde el archivo de texto
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
    
    grid.resize(W * H);
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
            // Interpretar símbolos del archivo
            if (t == "#") grid[y*W + x] = {CellType::Wall};
            else if (t == "S") grid[y*W + x] = {CellType::Start};
            else if (t == "G") grid[y*W + x] = {CellType::Goal};
            else if (t == "C" || t == "K") grid[y*W + x] = {CellType::Crystal};
            else grid[y*W + x] = {CellType::Empty};
        }
    }

    return true;
}

// Crea un laberinto por defecto si no se puede cargar desde archivo
void createDefaultMaze() {
    W = 12; H = 10;
    startX = 1; startY = 1;
    goalX = 10; goalY = 8;
    
    grid.resize(W * H);

    // Inicializar todo como vacío
    for (auto& cell : grid) {
        cell = {CellType::Empty};
    }

    // Definir posiciones de muros manualmente
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

    // Colocar cristales
    grid[3*W + 6] = {CellType::Crystal};
    grid[6*W + 9] = {CellType::Crystal};

     // Colocar inicio y meta
    grid[startY*W + startX] = {CellType::Start};
    grid[goalY*W + goalX] = {CellType::Goal};
}

// Reinicia el juego al estado inicial
void resetGame(sf::CircleShape& goal) {
    gameState = GameState::Menu;
    turnCount = 0;
    turnsSinceLastGoalMove = 0;
    path.clear();

     // Intentar cargar el laberinto desde archivo
    if (!loadMaze("../assets/maze.txt") && !loadMaze("assets/maze.txt") && !loadMaze("maze.txt")) {
        createDefaultMaze();
    }

    // Resetear estado de todas las celdas
    for (auto& cell : grid) {
        cell.visited = false;
        cell.isOnPath = false;
        cell.hasBeenTraversed = false;
        cell.isReflected = false;
    }
    
    grid[startY*W + startX].hasBeenTraversed = true;
    verifyGoal(goal);
}

// Verifica si las coordenadas están dentro del laberinto
bool inside(int y, int x) {
    return x >= 0 && x < W && y >= 0 && y < H;
}

// Genera eventos aleatorios que modifican el mapa
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

// Algoritmo BFS para encontrar el camino más corto
void bfsSolve() {
    int tempGoalX = goalX;
    int tempGoalY = goalY;
    
    // Resetear visitados
    for (auto& cell : grid) {
        cell.visited = false;
        cell.isOnPath = false;
    }
    
    grid[tempGoalY*W + tempGoalX].type = CellType::Goal;
    goalX = tempGoalX;
    goalY = tempGoalY;
    
    path.clear();
    
    vector<vector<bool>> vis(H, vector<bool>(W, false));
    vector<vector<pair<int,int>>> parent(H, vector<pair<int,int>>(W, {-1,-1}));
    queue<pair<int,int>> q;
    q.push({startY, startX});
    vis[startY][startX] = true;
    
    // Direcciones: arriba, abajo, derecha, izquierda
    int dirs[4][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}};
    bool found = false;

    while (!q.empty()) {
        auto [y,x] = q.front(); q.pop();
        
        
        if (y == goalY && x == goalX) { 
            found = true; 
            break; 
        }
        
        // Explorar vecinos
        for (auto& d : dirs) {
            int ny = y + d[0];
            int nx = x + d[1];
            
            if (!inside(ny,nx)) continue;
            if (grid[ny*W+nx].type == CellType::Wall || vis[ny][nx]) continue;
            
            vis[ny][nx] = true;
            parent[ny][nx] = {y,x};
            q.push({ny,nx});
        }
    }

    // Reconstruir camino si se encontró
    if (found) {
        for (int cy = goalY, cx = goalX; cy != -1; ) {
            path.push_back({cy,cx});
         
            auto p = parent[cy][cx];
            cy = p.first;
            cx = p.second;
        }
        reverse(path.begin(), path.end());
    }
}

// Determina el color de cada celda según su tipo y estado
sf::Color getCellColor(CellType type, int x, int y, bool visited, bool isOnPath, bool hasBeenTraversed, bool isReflected) {
    if (type == CellType::Crystal) return sf::Color(0, 255, 255, 180);
    if (type == CellType::Goal) return sf::Color(0, 200, 0, 150);
    if (isReflected) return sf::Color(150, 220, 255, 200);
    if (hasBeenTraversed) return sf::Color(192, 192, 192, 200);
    
    switch (type) {
        case CellType::Wall: return sf::Color(40, 40, 40);
        case CellType::Start: return sf::Color(100, 255, 100, 200);
        case CellType::Empty:
        default:
            return ((x + y) % 2 == 0) ? sf::Color(120, 120, 200, 120) : sf::Color(100, 100, 180, 120);
    }
}

// DFS recursivo para propagar reflejos desde cristales
void reflectDFS(int x, int y, int dx, int dy, int sourceX, int sourceY) {
    int nextX = x + dx;
    int nextY = y + dy;

    if (!inside(nextY, nextX)) return;
    if (grid[nextY*W + nextX].type == CellType::Wall) return;
    
    // Si la celda fuente está traversed, reflejar
    if (inside(sourceY, sourceX) && grid[sourceY*W + sourceX].hasBeenTraversed) {
        grid[nextY*W + nextX].hasBeenTraversed = true;
        grid[nextY*W + nextX].isReflected = true;
        
        // Continuar DFS
        reflectDFS(nextX, nextY, dx, dy, sourceX - dx, sourceY - dy);
    }
}

// Maneja los reflejos de luz de un cristal específico
void reflectFromCrystal(int crystalX, int crystalY) {
    // Direcciones: izquierda, derecha, arriba, abajo
    int dirs[4][2] = {{-1,0}, {1,0}, {0,-1}, {0,1}};
    
    for (int d = 0; d < 4; d++) {
        int dx = dirs[d][0];
        int dy = dirs[d][1];
        
        // Buscar celdas traversed en dirección opuesta
        int sourceX = crystalX - dx;
        int sourceY = crystalY - dy;
        
        // Propagar reflejo en la dirección correspondiente
        reflectDFS(crystalX, crystalY, dx, dy, sourceX, sourceY);
    }
}

// Actualiza todos los reflejos de cristales en el mapa
void reflectCrystals() {
    // Limpiar reflejos anteriores
    for (auto& c : grid) {
        c.isReflected = false;
    }

    // Procesar todos los cristales
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            if (grid[y*W + x].type != CellType::Crystal)
                continue;

            reflectFromCrystal(x, y);
        }
    }
}

// Versión optimizada del sistema de reflejos (no recursiva)
void optimizedReflectCrystals() {
    // Reset solo las celdas reflejadas previamente
    static vector<pair<int,int>> previouslyReflected;
    for (auto [x, y] : previouslyReflected) {
        grid[y*W + x].isReflected = false;
        grid[y*W + x].isDirty = true;
    }
    previouslyReflected.clear();
    
    // Usar queue en lugar de recursión para evitar stack overflow
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            if (grid[y*W + x].type != CellType::Crystal) continue;
            
            queue<pair<int,int>> q;
            int dirs[4][2] = {{-1,0}, {1,0}, {0,-1}, {0,1}};
            
            for (int d = 0; d < 4; d++) {
                int dx = dirs[d][0];
                int dy = dirs[d][1];
                
                q.push({x, y});
                
                while (!q.empty()) {
                    auto [cx, cy] = q.front();
                    q.pop();
                    
                    int nextX = cx + dx;
                    int nextY = cy + dy;
                    
                    if (!inside(nextY, nextX)) break;
                    if (grid[nextY*W + nextX].type == CellType::Wall) break;
                    
                    int sourceX = cx - dx;
                    int sourceY = cy - dy;
                    
                    if (inside(sourceY, sourceX) && grid[sourceY*W + sourceX].hasBeenTraversed) {
                        grid[nextY*W + nextX].hasBeenTraversed = true;
                        grid[nextY*W + nextX].isReflected = true;
                        grid[nextY*W + nextX].isDirty = true;
                        previouslyReflected.push_back({nextX, nextY});
                        q.push({nextX, nextY});
                    }
                }
            }
        }
    }
}

// Intenta mover al jugador a una nueva posición
bool tryMovePlayer(int newX, int newY, int& currentX, int& currentY, sf::CircleShape& player, sf::CircleShape& goal) {
    if (inside(newY, newX) && grid[newY*W + newX].type != CellType::Wall) {
        currentX = newX;
        currentY = newY;
        sf::Vector2f newPos(
            newX * cellSize + cellSize/2 - cellSize/5,
            newY * cellSize + cellSize/2 - cellSize/5
        );
        player.setPosition(newPos);

        // Marcar celda como atravesada y actualizar reflejos
        grid[currentY*W + currentX].hasBeenTraversed = true;
        reflectCrystals();

        // Incrementar contadores y verificar eventos
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

// Crea formas triangulares para representar las celdas
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

    // Alternar orientación del triángulo
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

// Texto personalizado
sf::Text createStyledText(const string& text, sf::Font& font, int size, sf::Color color, float x, float y) {
    sf::Text styledText(text, font, size);
    styledText.setFillColor(color);
    styledText.setPosition(x, y);
    return styledText;
}

// Convierte coordenadas de ventana a coordenadas del juego
sf::Vector2f windowToGameCoords(sf::Vector2i windowPos, const sf::RenderWindow& window) {
    return window.mapPixelToCoords(windowPos, gameView);
}

// Convierte coordenadas de ventana a coordenadas del menú
sf::Vector2f windowToMenuCoords(sf::Vector2i windowPos, const sf::RenderWindow& window) {
    return window.mapPixelToCoords(windowPos, menuView);
}

// Actualiza las vistas cuando cambia el tamaño de la ventana
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

// Calcula el número mínimo de movimientos usando programación dinámica
int calculateMinMoves() {
    vector<vector<int>> dp(H, vector<int>(W, INT_MAX));
    dp[startY][startX] = 0;
    
    queue<pair<int,int>> q;
    q.push({startY, startX});
    
    while (!q.empty()) {
        auto [y, x] = q.front();
        q.pop();
        
        if (y == goalY && x == goalX) {
            return dp[y][x];
        }
        
        int dirs[4][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}};
        
        for (auto& d : dirs) {
            int ny = y + d[0];
            int nx = x + d[1];
            
            if (inside(ny, nx) && grid[ny*W + nx].type != CellType::Wall) {
                int newCost = dp[y][x] + 1;
                
                // Considerar costo adicional por eventos cada 5 movimientos
                if ((newCost % TURNS_PER_EVENT) == 0) {
                    newCost += 2; // Penalización por evento
                }
                
                if (newCost < dp[ny][nx]) {
                    dp[ny][nx] = newCost;
                    q.push({ny, nx});
                }
            }
        }
    }
    
    return -1; // No hay camino
}

class SpatialHash {
private:
    unordered_map<int, vector<pair<int,int>>> grid;
    int cellSize;
    
public:
    SpatialHash(int size) : cellSize(size) {}
    
    int hash(int x, int y) {
        return (x / cellSize) * 1000 + (y / cellSize);
    }
    
    void insert(int x, int y) {
        grid[hash(x, y)].push_back({x, y});
    }
    
    vector<pair<int,int>> getNearby(int x, int y) {
        vector<pair<int,int>> nearby;
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                int key = hash(x + dx * cellSize, y + dy * cellSize);
                if (grid.count(key)) {
                    nearby.insert(nearby.end(), grid[key].begin(), grid[key].end());
                }
            }
        }
        return nearby;
    }
};

// Gestor de recursos para cargar fuentes y texturas eficientemente
class ResourceManager {
private:
    static unordered_map<string, sf::Font> fonts;
    static unordered_map<string, sf::Texture> textures;
    static sf::Font defaultFont;
    
public:
    static sf::Font& getFont(const string& path) {
        // Verificar si ya está en caché
        if (fonts.find(path) != fonts.end()) {
            return fonts[path];
        }
        
        // Intentar cargar la fuente
        sf::Font font;
        if (font.loadFromFile(path)) {
            fonts[path] = font;
            cout << "Fuente cargada exitosamente: " << path << endl;
            return fonts[path];
        } else {
            cout << "Error cargando fuente: " << path << endl;
        
            return getDefaultFont();
        }
    }
    
    static sf::Font& getDefaultFont() {
        if (defaultFont.getInfo().family.empty()) {
            // Intentar cargar fuente por defecto del sistema
            if (!defaultFont.loadFromFile("C:/Windows/Fonts/arial.ttf")) {

                cout << "No se pudo cargar fuente por defecto" << endl;
            }
        }
        return defaultFont;
    }
    
    static void loadSystemFont() {
        getDefaultFont();
    }
    
    static size_t getCacheSize() {
        return fonts.size() + textures.size();
    }
    
    static void clearCache() {
        fonts.clear();
        textures.clear();
        cout << "Caché de recursos limpiado" << endl;
    }
};

// Definiciones estáticas del ResourceManager
unordered_map<string, sf::Font> ResourceManager::fonts;
unordered_map<string, sf::Texture> ResourceManager::textures;
sf::Font ResourceManager::defaultFont;

int main() {
    // Inicializar generador de números aleatorios
    srand(time(nullptr));
    
    // Cargar laberinto
    if (!loadMaze("../assets/maze.txt") && !loadMaze("assets/maze.txt") && !loadMaze("maze.txt")) {
        createDefaultMaze();
    }

    // Configurar ventana
    float initialGameWidth = W * cellSize;
    float initialGameHeight = H * cellSize;
    float initialTotalWidth = initialGameWidth + menuWidth;
    
    sf::RenderWindow window(sf::VideoMode(initialTotalWidth, initialGameHeight), "Escape the Grid");
    window.setFramerateLimit(60);
    
    updateViews(window);
    
    // USAR ResourceManager en lugar de carga directa
    sf::Font& font = ResourceManager::getFont("../assets/arial.ttf");
    if (font.getInfo().family.empty()) {
        // Fallback si no se encuentra el archivo
        sf::Font& fallbackFont = ResourceManager::getFont("assets/arial.ttf");
        if (fallbackFont.getInfo().family.empty()) {
            // Si tampoco encuentra el fallback, usar fuente del sistema
            ResourceManager::loadSystemFont();
        }
    }
    
    // Configuración del menú lateral
    sf::RectangleShape menuBackground(sf::Vector2f(menuWidth, 2000));
    menuBackground.setPosition(0, 0);
    menuBackground.setFillColor(sf::Color(25, 25, 35, 240));
    menuBackground.setOutlineThickness(2.f);
    menuBackground.setOutlineColor(sf::Color(100, 100, 150, 150));

    // Título del juego
    sf::Text titleText("ESCAPE THE GRID", font, 24);
    titleText.setFillColor(sf::Color(150, 255, 255));
    sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setPosition((menuWidth - titleBounds.width) / 2.f, 20);

    // Botones del menú
    vector<unique_ptr<Button>> buttons;
    buttons.push_back(make_unique<Button>(30, 100, 240, 40, "JUGAR", font, sf::Color(50, 150, 50, 200)));
    buttons.push_back(make_unique<Button>(30, 150, 240, 40, "AUTOCOMPLETAR", font, sf::Color(150, 100, 50, 200)));
    buttons.push_back(make_unique<Button>(30, 200, 240, 40, "REINICIAR", font, sf::Color(150, 50, 50, 200)));

    // Textos informativos en dos columnas
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

    // Textos de estado del juego
    sf::Text statusText("", font, 14);
    sf::Text movesText("Movimientos: 0", font, 14);
    sf::Text timeText("Tiempo: 0:00", font, 14);

    // Crear jugador y meta como círculos
    sf::CircleShape player(cellSize/5), goal(cellSize/5);
    player.setFillColor(sf::Color(50, 150, 255));
    player.setOutlineThickness(2.f);
    player.setOutlineColor(sf::Color::White);
    goal.setFillColor(sf::Color(0, 255, 0, 255));
    goal.setOutlineThickness(3.f);
    goal.setOutlineColor(sf::Color::White);

    // Posicionar jugador en el centro de la celda inicial
    player.setPosition(startX * cellSize + cellSize/2 - cellSize/5, startY * cellSize + cellSize/2 - cellSize/5);
    verifyGoal(goal);

    // Variables de control del juego
    bool autoMode = false;
    bool solved = false;
    size_t step = 0;
    sf::Vector2f currentPos = player.getPosition();
    sf::Clock gameClock, moveClock;
    int currentX = startX, currentY = startY;
    int moveCount = 0;
    
    grid[startY*W + startX].hasBeenTraversed = true; // Marcar celda inicial como visitada

    // LOOP PRINCIPAL DEL JUEGO
    while (window.isOpen()) {
        float dt = moveClock.restart().asSeconds(); // Delta time para animaciones
        sf::Event e;
        
         // Manejar eventos
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) 
                window.close();
            
            if (e.type == sf::Event::Resized) {
                updateViews(window); // Reajustar vistas cuando se redimensiona
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
                                gameClock.restart();
                                break;
                            case 1: // AUTOCOMPLETAR
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
                            
                            // Solo permitir movimientos a celdas adyacentes
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
            
            // TECLAS
            if (e.type == sf::Event::KeyPressed) {
                // Movimiento con flechas
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
                
                // ENTER para activar modo automático
                if (e.key.code == sf::Keyboard::Enter && !autoMode && gameState == GameState::Playing) {
                    startY = currentY;
                    startX = currentX;
                    bfsSolve();
                    autoMode = true;
                    step = 0;
                    currentPos = player.getPosition();
                }
                
                // R para reiniciar
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
            float animationSpeed = 5.f;

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
                reflectCrystals();

                turnCount++;
                turnsSinceLastGoalMove++;
                
                // Mover la meta cada ciertos turnos
                if (turnsSinceLastGoalMove >= TURNS_TO_MOVE_GOAL) {
                    moveGoal(currentX, currentY, goal);
                }
                
                if (turnCount % TURNS_PER_EVENT == 0) {
                    triggerMapEvent(); // Cambiar el mapa
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

                // Verificar si llegó a la meta
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

          // Actualizar tiempo transcurrido
        float elapsedTime = gameClock.getElapsedTime().asSeconds();
        int minutes = (int)elapsedTime / 60;
        int seconds = (int)elapsedTime % 60;
        
        sf::Vector2u windowSize = window.getSize();
        
        // Actualizar textos de información
        movesText.setString("Movimientos: " + to_string(moveCount));
        timeText.setString("Tiempo: " + to_string(minutes) + ":" + (seconds < 10 ? "0" : "") + to_string(seconds));
        
        // Actualizar texto de estado según el estado del juego
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
        
        // Dibujar área de juego
        window.setView(gameView);

        // Dibujar cada celda del grid
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                Cell& cell = grid[y*W + x];
                sf::ConvexShape tri = makeTri(x, y);
                tri.setFillColor(getCellColor(cell.type, x, y, cell.visited, cell.isOnPath, cell.hasBeenTraversed, cell.isReflected));
                window.draw(tri);
                
                 // Efecto extra para cristales
                if (cell.type == CellType::Crystal) {
                    sf::ConvexShape crystalTri = makeTri(x, y);
                    crystalTri.setFillColor(sf::Color(255, 255, 255, 50));
                    window.draw(crystalTri);
                }
            }
        }

        // Dibujar meta y jugador
        verifyGoal(goal);
        window.draw(goal);
        window.draw(player);

        // Dibujar interfaz del menú
        window.setView(menuView);
        
        window.draw(menuBackground);
        

        window.draw(titleText);
        
        // Dibujar botones 
        for (auto& button : buttons) {
            button->draw(window);
        }
        
        // Dibujar textos informativos
        window.draw(infoTitle);
        for (auto& text : infoTexts) window.draw(text);
        
        window.draw(controlsTitle);
        for (auto& text : controlTexts) window.draw(text);
        
        window.draw(statusText);
        window.draw(movesText);
        window.draw(timeText);

        // Mostrar todo en pantalla
        window.display();
    }

    return 0;
}