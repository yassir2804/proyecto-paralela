# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

A Windows console-based **web browser simulator** built in C++14. Simulates tab management, bidirectional history navigation, bookmarks, incognito mode, and session persistence. The project is being extended for parallelization (concurrent operations on data structures).

## Build

**Compiler:** g++ (MinGW) or MSVC (Visual Studio 2022), C++14, Windows only (`<windows.h>` dependency).

```bat
g++ -std=c++14 -o navegador.exe Source.cpp Controladora.cpp Navegador.cpp Interfaz.cpp Historial.cpp ListaPestanias.cpp Pestania.cpp PestaniaIncognito.cpp SitioWeb.cpp Marcador.cpp ConfigHistorial.cpp Excepciones.cpp -I.
```

Or run the provided batch script:
```bat
compilar_y_ejecutar.bat
```

For OpenMP parallel builds, add `-fopenmp` to the g++ command.

Orphaned files — do NOT include in compilation: `Sesion.cpp`, `ListPestanias.cpp`, `AdminPestanias.h`.

The Visual Studio solution is `Proyecto1Datos.sln`; the Google Test project is under `UnitTest/`.

## Running Tests

Unit tests use Google Test (linked via `UnitTest/UnitTest.vcxproj`). Build and run from Visual Studio, or via the Developer Command Prompt:

```bat
msbuild UnitTest\UnitTest.vcxproj /p:Configuration=Debug
UnitTest\Debug\UnitTest.exe
```

Tests are in `UnitTest/test.cpp`. There is also a manual functional test plan (CP-01 through CP-20) documented separately covering all UI flows.

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
    ├── std::vector<SitioWeb*>  (16-site CSV database, loaded from sitios.csv)
    ├── std::list<Marcador*>    (global bookmarks)
    └── ConfigHistorial*        (singleton: maxEntradas, tiempoMaximo)
```

## Key Implementation Details

- **Navigation**: `Historial` uses `std::list<SitioWeb*>` with an iterator (`posicionActual`) for O(1) forward/back. Going back then visiting a new page truncates forward history (erases elements after `posicionActual`).
- **Tabs**: `ListaPestanias` uses `std::list<PestaniaAbstracta*>` with an iterator for the active tab. Tab-local history is isolated per tab.
- **Incognito tabs**: `PestaniaIncognito` holds only `SitioWeb* sitioActual` — no history, no bookmarks. Type is detected via `getTipo()` returning `"incognito"`.
- **Session persistence**: Binary serialization in `Navegador` (`guardarSesion()`/`cargarSesion()`). Saves tab count, each tab's type and history entries (URL + title), bookmarks, and config. File: `sesion.dat`.
- **Site database**: `cargarSitiosDesdeCSV()` reads `sitios.csv` into `std::vector<SitioWeb*> sitiosDisponibles` (16 entries). Searches scan this vector linearly.
- **History cleanup**: `limpiarSitiosViejos()` compares `std::chrono` timestamps against `ConfigHistorial::tiempoMaximo` — currently called every loop iteration (performance issue).
- **Keyboard input**: `Interfaz::detectarTecla()` uses `PeekConsoleInput` (non-blocking) to poll for key presses; returns `'\0'` when no input is available.

## Known Issues (relevant for parallelization work)

- `ConfigHistorial::getInstancia()` is not thread-safe (no mutex on lazy initialization — classic double-checked locking problem).
- `limpiarSitiosViejos()` runs every loop iteration instead of periodically — candidate for a background thread with a sleep interval.
- All searches (`buscarPaginaWeb`, `busquedaPalabraClave`, filter methods) are O(n) linear scans — candidates for parallel execution with OpenMP or `std::async`.
- Memory leak in `PestaniaIncognito::agregarPaginaWeb()`: allocates a new `SitioWeb` then immediately overwrites `sitioActual`, losing the old pointer without deleting it.
- `Navegador.cpp` is missing `#include <algorithm>` (compiles in MSVC but fails in strict g++).
- `Navegador::buscarPaginaWeb()` has unreachable `return nullptr` after a `throw` — harmless but indicates incomplete error handling design.
- No smart pointers anywhere — all ownership is manual raw pointers with no RAII. Adding threads requires careful attention to object lifetimes.
