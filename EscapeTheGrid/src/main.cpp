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

// Tipos de celdas del laberinto
enum class CellType { 
    Empty, Wall, Start, Goal, Crystal
};

// Estados del juego
enum class GameState { 
    Menu, Playing, Solved
};

// Estructura que representa cada celda del laberinto
struct Cell { 
    CellType type;
    bool visited = false;
    bool isOnPath = false;
    bool hasBeenTraversed = false;
};

// Botón de interfaz con funcionalidad completa
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
        text.setStyle(sf::Text::Bold);
        sf::FloatRect textBounds = text.getLocalBounds();
        text.setPosition(
            x + (width - textBounds.width) / 2.f,
            y + (height - textBounds.height) / 2.f - 2.f
        );
    }

    // Detecta si el punto esta del boton y detecta clics en el mouse
    bool contains(float x, float y) const {
        return shape.getGlobalBounds().contains(x, y);
    }
    
    // Actualiza el estado visual del botón basado en la posición del mouse
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

int lastX = -1, lastY = -1;         // Para tracking de dirección en cristales



// Función de validación de límites
bool inside(int y, int x) {
    return x >= 0 && x < W && y >= 0 && y < H;
}


// Refleja el mapa horizontalmente usando el cristal como eje
void reflectHorizontally(int crystalX, int crystalY) {
    // Reflejar lado izquierdo hacia derecha
    for (int x = 0; x < crystalX; x++) {
        int reflectedX = crystalX + (crystalX - x);
        if (reflectedX < W) {
            grid[crystalY*W + reflectedX] = grid[crystalY*W + x];
        }
    }
    
    // Reflejar lado derecho hacia izquierda
    for (int x = crystalX + 1; x < W; x++) {
        int reflectedX = crystalX - (x - crystalX);
        if (reflectedX >= 0) {
            grid[crystalY*W + reflectedX] = grid[crystalY*W + x];
        }
    }
}

// Refleja el mapa verticalmente usando el cristal como eje
void reflectVertically(int crystalX, int crystalY) {
    // Reflejar lado superior hacia abajo
    for (int y = 0; y < crystalY; y++) {
        int reflectedY = crystalY + (crystalY - y);
        if (reflectedY < H) {
            grid[reflectedY*W + crystalX] = grid[y*W + crystalX];
        }
    }
    
    // Reflejar lado inferior hacia arriba
    for (int y = crystalY + 1; y < H; y++) {
        int reflectedY = crystalY - (y - crystalY);
        if (reflectedY >= 0) {
            grid[reflectedY*W + crystalX] = grid[y*W + crystalX];
        }
    }
}

// Decide qué tipo de reflexión aplicar según la dirección de movimiento
void reflectDynamically(int crystalX, int crystalY, int directionX, int directionY) {
    cout << "¡Cristal activado en (" << crystalX << "," << crystalY << ")\n";
    
    if (directionX != 0) {
        reflectHorizontally(crystalX, crystalY);
        cout << "Reflejo HORIZONTAL activado\n";
    } else if (directionY != 0) {
        reflectVertically(crystalX, crystalY);
        cout << "Reflejo VERTICAL activado\n";
    }
}

// Función principal para manejar cristales
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


// Asegura que la meta esté correctamente posicionada
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


// Carga el laberinto desde un archivo de texto
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

// Crea un laberinto por defecto si no se puede cargar desde archivo
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

// Reinicia el juego a su estado inicial
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

