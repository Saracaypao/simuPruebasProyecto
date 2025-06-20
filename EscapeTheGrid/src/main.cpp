#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <iostream>
#include <cmath>

using namespace std;

enum class CellType { Empty, Wall, Start, Goal, Crystal };
struct Cell { 
    CellType type; 
    bool visited = false; 
    bool isOnPath = false;
    bool hasBeenTraversed = false;
};

enum class GameState { Menu, Playing, Solved };

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

int W, H;
vector<Cell> grid;
int startX, startY, goalX, goalY;
vector<pair<int,int>> path;
GameState gameState = GameState::Menu;

float cellSize = 35.f;
float menuWidth = 300.f;
sf::Vector2f gameOffset(0, 0);
sf::View gameView, menuView;

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
            if (t == "#")      grid[y*W+x] = {CellType::Wall,    false, false, false};
            else if (t == "S") grid[y*W+x] = {CellType::Start,   false, false, false};
            else if (t == "G") grid[y*W+x] = {CellType::Goal,    false, false, false};
            else if (t == "C") grid[y*W+x] = {CellType::Crystal, false, false, false};
            else               grid[y*W+x] = {CellType::Empty,   false, false, false};
        }
    }
    cout << "Laberinto cargado: " << W << "x" << H << endl;
    return true;
}

void createDefaultMaze() {
    W = 12; H = 10;
    startX = 1; startY = 1;
    goalX = 10; goalY = 8;
    
    grid.resize(W * H);
    
    for (int i = 0; i < W * H; i++) {
        grid[i] = {CellType::Empty, false, false, false};
    }
    
    vector<pair<int,int>> walls = {
        {0,0}, {0,1}, {0,2}, {0,3}, {0,4}, {0,5}, {0,6}, {0,7}, {0,8}, {0,9},
        {11,0}, {11,1}, {11,2}, {11,3}, {11,4}, {11,5}, {11,6}, {11,7}, {11,8}, {11,9},
        {1,0}, {2,0}, {3,0}, {4,0}, {5,0}, {6,0}, {7,0}, {8,0}, {9,0}, {10,0},
        {1,9}, {2,9}, {3,9}, {4,9}, {5,9}, {6,9}, {7,9}, {8,9}, {9,9}, {10,9},
        {2,2}, {2,3}, {2,4}, {4,2}, {4,4}, {4,5}, {4,6},
        {6,1}, {6,3}, {6,5}, {6,7}, {8,2}, {8,4}, {8,6}, {8,7},
        {3,7}, {5,7}, {7,7}, {9,3}, {9,5}
    };
    
    for (auto [x,y] : walls) {
        if (x < W && y < H) {
            grid[y*W + x] = {CellType::Wall, false, false, false};
        }
    }
    
    grid[3*W + 6] = {CellType::Crystal, false, false, false};
    grid[6*W + 9] = {CellType::Crystal, false, false, false};
    
    grid[startY*W + startX] = {CellType::Start, false, false, false};
    grid[goalY*W + goalX] = {CellType::Goal, false, false, false};
    
    cout << "Usando laberinto por defecto: " << W << "x" << H << endl;
}

bool inside(int y, int x) {
    return x >= 0 && x < W && y >= 0 && y < H;
}

void bfsSolve() {
    for (auto& cell : grid) {
        cell.visited = false;
        cell.isOnPath = false;
    }
    path.clear();
    
    vector<vector<bool>> vis(H, vector<bool>(W, false));
    vector<vector<pair<int,int>>> parent(H, vector<pair<int,int>>(W, {-1,-1}));
    queue<pair<int,int>> q;
    q.push({startY, startX});
    vis[startY][startX] = true;
    
    int dirs[4][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}};
    bool found = false;

    while (!q.empty()) {
        auto [y,x] = q.front(); q.pop();
        grid[y*W+x].visited = true;
        
        if (y == goalY && x == goalX) { 
            found = true; 
            break; 
        }
        
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

    if (found) {
        for (int cy = goalY, cx = goalX; cy != -1; ) {
            path.push_back({cy,cx});
            grid[cy*W+cx].isOnPath = true;
            auto p = parent[cy][cx];
            cy = p.first; 
            cx = p.second;
        }
        reverse(path.begin(), path.end());
    }
}

