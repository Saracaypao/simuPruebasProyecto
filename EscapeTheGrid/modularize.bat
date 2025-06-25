@echo off
echo =============================
echo 🔧 MODULARIZACIÓN DE CÓDIGO (OPCIONAL)
echo =============================

cd /d "%~dp0"

echo ⚠️  ATENCIÓN: Este script dividirá main.cpp en múltiples archivos
echo    para mejor organización del código.
echo.
echo ¿Quieres continuar? (S/N)
set /p "confirm="

if /I not "%confirm%"=="S" (
    echo ❌ Operación cancelada
    pause
    exit /b 0
)

echo.
echo [1/6] Creando archivos de tipos...

REM Crear Types.hpp
(
echo #pragma once
echo #include ^<SFML/Graphics.hpp^>
echo.
echo // Tipos de celdas del laberinto
echo enum class CellType { 
echo     Empty, Wall, Start, Goal, Crystal
echo };
echo.
echo // Estados del juego
echo enum class GameState { 
echo     Menu, Playing, Solved
echo };
echo.
echo // Estructura que representa cada celda del laberinto
echo struct Cell { 
echo     CellType type;
echo     bool visited = false;
echo     bool isOnPath = false;
echo     bool hasBeenTraversed = false;
echo };
) > src\core\Types.hpp

echo [2/6] Creando Button.hpp y Button.cpp...

REM Crear Button.hpp
(
echo #pragma once
echo #include ^<SFML/Graphics.hpp^>
echo #include ^<string^>
echo.
echo class Button {
echo public:
echo     Button^(float x, float y, float width, float height, 
echo            const std::string^& label, sf::Font^& font, sf::Color color^);
echo     
echo     bool contains^(float x, float y^) const;
echo     void updateHover^(float mouseX, float mouseY^);
echo     void draw^(sf::RenderWindow^& window^);
echo     
echo private:
echo     sf::RectangleShape shape;
echo     sf::Text text;
echo     bool isHovered = false;
echo };
) > src\ui\Button.hpp

REM Crear Button.cpp
(
echo #include "Button.hpp"
echo.
echo Button::Button^(float x, float y, float width, float height, 
echo                const std::string^& label, sf::Font^& font, sf::Color color^) {
echo     shape = sf::RectangleShape^(sf::Vector2f^(width, height^)^);
echo     shape.setPosition^(x, y^);
echo     shape.setFillColor^(color^);
echo     shape.setOutlineThickness^(2.f^);
echo     shape.setOutlineColor^(sf::Color^(255, 255, 255, 100^)^);
echo     
echo     text = sf::Text^(label, font, 16^);
echo     text.setFillColor^(sf::Color::White^);
echo     text.setStyle^(sf::Text::Bold^);
echo     sf::FloatRect textBounds = text.getLocalBounds^(^);
echo     text.setPosition^(
echo         x + ^(width - textBounds.width^) / 2.f,
echo         y + ^(height - textBounds.height^) / 2.f - 2.f
echo     ^);
echo }
echo.
echo bool Button::contains^(float x, float y^) const {
echo     return shape.getGlobalBounds^(^).contains^(x, y^);
echo }
echo.
echo void Button::updateHover^(float mouseX, float mouseY^) {
echo     bool wasHovered = isHovered;
echo     isHovered = contains^(mouseX, mouseY^);
echo     
echo     if ^(isHovered ^&^& !wasHovered^) {
echo         shape.setFillColor^(sf::Color^(
echo             shape.getFillColor^(^).r + 30,
echo             shape.getFillColor^(^).g + 30,
echo             shape.getFillColor^(^).b + 30,
echo             shape.getFillColor^(^).a
echo         ^)^);
echo     } else if ^(!isHovered ^&^& wasHovered^) {
echo         shape.setFillColor^(sf::Color^(
echo             shape.getFillColor^(^).r - 30,
echo             shape.getFillColor^(^).g - 30,
echo             shape.getFillColor^(^).b - 30,
echo             shape.getFillColor^(^).a
echo         ^)^);
echo     }
echo }
echo.
echo void Button::draw^(sf::RenderWindow^& window^) {
echo     window.draw^(shape^);
echo     window.draw^(text^);
echo }
) > src\ui\Button.cpp

