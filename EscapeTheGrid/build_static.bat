@echo off
cd /d "%~dp0"
if "%1"=="clean" goto :CLEAN

REM Crear carpeta build si no existe
if not exist build mkdir build

echo =============================
echo 🧱 Compilación ESTÁTICA (sin DLLs)
echo =============================

echo [1/3] Compilando...
g++ -c src\*.cpp -I SFML-2.5.1\include -I include -std=c++17 -DSFML_STATIC
if errorlevel 1 (
  echo ❌ Error durante compilación.
  pause
  exit /b 1
)

echo [2/3] Linkeando estáticamente...
g++ *.o -o build\EscapeTheGrid.exe ^
 -L SFML-2.5.1\lib ^
 -lsfml-graphics-s -lsfml-window-s -lsfml-system-s ^
 -lopengl32 -lfreetype -lwinmm -lgdi32 -lws2_32 -static-libgcc -static-libstdc++
if errorlevel 1 (
  echo ❌ Error durante linking estático.
  echo 💡 Asegúrate de tener las librerías estáticas (-s) disponibles.
  pause
  exit /b 1
)

echo [3/3] Copiando assets necesarios...
if exist assets (
  xcopy /E /Y assets build\assets\ >nul
  echo ✅ Assets copiados a build\assets\
) else (
  echo ⚠️  Carpeta assets no encontrada
)

echo 📦 Build estático completado.
echo 🎯 El ejecutable NO necesita DLLs externas.
echo.
echo Probando ejecutable...
cd build
EscapeTheGrid.exe
cd ..

pause
goto :EOF

:CLEAN
echo 🧹 Limpiando carpeta build\
if exist build (
  del /Q build\*.o >nul 2>&1
  del /Q build\EscapeTheGrid.exe >nul 2>&1
  del /Q build\*.dll >nul 2>&1
  rmdir /S /Q build\assets >nul 2>&1
)
echo ✅ Limpieza completa.
pause
