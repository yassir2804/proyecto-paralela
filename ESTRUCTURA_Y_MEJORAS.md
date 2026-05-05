# Estructura del Proyecto y Puntos de Mejora

## 1. Estructura Actual

```
Proyecto1Datos/
|-- Proyecto1Datos.sln
|-- .gitignore
|-- .gitattributes
|
|-- Proyecto1Datos/              <-- TODO el codigo fuente mezclado en una sola carpeta
|   |-- Source.cpp
|   |-- Controladora.h / .cpp
|   |-- Navegador.h / .cpp
|   |-- Interfaz.h / .cpp
|   |-- Historial.h / .cpp
|   |-- ListaPestanias.h / .cpp
|   |-- Pestania.h / .cpp
|   |-- PestaniaAbstracta.h
|   |-- PestaniaIncognito.h / .cpp
|   |-- SitioWeb.h / .cpp
|   |-- Marcador.h / .cpp
|   |-- ConfigHistorial.h / .cpp
|   |-- Excepciones.h / .cpp
|   |-- sitiosWeb.csv
|   |-- Sesion.cpp              <-- ARCHIVO HUERFANO (sin header, sin uso)
|   |-- AdminPestanias.h        <-- ARCHIVO VACIO
|   |-- ListPestanias.cpp       <-- ARCHIVO VACIO (duplicado de ListaPestanias.cpp)
|   |-- Proyecto1Datos.vcxproj
|   |-- Proyecto1Datos.vcxproj.filters
|
|-- UnitTest/
    |-- test.cpp
    |-- UnitTest.vcxproj
    |-- packages.config
    |-- archivos de prueba (.bin, .dat)
```

**Problemas de la estructura actual:**
- Todos los archivos fuente (.h y .cpp) estan en una sola carpeta plana
- Existen 3 archivos huerfanos que no se usan (Sesion.cpp, AdminPestanias.h, ListPestanias.cpp)
- No hay separacion entre modelos, vistas y controladores
- No hay carpeta para datos/recursos

---

## 2. Estructura Sugerida (Reorganizacion)

```
Proyecto1Datos/
|-- Proyecto1Datos.sln
|-- .gitignore
|-- .gitattributes
|-- DESCRIPCION_PROYECTO.md
|-- ESTRUCTURA_Y_MEJORAS.md
|
|-- Proyecto1Datos/
|   |-- Proyecto1Datos.vcxproj
|   |-- Proyecto1Datos.vcxproj.filters
|   |
|   |-- main/
|   |   |-- Source.cpp                    # Punto de entrada
|   |
|   |-- modelo/                           # Capa de datos
|   |   |-- SitioWeb.h / .cpp
|   |   |-- Historial.h / .cpp
|   |   |-- Marcador.h / .cpp
|   |   |-- ConfigHistorial.h / .cpp
|   |
|   |-- vista/                            # Capa de presentacion
|   |   |-- Interfaz.h / .cpp
|   |
|   |-- controlador/                      # Capa de control
|   |   |-- Controladora.h / .cpp
|   |   |-- Navegador.h / .cpp
|   |
|   |-- pestanias/                        # Subsistema de pestanias
|   |   |-- PestaniaAbstracta.h
|   |   |-- Pestania.h / .cpp
|   |   |-- PestaniaIncognito.h / .cpp
|   |   |-- ListaPestanias.h / .cpp
|   |
|   |-- excepciones/                      # Manejo de errores
|   |   |-- Excepciones.h / .cpp
|   |
|   |-- datos/                            # Recursos y datos
|       |-- sitiosWeb.csv
|
|-- UnitTest/
    |-- test.cpp
    |-- UnitTest.vcxproj
    |-- packages.config
```

**Nota:** Reorganizar en carpetas requiere actualizar el `.vcxproj` y los `#include`.
Para el proyecto de Programacion Paralela, puede que sea mas practico mantener la estructura plana
y enfocarse en la paralelizacion. La reorganizacion es una mejora de calidad pero no es el objetivo del curso.

---

## 3. Puntos de Mejora del Codigo

### 3.1 ERRORES / BUGS

