@echo off
echo Compilando benchmark paralelo (OpenMP, 4 hilos)...
g++ -std=c++14 -O2 -fopenmp -o bench_paralelo.exe benchmark_paralelo.cpp Controladora.cpp Navegador.cpp Interfaz.cpp Historial.cpp ListaPestanias.cpp Pestania.cpp PestaniaIncognito.cpp SitioWeb.cpp Marcador.cpp ConfigHistorial.cpp Excepciones.cpp -I.
if errorlevel 1 (
    echo ERROR en compilacion
    pause
) else (
    echo Compilacion exitosa. Ejecutando benchmark paralelo...
    bench_paralelo.exe
    pause
)
