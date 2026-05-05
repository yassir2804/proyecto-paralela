@echo off
echo ============================================
echo  Compilando benchmark de paralelizacion...
echo ============================================

g++ -std=c++14 -O2 -o benchmark.exe ^
    benchmark.cpp ^
    Historial.cpp ^
    SitioWeb.cpp ^
    ConfigHistorial.cpp ^
    Excepciones.cpp ^
    -I.

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Compilacion fallida.
    pause
    exit /b 1
)

echo [OK] Compilacion exitosa.
echo.
echo ============================================
echo  Ejecutando benchmark...
echo ============================================
echo.

benchmark.exe

pause
