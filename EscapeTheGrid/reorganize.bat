@echo off
echo =============================
echo 📁 REORGANIZANDO ESTRUCTURA DEL PROYECTO
echo =============================

cd /d "%~dp0"

echo [1/8] Creando estructura de carpetas...

REM Crear nuevas carpetas
mkdir scripts 2>nul
mkdir src\core 2>nul
mkdir src\entities 2>nul
mkdir src\maze 2>nul
mkdir src\ui 2>nul
mkdir src\algorithms 2>nul
mkdir include 2>nul
mkdir assets\fonts 2>nul
mkdir assets\textures 2>nul
mkdir assets\levels 2>nul
mkdir lib 2>nul
mkdir docs 2>nul
mkdir docs\screenshots 2>nul

echo [2/8] Moviendo scripts de build...
move build.bat scripts\ 2>nul
move build_static.bat scripts\ 2>nul
move build_static_alt.bat scripts\ 2>nul
move create_package.bat scripts\ 2>nul
move test_static.bat scripts\ 2>nul
move remove_dlls.bat scripts\ 2>nul

echo [3/8] Reorganizando assets...
move assets\ARIAL.TTF assets\fonts\arial.ttf 2>nul
move assets\arial.ttf assets\fonts\ 2>nul
move assets\maze.txt assets\levels\ 2>nul

echo [4/8] Moviendo SFML a lib...
if exist SFML-2.5.1 (
    move SFML-2.5.1 lib\ 2>nul
    echo ✅ SFML movido a lib\
)

echo [5/8] Creando archivos de documentación...

REM Crear README.md
echo # Escape The Grid > README.md
echo. >> README.md
echo Un juego de laberinto desarrollado en C++ con SFML. >> README.md
echo. >> README.md
echo ## Compilación >> README.md
echo. >> README.md
echo ```bash >> README.md
echo # Compilación normal >> README.md
echo scripts\build.bat >> README.md
echo. >> README.md
echo # Compilación estática (sin DLLs) >> README.md
echo scripts\build_static.bat >> README.md
echo ``` >> README.md
echo. >> README.md
echo ## Controles >> README.md
echo. >> README.md
echo - **Flechas**: Mover jugador >> README.md
echo - **ENTER**: Activar autocompletado >> README.md
echo - **R**: Reiniciar juego >> README.md
echo - **Click**: Mover a celda adyacente >> README.md

REM Crear .gitignore
echo # Build artifacts > .gitignore
echo build/ >> .gitignore
echo dist/ >> .gitignore
echo *.o >> .gitignore
echo *.exe >> .gitignore
echo. >> .gitignore
echo # Visual Studio >> .gitignore
echo .vs/ >> .gitignore
echo *.vcxproj >> .gitignore
echo *.sln >> .gitignore
echo. >> .gitignore
echo # Temporary files >> .gitignore
echo *.tmp >> .gitignore
echo *.log >> .gitignore

echo [6/8] Actualizando scripts para nueva estructura...

REM Actualizar script de build
(
echo @echo off
echo cd /d "%%~dp0\.."
echo if "%%1"=="clean" goto :CLEAN
echo.
echo if not exist build mkdir build
echo.
echo echo =============================
echo echo 🧱 Compilando EscapeTheGrid
echo echo =============================
echo.
echo echo [1/4] Compilando...
echo g++ -c src\*.cpp -I lib\SFML-2.5.1\include -I include -std=c++17
echo if errorlevel 1 ^(
echo   echo ❌ Error durante compilación.
echo   pause
echo   exit /b 1
echo ^)
echo.
echo echo [2/4] Linkeando...
echo g++ *.o -o build\EscapeTheGrid.exe ^
echo  -L lib\SFML-2.5.1\lib ^
echo  -lsfml-graphics -lsfml-window -lsfml-system ^
echo  -lopengl32 -lwinmm -lgdi32
echo if errorlevel 1 ^(
echo   echo ❌ Error durante linking.
echo   pause
echo   exit /b 1
echo ^)
echo.
echo echo [3/4] Copiando DLLs...
echo xcopy /Y lib\SFML-2.5.1\bin\*.dll build\ ^>nul
echo.
echo echo [4/4] Copiando assets...
echo xcopy /E /Y assets build\assets\ ^>nul
echo.
echo echo 📦 Build completado.
echo cd build
echo EscapeTheGrid.exe
echo cd ..
echo pause
echo goto :EOF
echo.
echo :CLEAN
echo echo 🧹 Limpiando...
echo if exist build ^(
echo   del /Q build\*.o ^>nul 2^>^&1
echo   del /Q build\EscapeTheGrid.exe ^>nul 2^>^&1
echo   del /Q build\*.dll ^>nul 2^>^&1
echo   rmdir /S /Q build\assets ^>nul 2^>^&1
echo ^)
echo echo ✅ Limpieza completa.
echo pause
) > scripts\build.bat

echo [7/8] Creando script de setup para desarrollo...

REM Crear setup script
(
echo @echo off
echo echo =============================
echo echo 🛠️  CONFIGURACIÓN DE DESARROLLO
echo echo =============================
echo.
echo cd /d "%%~dp0\.."
echo.
echo echo [1/3] Verificando estructura...
echo if not exist "src\main.cpp" ^(
echo     echo ❌ main.cpp no encontrado
echo     pause
echo     exit /b 1
echo ^)
echo.
echo if not exist "lib\SFML-2.5.1" ^(
echo     echo ❌ SFML no encontrado en lib\
echo     echo 💡 Mueve la carpeta SFML-2.5.1 a lib\
echo     pause
echo     exit /b 1
echo ^)
echo.
echo echo [2/3] Verificando assets...
echo if not exist "assets\levels\maze.txt" ^(
echo     echo ⚠️  maze.txt no encontrado en assets\levels\
echo ^)
echo.
echo if not exist "assets\fonts\arial.ttf" ^(
echo     echo ⚠️  arial.ttf no encontrado en assets\fonts\
echo ^)
echo.
echo echo [3/3] Compilando proyecto de prueba...
echo call scripts\build.bat
echo.
echo echo ✅ Setup completado
echo pause
) > scripts\setup.bat

echo [8/8] Limpiando archivos temporales...
del *.o 2>nul

echo.
echo ✅ REORGANIZACIÓN COMPLETADA
echo.
echo 📁 Nueva estructura creada:
echo   📂 scripts/     - Scripts de build y utilidades
echo   📂 src/         - Código fuente organizado
echo   📂 assets/      - Recursos organizados por tipo
echo   📂 lib/         - Librerías externas (SFML)
echo   📂 docs/        - Documentación
echo.
echo 🎯 Para compilar ahora usa: scripts\build.bat
echo 🎯 Para setup inicial: scripts\setup.bat
echo.
pause