#### BUG 1: Falta `#include <algorithm>` en Navegador.cpp
- **Archivo:** `Navegador.cpp`
- **Problema:** Usa `std::find_if` sin incluir `<algorithm>`. Compila en MSVC porque los headers de VS lo incluyen indirectamente, pero **falla con g++/MinGW**.
- **Solucion:** Agregar `#include <algorithm>` al inicio de Navegador.cpp

#### BUG 2: Memory leak en PestaniaIncognito::agregarPaginaWeb()
- **Archivo:** `PestaniaIncognito.cpp`, lineas 88-96
- **Problema:** Si `sitio` es no-nulo, se hace `delete sitio` y luego `new SitioWeb(*)` dos veces:
  ```cpp
  void PestaniaIncognito::agregarPaginaWeb(SitioWeb* sitioWeb) {
      if (sitio) {
          delete sitio;
          this->sitio = new SitioWeb(*sitioWeb);  // se asigna aqui
      }
      this->sitio = new SitioWeb(*sitioWeb);      // y se sobreescribe aqui (leak!)
  }
  ```
  La segunda asignacion fuera del `if` crea un nuevo objeto sin borrar el anterior.
- **Solucion:** Agregar `else` o un `return` dentro del `if`.

#### BUG 3: Codigo inalcanzable en Navegador::buscarPaginaWeb()
- **Archivo:** `Navegador.cpp`, lineas 226-228
- **Problema:**
  ```cpp
  throw ExcepcionGenerica("404 - Not Found");
  return nullptr;  // Nunca se ejecuta
  ```
- **Solucion:** Eliminar el `return nullptr` muerto.

