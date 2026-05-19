@echo off
echo Compilando benchmark secuencial...
g++ -std=c++14 -O2 -o bench_secuencial.exe benchmark_secuencial.cpp Controladora.cpp Navegador.cpp Interfaz.cpp Historial.cpp ListaPestanias.cpp Pestania.cpp PestaniaIncognito.cpp SitioWeb.cpp Marcador.cpp ConfigHistorial.cpp Excepciones.cpp -I.
if errorlevel 1 (
    echo ERROR en compilacion
    pause
) else (
    echo Compilacion exitosa. Ejecutando benchmark...
    bench_secuencial.exe
    pause
)
