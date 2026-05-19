@echo off
echo Compilando version secuencial...
g++ -std=c++14 -o navegador_secuencial.exe Source.cpp Controladora.cpp Navegador.cpp Interfaz.cpp Historial.cpp ListaPestanias.cpp Pestania.cpp PestaniaIncognito.cpp SitioWeb.cpp Marcador.cpp ConfigHistorial.cpp Excepciones.cpp -I.
if errorlevel 1 (
    echo ERROR en compilacion
) else (
    echo Compilacion exitosa: navegador_secuencial.exe
    navegador_secuencial.exe
)