#### BUG 4: Singleton ConfigHistorial no es thread-safe
- **Archivo:** `ConfigHistorial.cpp`, lineas 22-30
- **Problema:** `getInstancia()` tiene un race condition clasico. Si dos hilos llaman simultaneamente, ambos pueden ver `instancia == nullptr` y crear dos instancias.
- **Solucion (para version paralela):** Usar `std::call_once` con `std::once_flag`, o inicializacion estatica local (Meyers' Singleton).

### 3.2 PROBLEMAS DE DISENO

#### DISENO 1: Uso de punteros crudos (raw pointers) sin smart pointers
- **Problema:** Todo el proyecto usa `new`/`delete` manual. Esto genera riesgo de memory leaks y hace el manejo de memoria propenso a errores.
- **Donde:** Todas las clases (`Navegador`, `Historial`, `ListPestanias`, `Marcador`, `Pestania`)
- **Solucion ideal:** Usar `std::unique_ptr<>` o `std::shared_ptr<>` de C++11/14.

#### DISENO 2: Archivos huerfanos
- **Archivos:** `Sesion.cpp` (sin header y con `#include "Sesion.h"` que no existe), `AdminPestanias.h` (vacio), `ListPestanias.cpp` (vacio, hay un `ListaPestanias.cpp` con el codigo real)
- **Solucion:** Eliminar estos archivos del proyecto.

#### DISENO 3: `Interfaz` con metodos exclusivamente estaticos
- **Problema:** La clase `Interfaz` no tiene estado ni se instancia nunca. Es esencialmente un namespace con funciones.
- **Impacto:** Menor, pero no es idiomatico C++.

#### DISENO 4: Nombres de metodos poco descriptivos en `Controladora`
- **Problema:** `control0()`, `control1()`, ..., `control13()`, `control8_1()`, etc. no indican que hacen.
- **Solucion:** Renombrar a nombres descriptivos como `menuPrincipal()`, `navegarASitioWeb()`, `gestionarMarcadores()`, etc.

#### DISENO 5: `getMarcadoresGuardados()` retorna copia entera de la lista
- **Archivo:** `Navegador.cpp`, linea 43
- **Problema:** `return marcadoresGuardados;` copia toda la lista cada vez que se llama.
- **Solucion:** Retornar `const std::list<Marcador*>&` (referencia constante).

#### DISENO 6: Parametros por valor en lugar de por referencia constante
- **Archivos:** Varios - `SitioWeb::setUrl(std::string url)` en lugar de `const std::string& url`
- **Impacto:** Copias innecesarias de strings.

### 3.3 PROBLEMAS DE PORTABILIDAD

#### PORT 1: Dependencia de Windows API
- **Archivos:** `Interfaz.cpp`, `Controladora.cpp`
- **Problema:** `<windows.h>`, `GetStdHandle`, `PeekConsoleInput`, `FlushConsoleInputBuffer`, `Sleep()`, `system("cls")`, `system("pause")` son exclusivos de Windows.
- **Impacto:** El programa no compila en Linux/macOS.
- **Nota:** Para el proyecto de paralelizacion esto no es un problema si se usa Windows.

#### PORT 2: `#pragma once` en archivo .cpp
- **Archivo:** `Interfaz.cpp`, linea 2
- **Problema:** `#pragma once` solo tiene sentido en headers (.h), no en archivos de implementacion.

### 3.4 PROBLEMAS DE RENDIMIENTO (RELEVANTES PARA PARALELIZACION)

#### REND 1: Busquedas lineales O(n) en todas las colecciones
- **Archivos:** `Navegador::buscarPaginaWeb()`, `Navegador::buscarMarcadorPorSitio()`, `Historial::busquedaPalabraClave()`, `Historial::eliminarSitioSiExiste()`
- **Problema:** Todas las busquedas recorren la coleccion completa secuencialmente.
- **Oportunidad de paralelizacion:** Dividir la busqueda entre hilos con OpenMP.
- **Alternativa sin hilos:** Usar `std::unordered_map` para busquedas O(1) por URL.

#### REND 2: Limpieza de sitios viejos se ejecuta en CADA iteracion del bucle principal
- **Archivo:** `Interfaz.cpp`, linea 90
- **Problema:** `limpiarSitiosViejos()` recorre TODO el historial de TODAS las pestanias en cada ciclo del loop de deteccion de teclas. Esto es extremadamente ineficiente.
- **Solucion:** Ejecutar la limpieza periodicamente (cada N segundos) en lugar de en cada iteracion, idealmente en un hilo separado.

#### REND 3: Conversion a minusculas repetida en busqueda
- **Archivo:** `Historial.cpp`, lineas 169-189
- **Problema:** En `busquedaPalabraClave()`, se convierte el titulo de cada sitio a minusculas dentro del loop. Si hay N sitios, es O(N * longitud_titulo).
- **Solucion:** Almacenar una version en minusculas precalculada, o paralelizar la busqueda.

#### REND 4: Carga de CSV secuencial
- **Archivo:** `Navegador.cpp`, lineas 313-338
- **Problema:** Lee y parsea cada linea del CSV secuencialmente.
- **Oportunidad:** Con un CSV grande (miles de sitios), se puede leer todas las lineas y parsear en paralelo.

#### REND 5: Serialization secuencial
- **Archivos:** Todos los metodos `guardarArchivo*` y `cargarArchivo*`
- **Problema:** La exportacion/importacion de sesion serializa todo secuencialmente.
- **Oportunidad:** Serializar marcadores, pestanias y config en hilos separados.

### 3.5 CALIDAD DE CODIGO

#### CAL 1: Sin uso de `const` correctness consistente
- Algunos metodos que no modifican estado no estan marcados como `const`
- Parametros que deberian ser `const&` se pasan por valor

#### CAL 2: `system("pause")` y `system("cls")`
- Llamadas al sistema que son lentas y no portables
- Alternativa: Implementar limpieza de consola con secuencias ANSI o Windows API directa

#### CAL 3: Inconsistencia en nombres de archivos
- `ListaPestanias.h/cpp` vs `ListPestanias.cpp` (vacio)
- Mezcla de espanol e ingles en nombres de metodos

---

## 4. Resumen de Prioridades

### Para el proyecto de Programacion Paralela:
| Prioridad | Mejora | Razon |
|-----------|--------|-------|
| CRITICA | Corregir BUG 4 (Singleton thread-safe) | Causara race conditions al paralelizar |
| ALTA | Escalar datos (CSV con miles de sitios) | Sin datos grandes, el speedup no sera medible |
| ALTA | Paralelizar busquedas y limpieza | Son las operaciones mas costosas |
| MEDIA | Corregir BUG 1 (include algorithm) | Necesario para compilar con g++ |
| MEDIA | Mover limpieza a hilo separado (REND 2) | Mejora la responsividad del programa |
| BAJA | Corregir BUG 2, BUG 3 | Bugs reales pero no afectan la paralelizacion |
| BAJA | Reorganizar carpetas | Mejora organica pero no esencial |
