@echo off
setlocal

set SEC=version-secuencial
set PAR=version-paralela

set ARCHIVOS=ConfigHistorial.cpp ConfigHistorial.h Controladora.cpp Controladora.h Historial.cpp Historial.h Interfaz.cpp Interfaz.h ListaPestanias.cpp ListaPestanias.h Marcador.cpp Marcador.h Navegador.cpp Navegador.h Pestania.cpp Pestania.h PestaniaAbstracta.h PestaniaIncognito.cpp PestaniaIncognito.h SitioWeb.cpp SitioWeb.h Source.cpp AdminPestanias.h Excepciones.cpp Excepciones.h

echo ============================================================
echo   COMPARACION: version-secuencial vs version-paralela
echo ============================================================
echo.

if "%1"=="" goto resumen
goto archivo

:resumen
echo Archivos con diferencias:
echo.
for %%f in (%ARCHIVOS%) do (
    fc /b "%SEC%\%%f" "%PAR%\%%f" >nul 2>&1
    if errorlevel 1 (
        echo   [DIFERENTE]  %%f
    ) else (
        echo   [igual]      %%f
    )
)
echo.
echo Uso: comparar.bat [nombre_archivo]
echo Ejemplo: comparar.bat Historial.cpp
goto fin

:archivo
echo Diferencias en %1:
echo.
if not exist "%SEC%\%1" (
    echo ERROR: %SEC%\%1 no existe
    goto fin
)
if not exist "%PAR%\%1" (
    echo ERROR: %PAR%\%1 no existe
    goto fin
)
fc "%SEC%\%1" "%PAR%\%1"

:fin
endlocal
