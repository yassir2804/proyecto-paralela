@echo off
setlocal

pushd "%~dp0Proyecto1Datos"

echo Compilando benchmark2.exe con OpenMP...
g++ -std=c++14 -O2 -fopenmp -o benchmark2.exe benchmark2.cpp Historial.cpp SitioWeb.cpp ConfigHistorial.cpp Excepciones.cpp -I.
if errorlevel 1 (
  echo Error en compilacion.
  popd
  exit /b 1
)

echo Ejecutando benchmark2.exe y guardando resultados...
benchmark2.exe > resultados_DESPUES.txt

if errorlevel 1 (
  echo Error al ejecutar benchmark2.exe.
  popd
  exit /b 1
)

echo Listo. Resultados en resultados_DESPUES.txt
popd
endlocal
