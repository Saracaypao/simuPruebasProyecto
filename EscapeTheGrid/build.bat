@echo off
cd /d "%~dp0"
if "%1"=="clean" goto :CLEAN

REM Crear carpeta build si no existe
if not exist build mkdir build

echo =============================
echo 🧱 Compilando y generando en build\
echo =============================

echo [1/4] Compilando...
g++ -c src\*.cpp -I SFML-2.5.1\include -I include -std=c++17
if errorlevel 1 (
  echo ❌ Error durante compilación.
  pause
  exit /b 1
)

echo [2/4] Linkeando...
g++ *.o -o build\EscapeTheGrid.exe ^
 -L SFML-2.5.1\lib ^
 -lsfml-graphics -lsfml-window -lsfml-system ^
 -lopengl32 -lwinmm -lgdi32
if errorlevel 1 (
  echo ❌ Error durante linking.
  pause
  exit /b 1
)

echo [3/4] Copiando DLLs a build\
xcopy /Y SFML-2.5.1\bin\*.dll build\ >nul
if errorlevel 1 (
  echo ⚠️  No se copiaron DLLs (no se encontraron).
) else (
  echo ✅ DLLs copiados correctamente en build\.
)

echo [4/4] Ejecutando desde build\...
cd build
Echo 🎮 Inicio de applicación...
EscapeTheGrid.exe
cd ..

echo 📦 Build completado.
pause
goto :EOF

:CLEAN
echo 🧹 Limpiando carpeta build\
if exist build (
  del /Q build\*.o
  del /Q build\EscapeTheGrid.exe
  del /Q build\*.dll
)
echo ✅ Limpieza completa.
pause
