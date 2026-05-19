# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

A Windows console-based **web browser simulator** built in C++14. Simulates tab management, bidirectional history navigation, bookmarks, incognito mode, and session persistence. The project has two coexisting versions for a parallelization study: a sequential baseline with benchmarks and a parallel version that still needs OpenMP implementation.

## Repository Structure

```
proyecto-paralela/
├── version-secuencial/   ← benchmark runner (NOT the interactive app)
├── version-paralela/     ← interactive browser app (OpenMP not yet implemented)
├── Proyecto1Datos/       ← original Visual Studio source files
├── UnitTest/             ← Google Test project
├── PARALELIZACION.md     ← OpenMP implementation guide with ready-to-use code
└── comparar.bat          ← diff script between both versions
```

## CRITICAL: What each version actually contains

**`version-secuencial/`** — The sequential benchmark runner:
- `Source.cpp` replaces `main()` with a timing harness that measures 5 functions and saves results to `reporte_secuencial.txt`
- Adds `Navegador::busquedaMasiva()` — a new function that searches all 1000 sites in the DB
- `ConfigHistorial::getInstancia()` is **thread-safe** (double-checked locking with `std::mutex`)
- Does NOT run the interactive browser UI — it runs benchmarks and exits

**`version-paralela/`** — The interactive browser (base app, OpenMP NOT yet added):
- `Source.cpp` is the standard `main()` → `Controladora::control0()` loop (interactive UI)
- `ConfigHistorial::getInstancia()` is **NOT thread-safe** (no mutex)
- Has 1000-site CSV (`sitiosWeb.csv`) but no `#pragma omp` anywhere in the code
- This is where OpenMP parallelization still needs to be implemented

**The parallel version is not done yet.** `PARALELIZACION.md` contains the ready-to-apply OpenMP code for `busquedaMasiva`, `limpiarSitiosViejos`, and `cargarArchivoSitiosWebCSV`.

## Build

**Compiler:** g++ (MinGW) or MSVC (Visual Studio 2022), C++14, Windows only (`<windows.h>` dependency).

From inside `version-secuencial/` or `version-paralela/`:
```bat
compilar.bat
```

Or manually:
```bat
# Sequential benchmark runner
g++ -std=c++14 -o navegador_secuencial.exe Source.cpp Controladora.cpp Navegador.cpp Interfaz.cpp Historial.cpp ListaPestanias.cpp Pestania.cpp PestaniaIncognito.cpp SitioWeb.cpp Marcador.cpp ConfigHistorial.cpp Excepciones.cpp -I.

# Parallel version (once OpenMP is added)
g++ -std=c++14 -fopenmp -o navegador_paralelo.exe Source.cpp Controladora.cpp Navegador.cpp Interfaz.cpp Historial.cpp ListaPestanias.cpp Pestania.cpp PestaniaIncognito.cpp SitioWeb.cpp Marcador.cpp ConfigHistorial.cpp Excepciones.cpp -I.
```

Orphaned files — do NOT include in compilation: `Sesion.cpp`, `ListPestanias.cpp`, `AdminPestanias.h`.

The Visual Studio solution is `Proyecto1Datos.sln`; the Google Test project is under `UnitTest/`.

## Running Tests

Unit tests use Google Test (linked via `UnitTest/UnitTest.vcxproj`). Build and run from Visual Studio, or via the Developer Command Prompt:

```bat
msbuild UnitTest\UnitTest.vcxproj /p:Configuration=Debug
UnitTest\Debug\UnitTest.exe
```

Tests are in `UnitTest/test.cpp`.

## Comparing both versions

```bat
comparar.bat                  # shows which files differ between both versions
comparar.bat Historial.cpp    # shows line-by-line diff for a specific file
```

## Architecture

MVC pattern:

- **Model**: `SitioWeb`, `Historial`, `Marcador`, `ConfigHistorial` (singleton), `Pestania`/`PestaniaIncognito`/`PestaniaAbstracta`, `ListaPestanias`
- **View**: `Interfaz` (all static methods; Windows Console API for keyboard input via `PeekConsoleInput`)
- **Controller**: `Controladora` (owns `Navegador`; routes keyboard input to `control0()`–`control13()` and submenu handlers)

```
main() → Controladora::control0() [main loop]
    ↓
    Navegador  (central coordinator; owns all state)
    ├── ListaPestanias  →  PestaniaAbstracta
    │                      ├── Pestania  →  Historial  →  std::list<SitioWeb*>
    │                      └── PestaniaIncognito  (single SitioWeb*, no history)
    ├── std::vector<SitioWeb*>  (1000-site CSV database, loaded from sitiosWeb.csv)
    ├── std::list<Marcador*>    (global bookmarks)
    └── ConfigHistorial*        (singleton: maxEntradas, tiempoMaximo)
```

## Key Implementation Details

- **Navigation**: `Historial` uses `std::list<SitioWeb*>` with an iterator (`posicionActual`) for O(1) forward/back. Going back then visiting a new page truncates forward history.
- **Tabs**: `ListaPestanias` uses `std::list<PestaniaAbstracta*>` with an iterator for the active tab.
- **Incognito tabs**: `PestaniaIncognito` holds only `SitioWeb* sitioActual` — no history, no bookmarks. Detected via `getTipo()` returning `"incognito"`.
- **Session persistence**: Binary serialization in `Navegador` (`guardarSesion()`/`cargarSesion()`). File: `sesion.dat`.
- **Site database**: `cargarArchivoSitiosWebCSV()` reads `sitiosWeb.csv` into `std::vector<SitioWeb*> sitios`. The 1000-site version enables meaningful benchmark measurements.
- **busquedaMasiva()**: Added in `version-secuencial/Navegador.cpp` — linear scan over all sites matching keyword in title or URL. This is the primary OpenMP candidate.
- **Keyboard input**: `Interfaz::detectarTecla()` uses `PeekConsoleInput` (non-blocking); returns `'\0'` when no input.

## Parallelization Status

| What | Where to add OpenMP | Status |
|------|--------------------|----|
| `busquedaMasiva()` | `version-paralela/Navegador.cpp` | NOT done — `#pragma omp parallel for` + `omp_set_num_threads` |
| `limpiarSitiosViejos()` | `version-paralela/Historial.cpp` | NOT done |
| `cargarArchivoSitiosWebCSV()` | `version-paralela/Navegador.cpp` | NOT done |
| `ConfigHistorial` singleton | `version-paralela/ConfigHistorial.cpp` | NOT done — needs mutex like in `version-secuencial` |

See `PARALELIZACION.md` for ready-to-paste OpenMP code for each function.

## Known Issues

- Memory leak in `PestaniaIncognito::agregarPaginaWeb()`: allocates a new `SitioWeb` then overwrites `sitioActual` without deleting the old pointer.
- No smart pointers anywhere — all ownership is manual raw pointers.
- `limpiarSitiosViejos()` is called every main loop iteration instead of periodically.
