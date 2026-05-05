@echo off
chcp 65001 >nul
title Simulador de Navegador Web - Compilacion y Ejecucion
echo ============================================
echo   Compilando Simulador de Navegador Web
echo ============================================
echo.

cd /d "%~dp0Proyecto1Datos"

echo [1/2] Compilando con g++...
echo.

g++ -std=c++14 -o navegador.exe ^
    Source.cpp ^
    Controladora.cpp ^
    Navegador.cpp ^
    Interfaz.cpp ^
    Historial.cpp ^
    ListaPestanias.cpp ^
    Pestania.cpp ^
    PestaniaIncognito.cpp ^
    SitioWeb.cpp ^
    Marcador.cpp ^
    ConfigHistorial.cpp ^
    Excepciones.cpp ^
    -I. 2>&1

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] La compilacion fallo. Revisa los errores arriba.
    echo.
    pause
    exit /b 1
)

echo [2/2] Compilacion exitosa!
echo.
echo ============================================
echo   Ejecutando navegador...
echo ============================================
echo.

navegador.exe

pause
