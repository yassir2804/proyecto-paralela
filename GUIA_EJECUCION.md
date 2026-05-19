# Guía de ejecución del proyecto

Simulador de navegador web con análisis de paralelización usando OpenMP.

## Requisito previo

Necesita tener **g++ (MinGW-w64)** instalado y disponible en el PATH de Windows.

**Verificar instalación:**
```bat
g++ --version
```

Si el comando no se reconoce, instale MinGW-w64 desde https://winlibs.com (descargue la versión que incluya OpenMP/POSIX threads).

**Verificar soporte OpenMP:**
```bat
echo #include^<omp.h^> > test.cpp && echo int main(){return 0;} >> test.cpp && g++ -fopenmp test.cpp && echo OpenMP OK && del test.cpp a.exe
```

Si imprime `OpenMP OK`, el entorno está completo para ambas versiones.

---

## Estructura del proyecto

```
proyecto-paralela/
├── version-secuencial/   ← benchmark secuencial (mide tiempos, genera reporte)
└── version-paralela/     ← benchmark paralelo con OpenMP (4 hilos)
```

Cada versión tiene dos modos de ejecución: **app interactiva** y **benchmark**.

---

## Opción 1 — App interactiva del navegador

Abre una terminal (CMD o PowerShell) dentro de la carpeta correspondiente y ejecuta:

```bat
cd version-secuencial
compilar.bat
```

```bat
cd version-paralela
compilar.bat
```

Compila automáticamente y lanza la aplicación. El navegador carga una base de datos de **1000 sitios web** desde `sitiosWeb.csv`.

---

## Opción 2 — Benchmarks de rendimiento

Los benchmarks miden el tiempo de ejecución de las funciones principales y guardan los resultados en un archivo de reporte.

**Secuencial:**
```bat
cd version-secuencial
benchmark.bat
```
Genera `reporte_secuencial.txt` con los tiempos medidos.

**Paralelo:**
```bat
cd version-paralela
benchmark.bat
```
Genera `reporte_paralelo.txt` con los tiempos usando 4 hilos OpenMP.

---

## Funciones medidas en los benchmarks

| Función | Descripción |
|---|---|
| `cargarArchivoSitiosWebCSV()` | Carga los 1000 sitios desde el CSV |
| `busquedaMasiva()` | Búsqueda lineal sobre los 1000 sitios |
| `limpiarSitiosViejos()` | Limpieza de historial por tiempo |
| `guardarSesion()` | Serialización binaria de la sesión |
| `cargarSesion()` | Deserialización de la sesión |

---

## Comparar resultados

Abra ambos archivos de reporte para comparar los tiempos:

- `version-secuencial/reporte_secuencial.txt`
- `version-paralela/reporte_paralelo.txt`
