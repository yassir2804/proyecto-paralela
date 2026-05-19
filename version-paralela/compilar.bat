@echo off
echo Compilando navegador paralelo (OpenMP, 4 hilos)...
g++ -std=c++14 -O2 -fopenmp -o navegador_paralelo.exe Source.cpp Controladora.cpp Navegador.cpp Interfaz.cpp Historial.cpp ListaPestanias.cpp Pestania.cpp PestaniaIncognito.cpp SitioWeb.cpp Marcador.cpp ConfigHistorial.cpp Excepciones.cpp -I.
if errorlevel 1 (
    echo ERROR en compilacion
    pause
) else (
    echo Compilacion exitosa. Iniciando navegador paralelo...
    navegador_paralelo.exe
)