echo [3/6] Creando archivos de algoritmos...

REM Crear BFS.hpp
(
echo #pragma once
echo #include "../core/Types.hpp"
echo #include ^<vector^>
echo #include ^<utility^>
echo.
echo class BFS {
echo public:
echo     static std::vector^<std::pair^<int,int^>^> solve^(
echo         const std::vector^<Cell^>^& grid, int W, int H,
echo         int startX, int startY, int goalX, int goalY^);
echo };
) > src\algorithms\BFS.hpp

echo [4/6] Creando documentación mejorada...

REM Crear manual.md
(
echo # Manual de Usuario - Escape The Grid
echo.
echo ## Descripción
echo Escape The Grid es un juego de laberinto donde el jugador debe encontrar
echo la salida evitando obstáculos y utilizando cristales especiales que 
echo modifican el mapa.
echo.
echo ## Características
echo - **Laberintos dinámicos**: El mapa cambia durante el juego
echo - **Cristales mágicos**: Reflejan partes del laberinto
echo - **Meta móvil**: La salida se mueve periódicamente
echo - **Autocompletado**: El juego puede resolverse automáticamente
echo - **Interfaz intuitiva**: Controles simples y menú gráfico
echo.
echo ## Instalación
echo 1. Descargar el juego
echo 2. Ejecutar `EscapeTheGrid.exe`
echo 3. ¡Disfrutar!
echo.
echo ## Desarrollo
echo ### Requisitos
echo - MinGW/GCC con C++17
echo - SFML 2.5.1
echo - Windows 10/11
echo.
echo ### Compilación
echo ```bash
echo scripts\build.bat        # Compilación normal
echo scripts\build_static.bat # Compilación estática
echo ```
) > docs\manual.md

echo [5/6] Creando CMakeLists.txt (opcional)...

REM Crear CMakeLists.txt
(
echo cmake_minimum_required^(VERSION 3.16^)
echo project^(EscapeTheGrid^)
echo.
echo set^(CMAKE_CXX_STANDARD 17^)
echo set^(CMAKE_CXX_STANDARD_REQUIRED ON^)
echo.
echo # Buscar SFML
echo find_package^(sfml-graphics REQUIRED^)
echo find_package^(sfml-window REQUIRED^)
echo find_package^(sfml-system REQUIRED^)
echo.
echo # Archivos fuente
echo file^(GLOB_RECURSE SOURCES "src/*.cpp"^)
echo.
echo # Crear ejecutable
echo add_executable^(EscapeTheGrid ${SOURCES}^)
echo.
echo # Linkear SFML
echo target_link_libraries^(EscapeTheGrid 
echo     sfml-graphics 
echo     sfml-window 
echo     sfml-system^)
echo.
echo # Incluir directorios
echo target_include_directories^(EscapeTheGrid PRIVATE 
echo     "src"
echo     "include"^)
echo.
echo # Copiar assets
echo add_custom_command^(TARGET EscapeTheGrid POST_BUILD
echo     COMMAND ${CMAKE_COMMAND} -E copy_directory
echo     ${CMAKE_SOURCE_DIR}/assets $^<TARGET_FILE_DIR:EscapeTheGrid^>/assets^)
) > CMakeLists.txt

echo [6/6] Actualizando rutas en scripts...

REM Actualizar build_static.bat para nueva estructura
if exist scripts\build_static.bat (
    powershell -Command "(Get-Content scripts\build_static.bat) -replace 'SFML-2.5.1', 'lib\SFML-2.5.1' | Set-Content scripts\build_static.bat"
)

echo.
echo ✅ MODULARIZACIÓN COMPLETADA
echo.
echo 📁 Archivos creados:
echo   📄 src\core\Types.hpp       - Definiciones de tipos
echo   📄 src\ui\Button.hpp/.cpp   - Clase Button modular
echo   📄 src\algorithms\BFS.hpp   - Algoritmo BFS separado
echo   📄 docs\manual.md           - Manual de usuario
echo   📄 CMakeLists.txt           - Build system alternativo
echo.
echo 💡 Para continuar la modularización completa, ejecuta:
echo    1. scripts\reorganize.bat
echo    2. Mueve código restante de main.cpp a módulos
echo.
pause