bool tryMovePlayer(int newX, int newY, int& currentX, int& currentY, sf::CircleShape& player) {
    if (inside(newY, newX) && grid[newY*W + newX].type != CellType::Wall) {
        currentX = newX;
        currentY = newY;
        sf::Vector2f newPos(newX * cellSize + cellSize/2 - cellSize/5, newY * cellSize + cellSize/2 - cellSize/5);
        player.setPosition(newPos);
        grid[currentY*W + currentX].hasBeenTraversed = true;
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
    
    float left   = px - horizontalStretch/2;
    float right  = px + triWidth + horizontalStretch/2;
    float centerX = px + triWidth / 2.f;
    
    float top = py;
    float bottom = py + triHeight + verticalStretch;

    if (up) {
        tri.setPoint(0, { centerX, top });
        tri.setPoint(1, { left,    bottom });
        tri.setPoint(2, { right,   bottom });
    } else {
        tri.setPoint(0, { centerX, bottom });
        tri.setPoint(1, { left,    top });
        tri.setPoint(2, { right,   top });
    }

    tri.setOutlineThickness(0.5f);
    tri.setOutlineColor(sf::Color(100, 100, 100, 100));
    return tri;
}

sf::Color getCellColor(CellType type, int x, int y, bool visited, bool isOnPath, bool hasBeenTraversed) {
    if (hasBeenTraversed) {
        return sf::Color(192, 192, 192, 200);
    }
    
    if (visited) {
        return sf::Color(150, 150, 255, 150);
    }
    
    switch (type) {
        case CellType::Wall:
            return sf::Color(40, 40, 40);
        case CellType::Crystal:
            return sf::Color(0, 255, 255, 180);
        case CellType::Empty:
        default:
            return ((x + y) % 2 == 0) 
                ? sf::Color(120, 120, 200, 120)
                : sf::Color(100, 100, 180, 120);
    }
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
        cout << "No se encontro maze.txt, creando laberinto por defecto..." << endl;
        createDefaultMaze();
    }

    float initialGameWidth = W * cellSize;
    float initialGameHeight = H * cellSize;
    float initialTotalWidth = initialGameWidth + menuWidth;
    
    sf::RenderWindow window({ (unsigned int)initialTotalWidth, (unsigned int)initialGameHeight }, 
                           "Escape the Grid", 
                           sf::Style::Default);
    window.setFramerateLimit(60);
    
    updateViews(window);
    
    sf::Font font;
    if (!font.loadFromFile("../assets/arial.ttf"))
        font.loadFromFile("assets/arial.ttf");

    sf::RectangleShape menuBackground(sf::Vector2f(menuWidth, 2000));
    menuBackground.setPosition(0, 0);
    menuBackground.setFillColor(sf::Color(25, 25, 35, 240));
    menuBackground.setOutlineThickness(2.f);
    menuBackground.setOutlineColor(sf::Color(100, 100, 150, 150));

    // Texto de bienvenida arriba de todo (centrado)
    sf::Text titleText("ESCAPE THE GRID", font, 24);
    titleText.setFillColor(sf::Color(150, 255, 255));
    sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setPosition((menuWidth - titleBounds.width) / 2.f, 20);
    
    /*sf::Text titleText("Escape the Grid", font, 20);
    titleText.setFillColor(sf::Color(255, 255, 150));
    sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setPosition((menuWidth - titleBounds.width) / 2.f, 50);*/

    // Botones debajo del título
    vector<unique_ptr<Button>> buttons;
    buttons.push_back(make_unique<Button>(30, 100, 240, 40, "JUGAR", font, sf::Color(50, 150, 50, 200)));
    buttons.push_back(make_unique<Button>(30, 150, 240, 40, "AUTOCOMPLETAR", font, sf::Color(150, 100, 50, 200)));
    buttons.push_back(make_unique<Button>(30, 200, 240, 40, "REINICIAR", font, sf::Color(150, 50, 50, 200)));

    // Configuración de columnas para información y controles
    float column1X = 30;          // Columna izquierda (Información)
    float column2X = 160;         // Columna derecha (Controles)
    float columnsStartY = 260;    // Posición vertical para las columnas

    // Información (columna izquierda)
    sf::Text infoTitle = createStyledText("INFORMACION", font, 16, sf::Color(255, 255, 150), column1X, columnsStartY);
    sf::Text playerInfo = createStyledText("- Jugador (azul)", font, 14, sf::Color(100, 150, 255), column1X, columnsStartY + 30);
    sf::Text goalInfo = createStyledText("- Meta (verde)", font, 14, sf::Color(100, 255, 100), column1X, columnsStartY + 60);
    sf::Text wallInfo = createStyledText("- Muro (gris)", font, 14, sf::Color(150, 150, 150), column1X, columnsStartY + 90);
    sf::Text crystalInfo = createStyledText("- Cristal (cyan)", font, 14, sf::Color(0, 255, 255), column1X, columnsStartY + 120);

    // Controles (columna derecha)
    sf::Text controlsTitle = createStyledText("CONTROLES", font, 16, sf::Color(255, 255, 150), column2X, columnsStartY);
    sf::Text arrowKeys = createStyledText("- Flechas: Mover", font, 14, sf::Color(200, 200, 200), column2X, columnsStartY + 30);
    sf::Text enterKey = createStyledText("- ENTER: Resolver", font, 14, sf::Color(200, 200, 200), column2X, columnsStartY + 60);
    sf::Text rKey = createStyledText("- R: Reiniciar", font, 14, sf::Color(200, 200, 200), column2X, columnsStartY + 90);
    sf::Text clickInfo = createStyledText("- Click: Mover", font, 14, sf::Color(200, 200, 200), column2X, columnsStartY + 120);

    // Estado del juego (abajo)
    sf::Text statusText("", font, 14);
    sf::Text movesText("Movimientos: 0", font, 14);
    sf::Text timeText("Tiempo: 0:00", font, 14);

    sf::CircleShape player(cellSize/5), goal(cellSize/5);
    player.setFillColor(sf::Color(50, 150, 255));
    player.setOutlineThickness(2.f);
    player.setOutlineColor(sf::Color::White);
    goal.setFillColor(sf::Color(50, 255, 50));
    goal.setOutlineThickness(2.f);
    goal.setOutlineColor(sf::Color::White);

    player.setPosition(startX * cellSize + cellSize/2 - cellSize/5, startY * cellSize + cellSize/2 - cellSize/5);
    goal.setPosition(goalX * cellSize + cellSize/2 - cellSize/5, goalY * cellSize + cellSize/2 - cellSize/5);

    bool autoMode = false;
    bool solved = false;
    size_t step = 0;
    sf::Vector2f currentPos = player.getPosition();
    sf::Clock gameClock, moveClock;
    int currentX = startX, currentY = startY;
    int moveCount = 0;
    
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
                
                for (size_t i = 0; i < buttons.size(); i++) {
                    if (buttons[i]->contains(menuCoords.x, menuCoords.y)) {
                        switch (i) {
                            case 0: // Boton JUGAR
                                gameState = GameState::Playing;
                                autoMode = false;
                                gameClock.restart();
                                break;
                            case 1: // Boton AUTO
                                if (gameState == GameState::Menu || gameState == GameState::Playing) {
                                    gameState = GameState::Playing;
                                    bfsSolve();
                                    autoMode = true;
                                    step = 0;
                                    currentPos = player.getPosition();
                                    moveCount = 0;
                                    gameClock.restart();
                                }
                                break;
                            case 2: // Boton RESET
                                autoMode = false;
                                solved = false;
                                step = 0;
                                moveCount = 0;
                                currentX = startX;
                                currentY = startY;
                                currentPos = sf::Vector2f(startX * cellSize + cellSize/2 - cellSize/5, startY * cellSize + cellSize/2 - cellSize/5);
                                player.setPosition(currentPos);
                                for (auto& cell : grid) {
                                    cell.visited = false;
                                    cell.isOnPath = false;
                                    cell.hasBeenTraversed = false;
                                }
                                grid[startY*W + startX].hasBeenTraversed = true;
                                path.clear();
                                gameState = GameState::Menu;
                                gameClock.restart();
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
                                if (tryMovePlayer(clickX, clickY, currentX, currentY, player)) {
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
                        moved = tryMovePlayer(currentX, currentY - 1, currentX, currentY, player);
                    }
                    else if (e.key.code == sf::Keyboard::Down) {
                        moved = tryMovePlayer(currentX, currentY + 1, currentX, currentY, player);
                    }
                    else if (e.key.code == sf::Keyboard::Left) {
                        moved = tryMovePlayer(currentX - 1, currentY, currentX, currentY, player);
                    }
                    else if (e.key.code == sf::Keyboard::Right) {
                        moved = tryMovePlayer(currentX + 1, currentY, currentX, currentY, player);
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
                    bfsSolve();
                    autoMode = true;
                    step = 0;
                    currentPos = player.getPosition();
                }
                
                if (e.key.code == sf::Keyboard::R) {
                    autoMode = false;
                    solved = false;
                    step = 0;
                    moveCount = 0;
                    currentX = startX;
                    currentY = startY;
                    currentPos = sf::Vector2f(startX * cellSize + cellSize/2 - cellSize/5, startY * cellSize + cellSize/2 - cellSize/5);
                    player.setPosition(currentPos);
                    for (auto& cell : grid) {
                        cell.visited = false;
                        cell.isOnPath = false;
                        cell.hasBeenTraversed = false;
                    }
                    grid[startY*W + startX].hasBeenTraversed = true;
                    path.clear();
                    gameState = GameState::Menu;
                    gameClock.restart();
                }
            }
        }

        if (autoMode && step < path.size()) {
            auto [y, x] = path[step];
            sf::Vector2f nextPos(x * cellSize + cellSize/2 - cellSize/5, 
                                y * cellSize + cellSize/2 - cellSize/5);
            
            sf::Vector2f direction = nextPos - currentPos;
            float distance = sqrt(direction.x * direction.x + direction.y * direction.y);
            
            float animationSpeed = 5.0f;
            if (distance > 1.0f) {
                direction /= distance;
                currentPos += direction * animationSpeed * dt * cellSize;
                player.setPosition(currentPos);
            } else {
                currentPos = nextPos;
                player.setPosition(currentPos);
                currentX = x;
                currentY = y;
                grid[y*W + x].hasBeenTraversed = true;
                step++;
                moveCount++;
                
                if (step >= path.size()) {
                    solved = true;
                    gameState = GameState::Solved;
                }
            }
        }

        float elapsedTime = gameClock.getElapsedTime().asSeconds();
        int minutes = (int)elapsedTime / 60;
        int seconds = (int)elapsedTime % 60;
        
        sf::Vector2u windowSize = window.getSize();
        float statusY = windowSize.y - 80;
        
        movesText.setString("Movimientos: " + to_string(moveCount));
        timeText.setString("Tiempo: " + to_string(minutes) + ":" + (seconds < 10 ? "0" : "") + to_string(seconds));
        
        switch (gameState) {
            case GameState::Menu:
                statusText.setString("Bienvenido!");
                statusText.setFillColor(sf::Color(150, 255, 150));
                break;
            case GameState::Playing:
                if (autoMode) {
                    statusText.setString("Resolviendo...");
                    statusText.setFillColor(sf::Color(255, 255, 150));
                } else {
                    statusText.setString("Jugando!");
                    statusText.setFillColor(sf::Color(150, 150, 255));
                }
                break;
            case GameState::Solved:
                statusText.setString("Completado!");
                statusText.setFillColor(sf::Color(150, 255, 150));
                break;
        }

        // Posicionar textos de estado abajo (centrados)
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
                
                sf::Color fillColor = getCellColor(cell.type, x, y, cell.visited, cell.isOnPath, cell.hasBeenTraversed);
                tri.setFillColor(fillColor);
                
                window.draw(tri);
                
                if (cell.type == CellType::Crystal) {
                    sf::ConvexShape crystalTri = makeTri(x, y);
                    crystalTri.setFillColor(sf::Color(255, 255, 255, 50));
                    window.draw(crystalTri);
                }
            }
        }

        window.draw(goal);
        window.draw(player);

        window.setView(menuView);
        
        window.draw(menuBackground);
        
        // Bienvenida arriba de todo (centrada)
        //window.draw(welcomeText);
        window.draw(titleText);
        
        // Botones
        for (auto& button : buttons) {
            button->draw(window);
        }
        
        // Columnas de información y controles
        window.draw(infoTitle);
        window.draw(playerInfo);
        window.draw(goalInfo);
        window.draw(wallInfo);
        window.draw(crystalInfo);
        
        window.draw(controlsTitle);
        window.draw(arrowKeys);
        window.draw(enterKey);
        window.draw(rKey);
        window.draw(clickInfo);
        
        // Estado del juego abajo (centrado)
        window.draw(statusText);
        window.draw(movesText);
        window.draw(timeText);
        
        window.display();
    }

    return 0;
}