// Eventos dinámicos del mapa - aparecen/desaparecen muros
void triggerMapEvent() {
    static vector<int> validIndices;
    validIndices.clear();
    
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

// Algoritmo BFS para encontrar el camino más corto al objetivo
void bfsSolve() {
    // Cache para evitar recalcular el mismo camino
    static vector<pair<int,int>> lastPath;
    static int lastStartX = -1, lastStartY = -1, lastGoalX = -1, lastGoalY = -1;
    static int lastTurnCount = -1;
    
    if (lastStartX == startX && lastStartY == startY && lastGoalX == goalX && 
        lastGoalY == goalY && lastTurnCount == turnCount && !lastPath.empty()) {
        path = lastPath;
        return;
    }
    
    path.clear();
    
    if (W <= 0 || H <= 0 || grid.empty()) {
        cout << "Error: Grid inválido\n";
        return;
    }
    
    vector<int> cost(W * H, INT_MAX);
    vector<int> parentX(W * H, -1);
    vector<int> parentY(W * H, -1);
    queue<int> q;
    
    int startIdx = startY * W + startX;
    int goalIdx = goalY * W + goalX;
    
    if (startIdx < 0 || startIdx >= W * H || goalIdx < 0 || goalIdx >= W * H) {
        cout << "Error: Índices fuera de rango\n";
        return;
    }
    
    cost[startIdx] = 0;
    q.push(startIdx);
    
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
        
        for (int i = 0; i < 4; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            
            if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;
            
            int newIdx = ny * W + nx;
            
            if (newIdx < 0 || newIdx >= W * H) continue;
            if (grid[newIdx].type == CellType::Wall) continue;
            
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

    // Reconstruir camino
    if (found) {
        path.reserve(W + H);
        
        int cx = goalX, cy = goalY;
        while (cx != -1 && cy != -1) {
            int idx = cy * W + cx;
            if (idx < 0 || idx >= W * H) break;
            
            path.emplace_back(cy, cx);
            int px = parentX[idx];
            int py = parentY[idx];
            
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

sf::Color getCellColor(CellType type, int x, int y, bool visited, bool isOnPath, bool hasBeenTraversed) {
    static const sf::Color wallColor(40, 40, 40);           
    static const sf::Color crystalColor(0, 255, 255, 180);  
    static const sf::Color goalColor(0, 200, 0, 150);       
    static const sf::Color traversedColor(192, 192, 192, 200); 
    static const sf::Color normalColor1(120, 120, 200, 120);   
    static const sf::Color normalColor2(100, 100, 180, 120);   
    
    switch (type) {
        case CellType::Wall: 
            return wallColor;
        case CellType::Crystal: 
            return crystalColor;
        case CellType::Goal: 
            return goalColor;
        case CellType::Empty:
        case CellType::Start:

            return hasBeenTraversed ? traversedColor : 
                   ((x + y) & 1) ? normalColor2 : normalColor1; 
        default:
            return ((x + y) & 1) ? normalColor2 : normalColor1;
    }
}



// Función principal para manejar el movimiento del jugador
bool tryMovePlayer(int newX, int newY, int& currentX, int& currentY, sf::CircleShape& player, sf::CircleShape& goal) {
    // Verificaciones de seguridad - evitar crashes por accesos inválidos
    if (newX < 0 || newX >= W || newY < 0 || newY >= H) return false; 
    if (grid.empty() || W <= 0 || H <= 0) return false;               
    
    int newIdx = newY * W + newX;
    int currentIdx = currentY * W + currentX;
    
    if (newIdx < 0 || newIdx >= grid.size()) return false;
    if (currentIdx < 0 || currentIdx >= grid.size()) return false;
    
    // Verificar que la celda destino no sea un muro
    if (grid[newIdx].type != CellType::Wall) {
        grid[currentIdx].hasBeenTraversed = true;
        
        // Actualizar posición lógica del jugador
        currentX = newX;
        currentY = newY;
        
        // Calcular nueva posición visual en píxeles
        sf::Vector2f newPos(
            newX * cellSize + cellSize/2 - cellSize/5,    
            newY * cellSize + cellSize/2 - cellSize/5     
        );
        player.setPosition(newPos);

        // MARCAR la nueva celda también como recorrida
        grid[newIdx].hasBeenTraversed = true;
        
        // Solo reflejar cuando el jugador pasa por un cristal (no cuando solo se mueve)
        if (grid[newIdx].type == CellType::Crystal) {
            reflectCrystals(currentX, currentY);  // Activar reflexión del mapa
        } else {
            // Actualizar posición anterior sin activar reflejo
            lastX = currentX;
            lastY = currentY;
        }

        turnCount++;
        turnsSinceLastGoalMove++;
        
        // Verificar si es tiempo de mover la meta
        if (turnsSinceLastGoalMove >= TURNS_TO_MOVE_GOAL) {
            moveGoal(currentX, currentY, goal);
        }
        
        // Verificar si es tiempo de disparar evento de mapa
        if (turnCount % TURNS_PER_EVENT == 0) {
            triggerMapEvent();  
        }

        return true;  
    }
    return false; 
}



// Función optimizada para crear triángulos que representan las celdas del laberinto
sf::ConvexShape makeTri(int x, int y) {
    static sf::ConvexShape tri; 
    tri.setPointCount(3);       
    
    // Precálculos constantes - calculados una sola vez para mejor performance
    static const float verticalGap = 8.f;      
    static const float horizontalGap = 3.f;    
    static const float triWidthBase = cellSize - horizontalGap;   
    static const float triHeightBase = cellSize - verticalGap;    
    static const float horizontalStretch = 10.f;                  
    static const sf::Color outlineColor(100, 100, 100, 100);      
    
    // Determinar orientación del triángulo usando patrón de tablero de ajedrez
    bool up = (x + y) & 1; 

    // Calcular posición base del triángulo en el grid
    float px = x * cellSize + horizontalGap * 0.5f;
    float py = y * cellSize + verticalGap * 0.5f;
    
    // Calcular coordenadas de los vértices del triángulo
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

// Función helper para crear texto estilizado de manera consistente
sf::Text createStyledText(const string& text, sf::Font& font, int size, sf::Color color, float x, float y) {
    sf::Text styledText(text, font, size);
    styledText.setFillColor(color);
    styledText.setPosition(x, y);
    return styledText;
}

// Función para convertir coordenadas de ventana a coordenadas del juego
sf::Vector2f windowToGameCoords(sf::Vector2i windowPos, const sf::RenderWindow& window) {
    return window.mapPixelToCoords(windowPos, gameView);
}

// Función para convertir coordenadas de ventana a coordenadas del menú
sf::Vector2f windowToMenuCoords(sf::Vector2i windowPos, const sf::RenderWindow& window) {
    return window.mapPixelToCoords(windowPos, menuView);
}

// Función para actualizar las vistas cuando cambia el tamaño de ventana
// Implementa responsive design para diferentes tamaños de ventana
void updateViews(sf::RenderWindow& window) {
    sf::Vector2u windowSize = window.getSize();  // Obtener tamaño actual de ventana
    
    // Calcular dimensiones del área de juego en píxeles
    float gameWidth = W * cellSize;
    float gameHeight = H * cellSize;
    

    float gameViewWidth = windowSize.x - menuWidth;
    

    gameView.reset(sf::FloatRect(0, 0, gameWidth, gameHeight));
    gameView.setViewport(sf::FloatRect(0, 0, gameViewWidth / windowSize.x, 1.0f));

    menuView.reset(sf::FloatRect(0, 0, menuWidth, windowSize.y));
    menuView.setViewport(sf::FloatRect(gameViewWidth / windowSize.x, 0, menuWidth / windowSize.x, 1.0f));
}



// Implementa el game loop principal y maneja toda la lógica de la aplicación
int main() {
    // ==================== INICIALIZACIÓN ====================
    
    // Intentar cargar laberinto desde archivo, con fallback a laberinto por defecto
    if (!loadMaze("../assets/maze.txt") && !loadMaze("assets/maze.txt") && !loadMaze("maze.txt")) {
        createDefaultMaze();  // Si no se encuentra archivo, crear laberinto hardcodeado
    }

    // Calcular dimensiones iniciales de la ventana
    float initialGameWidth = W * cellSize;          
    float initialGameHeight = H * cellSize;         
    float initialTotalWidth = initialGameWidth + menuWidth; 
    
    // Crear ventana principal con SFML
    sf::RenderWindow window(sf::VideoMode(initialTotalWidth, initialGameHeight), "Escape the Grid");
    window.setFramerateLimit(60);  
    

    updateViews(window);
    
    sf::Font font;
    if (!font.loadFromFile("../assets/arial.ttf") && !font.loadFromFile("assets/arial.ttf")) {
        cerr << "Error cargando fuente" << endl;
        return 1;  // Error crítico - sin fuente no podemos mostrar texto
    }
    
    sf::RectangleShape menuBackground(sf::Vector2f(menuWidth, 2000));  // Alto grande para cubrir toda la pantalla
    menuBackground.setPosition(0, 0);
    menuBackground.setFillColor(sf::Color(25, 25, 35, 240));      // Azul oscuro semi-transparente
    menuBackground.setOutlineThickness(2.f);
    menuBackground.setOutlineColor(sf::Color(100, 100, 150, 150)); // Borde azul claro

    // Crear título del juego
    sf::Text titleText("ESCAPE THE GRID", font, 24);
    titleText.setFillColor(sf::Color(150, 255, 255));  
    sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setPosition((menuWidth - titleBounds.width) / 2.f, 20);  

    // Crear botones de la interfaz usando smart pointers (gestión automática de memoria)
    vector<unique_ptr<Button>> buttons;
    buttons.push_back(make_unique<Button>(30, 100, 240, 40, "JUGAR", font, sf::Color(50, 150, 50, 200)));           
    buttons.push_back(make_unique<Button>(30, 150, 240, 40, "AUTOCOMPLETAR", font, sf::Color(150, 100, 50, 200)));  
    buttons.push_back(make_unique<Button>(30, 200, 240, 40, "REINICIAR", font, sf::Color(150, 50, 50, 200)));       

    // Crear textos informativos del menú - divididos en dos columnas
    float column1X = 30, column2X = 160, columnsStartY = 260;
    
    // Sección de información (columna izquierda)
    sf::Text infoTitle = createStyledText("INFORMACION", font, 16, sf::Color(255, 255, 150), column1X, columnsStartY);
    vector<sf::Text> infoTexts = {
        createStyledText("- Jugador (azul)", font, 14, sf::Color(100, 150, 255), column1X, columnsStartY + 30),
        createStyledText("- Meta (verde)", font, 14, sf::Color(100, 255, 100), column1X, columnsStartY + 60),
        createStyledText("- Muro (gris)", font, 14, sf::Color(150, 150, 150), column1X, columnsStartY + 90),
        createStyledText("- Cristal (cyan)", font, 14, sf::Color(0, 255, 255), column1X, columnsStartY + 120)
    };

    // Sección de controles (columna derecha)
    sf::Text controlsTitle = createStyledText("CONTROLES", font, 16, sf::Color(255, 255, 150), column2X, columnsStartY);
    vector<sf::Text> controlTexts = {
        createStyledText("- Flechas: Mover", font, 14, sf::Color(200, 200, 200), column2X, columnsStartY + 30),
        createStyledText("- ENTER: Resolver", font, 14, sf::Color(200, 200, 200), column2X, columnsStartY + 60),
        createStyledText("- R: Reiniciar", font, 14, sf::Color(200, 200, 200), column2X, columnsStartY + 90),
        createStyledText("- Click: Mover", font, 14, sf::Color(200, 200, 200), column2X, columnsStartY + 120)
    };

    // Textos dinámicos para mostrar estado del juego
    sf::Text statusText("", font, 14);         
    sf::Text movesText("Movimientos: 0", font, 14);   
    sf::Text timeText("Tiempo: 0:00", font, 14);      

    
    // Crear sprites para jugador y meta usando círculos
    sf::CircleShape player(cellSize/5), goal(cellSize/5);
    
    // Configurar apariencia del jugador
    player.setFillColor(sf::Color(50, 150, 255));  
    player.setOutlineThickness(2.f);
    player.setOutlineColor(sf::Color::White);      
    
    // Configurar apariencia de la meta
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
    
    // Inicializar sistema de tracking de posición para cristales
    lastX = startX;
    lastY = startY;

    // IMPORTANTE: Asegurar que la celda de inicio esté marcada desde el principio
    grid[startY*W + startX].hasBeenTraversed = true;


    while (window.isOpen()) {
        float dt = moveClock.restart().asSeconds();  
        sf::Event e;  
        
        // Procesar todos los eventos pendientes en la cola del sistema
        while (window.pollEvent(e)) {
            // Evento de cierre de ventana
            if (e.type == sf::Event::Closed) 
                window.close();
            
            // Evento de redimensionamiento de ventana
            if (e.type == sf::Event::Resized) {
                updateViews(window);  // Reajustar vistas para mantener proporciones
            }
                
            if (e.type == sf::Event::MouseButtonPressed) {
                sf::Vector2i mousePos(e.mouseButton.x, e.mouseButton.y);
                
                // Convertir posición del mouse a coordenadas de las diferentes vistas
                sf::Vector2f menuCoords = windowToMenuCoords(mousePos, window);
                sf::Vector2f gameCoords = windowToGameCoords(mousePos, window);
                
                // Verificar clics en botones del menú
                bool buttonClicked = false;
                for (size_t i = 0; i < buttons.size(); i++) {
                    if (buttons[i]->contains(menuCoords.x, menuCoords.y)) {
                        buttonClicked = true;
                        
                        // Switch para manejar diferentes botones
                        switch (i) {
                            case 0: // BOTÓN JUGAR
                                gameState = GameState::Playing;
                                autoMode = false;
                                solved = false;
                                // Asegurar que la celda actual esté marcada al empezar a jugar
                                grid[currentY*W + currentX].hasBeenTraversed = true;
                                gameClock.restart();  // Reiniciar cronómetro
                                break;
                                
                            case 1: // BOTÓN AUTOCOMPLETAR
                                if (gameState == GameState::Menu || gameState == GameState::Playing) {
                                    // Debug info para tracking del autocompletado
                                    cout << "Iniciando autocompletado desde (" << currentX << "," << currentY << ")\n";
                                    cout << "Meta en (" << goalX << "," << goalY << ")\n";
                                    cout << "Grid size: " << W << "x" << H << " = " << grid.size() << " celdas\n";
                                    
                                    gameState = GameState::Playing;
                                    solved = false;
                                    
                                    // Actualizar punto de inicio para BFS
                                    startY = currentY;
                                    startX = currentX;
                                    
                                    bfsSolve();  // Calcular camino óptimo
                                    
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
                                
                            case 2: // BOTÓN REINICIAR
                                resetGame(goal);  // Resetear todo el estado del juego
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
                                
                                // Reinicializar tracking de posición
                                grid[startY*W + startX].hasBeenTraversed = true;
                                lastX = startX;
                                lastY = startY;
                                break;
                        }
                        break;
                    }
                }
                
                // Permitir movimiento por clic solo si no estamos en modo automático
                if (e.mouseButton.button == sf::Mouse::Left && !autoMode && !solved && gameState == GameState::Playing) {
                    if (gameCoords.x >= 0 && gameCoords.y >= 0) {
                        // Convertir coordenadas de píxel a coordenadas de grid
                        int clickX = gameCoords.x / cellSize;
                        int clickY = gameCoords.y / cellSize;
                        
                        // Verificar que el clic esté dentro del grid
                        if (clickX >= 0 && clickX < W && clickY >= 0 && clickY < H) {
                            // Calcular distancia Manhattan para validar movimiento
                            int dx = abs(clickX - currentX);
                            int dy = abs(clickY - currentY);
                            
                            // Solo permitir movimientos a celdas adyacentes (no diagonales)
                            if ((dx == 1 && dy == 0) || (dx == 0 && dy == 1)) {
                                if (tryMovePlayer(clickX, clickY, currentX, currentY, player, goal)) {
                                    moveCount++;
                                    // Verificar victoria
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
                
                // Actualizar estado de hover para todos los botones
                for (auto& button : buttons) {
                    button->updateHover(menuCoords.x, menuCoords.y);
                }
            }
                
            if (e.type == sf::Event::KeyPressed) {
                // Movimiento con teclas de flecha (solo en modo manual)
                if (!autoMode && !solved && gameState == GameState::Playing) {
                    bool moved = false;
                    
                    // Mapear teclas a direcciones de movimiento
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
                    
                    // Si el movimiento fue exitoso, verificar victoria
                    if (moved) {
                        moveCount++;
                        if (currentX == goalX && currentY == goalY) {
                            solved = true;
                            gameState = GameState::Solved;
                        }
                    }
                }
                
                // Tecla ENTER - activar autocompletado
                if (e.key.code == sf::Keyboard::Enter && !autoMode && gameState == GameState::Playing) {
                    startY = currentY;  // Actualizar punto de inicio para BFS
                    startX = currentX;
                    bfsSolve();         // Calcular camino óptimo
                    autoMode = true;    // Activar modo automático
                    step = 0;           // Empezar desde el primer paso
                    currentPos = player.getPosition();
                }
                
                // Tecla R - reiniciar juego
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
        
        // El modo automático ejecuta el camino calculado por BFS paso a paso
        if (autoMode && step < path.size()) {
            auto [y, x] = path[step]; 
            
            // Calcular posición destino en píxeles
            sf::Vector2f nextPos(
                x * cellSize + cellSize/2 - cellSize/5,
                y * cellSize + cellSize/2 - cellSize/5
            );

            // Calcular vector de dirección y distancia para animación suave
            sf::Vector2f direction = nextPos - currentPos;
            float distance = sqrt(direction.x*direction.x + direction.y*direction.y);
            

            // Rod Cutting optimization: dividir la animación en segmentos óptimos
            vector<int> framePrices = {1, 3, 4, 5}; 
            int totalFrames = max(1, (int)(distance / 2.0f));
            
            // Calcular división óptima de frames usando algoritmo Rod Cutting
            vector<int> dp(totalFrames + 1, 0);
            for (int i = 1; i <= totalFrames; i++) {
                for (int j = 1; j <= min(i, (int)framePrices.size()); j++) {
                    dp[i] = max(dp[i], framePrices[j-1] + dp[i-j]);
                }
            }
            
            // Usar velocidad adaptativa basada en la división óptima
            float optimalSpeed = 2.0f + (dp[totalFrames] * 0.1f);
            float animationSpeed = min(optimalSpeed, 6.0f); 


            if (distance > 1.f) {
                direction /= distance;  // Normalizar vector de dirección
                currentPos += direction * animationSpeed * dt * cellSize;
                player.setPosition(currentPos);
            } else {
                // Llegamos al destino - actualizar estado del juego
                
                // Marcar la celda anterior como recorrida (si no es el primer paso)
                if (step > 0) {
                    auto [prevY, prevX] = path[step - 1];
                    grid[prevY*W + prevX].hasBeenTraversed = true;
                }
                
                // Snap a la posición exacta y actualizar coordenadas lógicas
                currentPos = nextPos;
                player.setPosition(currentPos);
                currentX = x;
                currentY = y;

                // Marcar la nueva celda como recorrida
                grid[y*W + x].hasBeenTraversed = true;
                
                // Manejar interacción con cristales
                if (grid[y*W + x].type == CellType::Crystal) {
                    reflectCrystals(currentX, currentY);  // Activar reflexión
                } else {
                    // Actualizar posición anterior sin activar reflejo
                    lastX = currentX;
                    lastY = currentY;
                }

                // Actualizar contadores de juego
                turnCount++;
                turnsSinceLastGoalMove++;
                
                // Verificar eventos temporales
                if (turnsSinceLastGoalMove >= TURNS_TO_MOVE_GOAL) {
                    moveGoal(currentX, currentY, goal);
                }
                
                // Manejar eventos de cambio de mapa
                if (turnCount % TURNS_PER_EVENT == 0) {
                    triggerMapEvent();  // Cambiar estructura del mapa
                    
                    // Recalcular camino debido a cambios en el mapa
                    startY = currentY;
                    startX = currentX;

                    // Preservar posición de la meta durante recálculo
                    int savedGoalX = goalX;
                    int savedGoalY = goalY;

                    bfsSolve();  // Recalcular camino óptimo

                    // Restaurar meta y actualizar visualización
                    goalX = savedGoalX;
                    goalY = savedGoalY;
                    grid[goalY*W + goalX].type = CellType::Goal;
                    verifyGoal(goal);

                    // Reiniciar autocompletado con nuevo camino
                    step = 0;
                    if (path.empty()) {
                        autoMode = false;  // No hay camino posible
                        break;
                    }
                    continue;  // Saltar al siguiente frame
                }

                step++;     
                moveCount++;

                // Verificar condición de victoria
                if (currentX == goalX && currentY == goalY) {
                    solved = true;
                    gameState = GameState::Solved;
                    autoMode = false;
                }
            }
            
            // Verificación periódica de la meta (cada 5 pasos)
            if (step % 5 == 0) {
                verifyGoal(goal);
            }
        }
        
        // Calcular tiempo transcurrido para el cronómetro
        float elapsedTime = gameClock.getElapsedTime().asSeconds();
        int minutes = (int)elapsedTime / 60;
        int seconds = (int)elapsedTime % 60;
        
        sf::Vector2u windowSize = window.getSize();
        
        // Actualizar textos informativos
        movesText.setString("Movimientos: " + to_string(moveCount));
        timeText.setString("Tiempo: " + to_string(minutes) + ":" + (seconds < 10 ? "0" : "") + to_string(seconds));
        
        // Actualizar mensaje de estado basado en el estado actual del juego
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
        
        // Posicionar textos dinámicamente en la parte inferior del menú
        sf::FloatRect statusBounds = statusText.getLocalBounds();
        statusText.setPosition((menuWidth - statusBounds.width) / 2.f, windowSize.y - 60);
        
        sf::FloatRect movesBounds = movesText.getLocalBounds();
        movesText.setPosition((menuWidth - movesBounds.width) / 2.f, windowSize.y - 40);
        
        sf::FloatRect timeBounds = timeText.getLocalBounds();
        timeText.setPosition((menuWidth - timeBounds.width) / 2.f, windowSize.y - 20);

        
        // Limpiar pantalla con color de fondo oscuro
        window.clear(sf::Color(15, 15, 25));
        
        // Cambiar a la vista del juego para renderizar el laberinto
        window.setView(gameView);

        if (W > 0 && H > 0 && !grid.empty()) {
            // Renderizar cada celda del laberinto
            for (int y = 0; y < H; ++y) {
                for (int x = 0; x < W; ++x) {
                    int idx = y * W + x;
                    if (idx >= 0 && idx < grid.size()) { // Verificación de seguridad adicional
                        Cell& cell = grid[idx];
                        
                        // Crear triángulo para esta celda
                        sf::ConvexShape tri = makeTri(x, y);
                        tri.setFillColor(getCellColor(cell.type, x, y, cell.visited, cell.isOnPath, cell.hasBeenTraversed));
                        window.draw(tri);
                        
                        // Efecto especial para cristales - overlay brillante
                        if (cell.type == CellType::Crystal) {
                            sf::ConvexShape crystalTri = makeTri(x, y);
                            crystalTri.setFillColor(sf::Color(255, 255, 255, 50));  // Blanco semi-transparente
                            window.draw(crystalTri);
                        }
                    }
                }
            }
        }

        // Renderizar meta y jugador por encima del laberinto
        verifyGoal(goal);  
        window.draw(goal);
        window.draw(player);
        
        // Cambiar a la vista del menú para renderizar la interfaz
        window.setView(menuView);
        
        // Renderizar elementos del menú en orden (fondo primero, texto encima)
        window.draw(menuBackground);
        window.draw(titleText);
        
        // Renderizar botones interactivos
        for (auto& button : buttons) {
            button->draw(window);
        }
        
        // Renderizar secciones informativas
        window.draw(infoTitle);
        for (auto& text : infoTexts) window.draw(text);
        
        window.draw(controlsTitle);
        for (auto& text : controlTexts) window.draw(text);
        
        // Renderizar información de estado en la parte inferior
        window.draw(statusText);
        window.draw(movesText);
        window.draw(timeText);
        
        // Mostrar todo el contenido renderizado en pantalla
        window.display();
    }

    return 0;  // Programa terminado exitosamente
}