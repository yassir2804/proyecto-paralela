@echo off
echo Compilando version paralela (OpenMP)...
g++ -std=c++14 -fopenmp -o navegador_paralelo.exe Source.cpp Controladora.cpp Navegador.cpp Interfaz.cpp Historial.cpp ListaPestanias.cpp Pestania.cpp PestaniaIncognito.cpp SitioWeb.cpp Marcador.cpp ConfigHistorial.cpp Excepciones.cpp -I.
if errorlevel 1 (
    echo ERROR en compilacion
) else (
    echo Compilacion exitosa: navegador_paralelo.exe
    navegador_paralelo.exe
)
