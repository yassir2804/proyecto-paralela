# PRESENTACIÓN — DIAPOSITIVAS PARA CANVA
> Cada sección `---SLIDE---` es una diapositiva independiente.

---SLIDE 1 — PORTADA---

# Paralelización de un Simulador de Navegador Web
## De Estructuras de Datos a Programación Paralela

**Universidad Nacional de Costa Rica**
Facultad de Ciencias Exactas y Naturales · Escuela de Informática

Programación Paralela — EIF-XXX
Ciclo I 2026 · Yassir Jiménez Carballo

---SLIDE 2 — EL PROYECTO ORIGINAL---

# ¿Qué es el proyecto original?

**Curso:** EIF207 – Estructuras de Datos
**Entregado:** Septiembre 2024 · Valor 20% de la nota

### ¿Qué debíamos construir?
Un **simulador de navegador web en consola** con:
- Historial de navegación (atrás / adelante con flechas)
- Múltiples pestañas con historial independiente
- Sistema de marcadores con búsqueda y etiquetas
- Modo incógnito (sin historial ni bookmarks)
- Importación / exportación de sesión en binario
- Políticas de historial: límite de entradas y tiempo máximo
- Carga de 1 000 sitios web desde un CSV

**Restricción del curso:** estrictamente prohibido usar hilos.

---SLIDE 3 — CONOCIMIENTO EN AQUEL MOMENTO---

# ¿Qué sabíamos entonces?

| Tema | Nivel |
|------|-------|
| C++ (punteros, herencia, STL) | Intermedio |
| Estructuras lineales (list, vector, stack) | Sólido |
| Serialización binaria | Básico |
| Programación orientada a objetos | Sólido |
| Concurrencia / hilos | **Ninguno** — no estaba en el plan del curso |
| OpenMP / paralelismo | **Ninguno** |

El proyecto se desarrolló 100 % secuencial por diseño del curso.
Todo el código corre en **1 solo hilo**.

---SLIDE 4 — ARQUITECTURA---

# Arquitectura del proyecto (MVC)

```
main()  →  Controladora::control0()  [bucle principal]
               ↓
           Navegador  (coordinador central)
           ├── ListaPestanias  →  PestaniaAbstracta
           │                      ├── Pestania  →  Historial  →  std::list<SitioWeb*>
           │                      └── PestaniaIncognito  (sin historial)
           ├── std::vector<SitioWeb*>   ← 1 000 sitios del CSV
           ├── std::list<Marcador*>     ← bookmarks globales
           └── ConfigHistorial*         ← singleton (maxEntradas, tiempoMaximo)
```

**Clases principales:** SitioWeb · Historial · Marcador · ConfigHistorial
**Vista:** Interfaz (Windows Console API, PeekConsoleInput)
**Controlador:** Controladora (13 submenús, routing de teclado)

---SLIDE 5 — ¿POR QUÉ LO ELEGIMOS?---

# ¿Por qué elegimos este proyecto para paralelizar?

1. **Ya teníamos prohibido usar hilos** — era la oportunidad perfecta para ver qué ganábamos si los usábamos.

2. **Operaciones sobre colecciones grandes e independientes** — buscar en 1 000 sitios, limpiar historial, cargar CSV: cada elemento se puede evaluar sin depender del anterior.

3. **Cuello de botella claro** — `cargarArchivoSitiosWebCSV()` consumía el **79.6 %** del tiempo total secuencial. La Ley de Amdahl señalaba directamente ahí.

4. **Variedad de patrones** — el proyecto permite demostrar tres esquemas distintos de paralelismo: búsqueda simple, pipeline IO-cómputo, y 2 fases para estructuras no thread-safe.

5. **Contexto académico real** — no es un ejemplo inventado: es código que ya usamos, con datos reales (1 000 sitios), lo que hace la comparación honesta y significativa.

---SLIDE 6 — LAS 5 FUNCIONES (RESUMEN)---

# Las 5 funciones que paralelizamos

| # | Función | Archivo | Patrón OpenMP |
|---|---------|---------|---------------|
| 1 | `busquedaMasiva()` | Navegador.cpp | `parallel for` + compactación |
| 2 | `buscarPaginaWeb()` | Navegador.cpp | `parallel for` + `critical` + early-exit |
| 3 | `cargarArchivoSitiosWebCSV()` | Navegador.cpp | Pipeline: IO serial → parseo paralelo → merge serial |
| 4 | `busquedaPalabraClave()` | Historial.cpp | `parallel for schedule(dynamic)` |
| 5 | `limpiarSitiosViejos()` | Historial.cpp | 2 fases: evaluación paralela + borrado serial |

**Bonus:** `ConfigHistorial` (singleton) refactorizado con `std::mutex` para ser thread-safe.

---SLIDE 7A — FUNCIÓN 1: busquedaMasiva---

# Función 1 — `busquedaMasiva()`

### ANTES (secuencial)
```cpp
std::vector<SitioWeb*> Navegador::busquedaMasiva(const std::string& palabraClave) {
    std::vector<SitioWeb*> resultado;
    for (int i = 0; i < (int)sitios.size(); i++) {
        std::string titulo = sitios[i]->getTitulo();
        std::string url    = sitios[i]->getUrl();
        std::transform(titulo.begin(), titulo.end(), titulo.begin(), ::tolower);
        std::transform(url.begin(), url.end(), url.begin(), ::tolower);
        std::string kw = palabraClave;
        std::transform(kw.begin(), kw.end(), kw.begin(), ::tolower);
        if (titulo.find(kw) != std::string::npos || url.find(kw) != std::string::npos)
            resultado.push_back(sitios[i]);
    }
    return resultado;
}
```

### DESPUÉS (paralelo)
```cpp
std::vector<SitioWeb*> Navegador::busquedaMasiva(const std::string& palabraClave) {
    int n = (int)sitios.size();
    std::vector<SitioWeb*> resultados(n, nullptr);   // ← índices fijos, sin race condition

    #pragma omp parallel for schedule(dynamic, 64) shared(resultados)
    for (int i = 0; i < n; i++) {
        std::string titulo = sitios[i]->getTitulo();
        std::string url    = sitios[i]->getUrl();
        std::transform(titulo.begin(), titulo.end(), titulo.begin(), ::tolower);
        std::transform(url.begin(), url.end(), url.begin(), ::tolower);
        std::string kw = palabraClave;
        std::transform(kw.begin(), kw.end(), kw.begin(), ::tolower);
        if (titulo.find(kw) != std::string::npos || url.find(kw) != std::string::npos)
            resultados[i] = sitios[i];               // ← cada hilo escribe en su propio índice
    }

    std::vector<SitioWeb*> compacto;
    for (auto s : resultados) if (s) compacto.push_back(s);
    return compacto;
}
```
**Cambio clave:** en vez de `push_back` (no thread-safe), pre-allocamos el vector con índices fijos — cada hilo escribe solo en su posición sin tocar las demás.

---SLIDE 7B — FUNCIÓN 2: buscarPaginaWeb---

# Función 2 — `buscarPaginaWeb()`

### ANTES (secuencial)
```cpp
SitioWeb* Navegador::buscarPaginaWeb(const std::string url) {
    for (auto& s : sitios)
        if (s->getUrl() == url) return s;
    return nullptr;
}
```

### DESPUÉS (paralelo con early-exit)
```cpp
SitioWeb* Navegador::buscarPaginaWeb(const std::string url) {
    SitioWeb* resultado    = nullptr;
    int idxEncontrado      = (int)sitios.size();   // ← "no encontrado aún"

    #pragma omp parallel for shared(resultado, idxEncontrado)
    for (int i = 0; i < (int)sitios.size(); i++) {
        #pragma omp flush(idxEncontrado)            // ← ver el valor actualizado por otros hilos
        if (i >= idxEncontrado) continue;           // ← saltarse trabajo innecesario
        if (sitios[i]->getUrl() == url) {
            #pragma omp critical
            {
                if (i < idxEncontrado) {            // ← guardar el índice más bajo encontrado
                    idxEncontrado = i;
                    resultado = sitios[i];
                }
            }
        }
    }
    return resultado;
}
```
**Cambio clave:** el secuencial para al primer match con `return`; en paralelo no se puede salir del loop, pero con `flush` + `critical` los hilos ignoran índices mayores al ya encontrado.

---SLIDE 7C — FUNCIÓN 3: cargarArchivoSitiosWebCSV---

# Función 3 — `cargarArchivoSitiosWebCSV()`

### ANTES (secuencial)
```cpp
void Navegador::cargarArchivoSitiosWebCSV(const std::string& rutaArchivo) {
    std::ifstream archivo(rutaArchivo);
    std::string linea;
    while (std::getline(archivo, linea)) {
        if (linea.empty()) continue;
        std::stringstream ss(linea);
        std::string url, titulo, dominio;
        if (std::getline(ss, url, ',') && std::getline(ss, titulo, ',') &&
            std::getline(ss, dominio))
            sitios.push_back(new SitioWeb(url, titulo, dominio));
    }
}
```

### DESPUÉS (pipeline 3 fases)
```cpp
void Navegador::cargarArchivoSitiosWebCSV(const std::string& rutaArchivo) {
    // FASE 1 — serial: la IO de archivos no es thread-safe
    std::vector<std::string> lineas;
    std::ifstream archivo(rutaArchivo);
    std::string linea;
    while (std::getline(archivo, linea))
        if (!linea.empty()) lineas.push_back(linea);

    // FASE 2 — paralelo: construir objetos, cada línea es independiente
    int n = (int)lineas.size();
    std::vector<SitioWeb*> temp(n, nullptr);
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < n; i++) {
        std::stringstream ss(lineas[i]);
        std::string url, titulo, dominio;
        if (std::getline(ss, url, ',') && std::getline(ss, titulo, ',') &&
            std::getline(ss, dominio))
            temp[i] = new SitioWeb(url, titulo, dominio);
    }

    // FASE 3 — serial: consolidar al vector principal
    for (auto s : temp) if (s) sitios.push_back(s);
}
```
**Cambio clave:** separar IO (serial obligatorio) del parseo/construcción de objetos (paralelo seguro).

---SLIDE 7D — FUNCIÓN 4: busquedaPalabraClave---

# Función 4 — `busquedaPalabraClave()`

### ANTES (secuencial)
```cpp
std::string Historial::busquedaPalabraClave(const std::string& palabraClave) {
    std::string resultado;
    for (auto it = sitiosVisitados.begin(); it != sitiosVisitados.end(); ++it) {
        std::string titulo = (*it)->getTitulo();
        std::string url    = (*it)->getUrl();
        std::transform(titulo.begin(), titulo.end(), titulo.begin(), ::tolower);
        std::transform(url.begin(), url.end(), url.begin(), ::tolower);
        std::string kw = palabraClave;
        std::transform(kw.begin(), kw.end(), kw.begin(), ::tolower);
        if (titulo.find(kw) != std::string::npos || url.find(kw) != std::string::npos)
            resultado += (*it)->toString() + "\n";
    }
    return resultado;
}
```

### DESPUÉS (paralelo)
```cpp
std::string Historial::busquedaPalabraClave(const std::string& palabraClave) {
    // std::list no tiene acceso aleatorio → copiamos a vector primero
    std::vector<SitioWeb*> vec(sitiosVisitados.begin(), sitiosVisitados.end());
    int n = (int)vec.size();
    std::vector<std::string> partes(n);
    std::vector<bool>        coincide(n, false);

    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < n; i++) {
        std::string titulo = vec[i]->getTitulo();
        std::string url    = vec[i]->getUrl();
        std::transform(titulo.begin(), titulo.end(), titulo.begin(), ::tolower);
        std::transform(url.begin(), url.end(), url.begin(), ::tolower);
        std::string kw = palabraClave;
        std::transform(kw.begin(), kw.end(), kw.begin(), ::tolower);
        if (titulo.find(kw) != std::string::npos || url.find(kw) != std::string::npos) {
            partes[i]   = vec[i]->toString();
            coincide[i] = true;
        }
    }

    std::string resultado;
    for (int i = 0; i < n; i++) if (coincide[i]) resultado += partes[i] + "\n";
    return resultado;
}
```
**Cambio clave:** `std::list` no tiene iterador aleatorio (necesario para `parallel for`), así que se pre-indexa en un `vector` antes del bloque paralelo.

---SLIDE 7E — FUNCIÓN 5: limpiarSitiosViejos---

# Función 5 — `limpiarSitiosViejos()`

### ANTES (secuencial)
```cpp
bool Historial::limpiarSitiosViejos() {
    if (sitiosVisitados.empty() || tiempoMaximo <= 0) return false;
    bool huboLimpieza = false;
    auto ahora = std::chrono::system_clock::now();
    for (auto it = sitiosVisitados.begin(); it != sitiosVisitados.end(); ) {
        auto diff = std::chrono::duration_cast<std::chrono::seconds>(
            ahora - (*it)->getFechaVisita()).count();
        if (diff > tiempoMaximo) { it = sitiosVisitados.erase(it); huboLimpieza = true; }
        else ++it;
    }
    return huboLimpieza;
}
```

### DESPUÉS (2 fases: evaluación paralela + borrado serial)
```cpp
bool Historial::limpiarSitiosViejos() {
    if (sitiosVisitados.empty() || tiempoMaximo <= 0) return false;
    std::vector<SitioWeb*> vec(sitiosVisitados.begin(), sitiosVisitados.end());
    int n = (int)vec.size();
    std::vector<bool> expirado(n, false);
    auto ahora = std::chrono::system_clock::now();

    // FASE 1 — paralelo: evaluar cuáles expiraron (solo lectura)
    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        auto diff = std::chrono::duration_cast<std::chrono::seconds>(
            ahora - vec[i]->getFechaVisita()).count();
        expirado[i] = (diff > tiempoMaximo);
    }

    // FASE 2 — serial: borrar de la lista (std::list::erase no es thread-safe)
    bool huboLimpieza = false;
    auto it = sitiosVisitados.begin();
    for (int i = 0; i < n; i++, ++it) {
        if (expirado[i]) { it = sitiosVisitados.erase(it); --it; huboLimpieza = true; }
    }
    return huboLimpieza;
}
```
**Cambio clave:** `std::list::erase` modifica punteros internos de la lista y no puede ejecutarse en paralelo. Se separa la evaluación (paralela, solo lectura) del borrado (serial, escritura).

---SLIDE 8 — HILOS: LEY DE AMDAHL---

# ¿Cuántos hilos? — Ley de Amdahl

**Fórmula:** `S(n) = 1 / ((1 - p) + p/n)`
**Fracción paralela estimada:** `p = 0.85` (cargarCSV = 79.6 % del tiempo total)

| Hilos (n) | Speedup teórico |
|-----------|----------------|
| 1 | 1.00× |
| 2 | 1.54× |
| **4** | **2.35×** ← elegido |
| 8 | 3.08× |
| 16 | 3.76× |
| ∞ | 6.67× (límite) |

### ¿Por qué 4?
- De 1 a 4 hilos: ganancia de **+1.35×**
- De 4 a 8 hilos: ganancia de solo **+0.73×** (rendimiento decreciente)
- 4 hilos = núcleos físicos típicos de una laptop → sin contención de contexto
- Punto de inflexión de la curva para p = 0.85

```cpp
// Source.cpp — versión paralela
omp_set_num_threads(4);
```

---SLIDE 9 — COMPARACIÓN DE TIEMPOS---

# Comparación de tiempos reales
### Benchmark: 1 000 sitios, 10 pestañas × 100 páginas · 3 corridas · g++ -O2

| Función | Secuencial (1h) | Paralelo (4h) | Speedup |
|---------|----------------|---------------|---------|
| busquedaMasiva | 0.421 ms | 0.302 ms | **1.39×** |
| busquedaPalabraClave | 0.022 ms | 0.091 ms | 0.24× ⚠ |
| limpiarSitiosViejos | 0.010 ms | 0.768 ms | 0.013× ⚠ |
| cargarArchivoSitiosWebCSV | 1.894 ms | 1.471 ms | **1.29×** |
| toString | 0.033 ms | 0.025 ms | **1.35×** |
| **TOTAL** | **2.380 ms** | **2.657 ms** | **0.90×** |

**Speedup teórico Amdahl (p=0.85, n=4):** 2.35×
**Speedup real medido:** 0.90×

⚠ El overhead de crear y sincronizar 4 hilos supera al tiempo de cómputo cuando hay solo 100–1000 elementos. Con 50 000+ elementos los resultados serían distintos.

---SLIDE 10 — ¿POR QUÉ EL OVERHEAD GANÓ?---

# ¿Por qué no hubo speedup general?

### 3 razones concretas

**1. Dataset demasiado pequeño**
OpenMP tarda ~50–200 µs en crear y coordinar 4 hilos. Con 1 000 elementos de operaciones simples (comparar strings cortos), cada hilo hace microsegundos de trabajo. El overhead domina.

**2. `std::list` fuerza 3 pases en vez de 1**
`limpiarSitiosViejos` hace: copiar lista → evaluar en paralelo → borrar secuencial. El secuencial hace un solo pase. Con pocos elementos el costo se triplica.

**3. Caché de CPU**
1 000 punteros × 8 bytes = 8 KB → entra entero en L1. El secuencial tiene acceso lineal perfecto. Cuatro hilos fragmentan el acceso y generan más cache misses.

### ¿Cuándo sí daría speedup?
Con **50 000+ sitios**, el tiempo de cómputo supera al overhead y las mejoras predichas por Amdahl se materializan.

---SLIDE 11 — CONCLUSIÓN---

# Conclusión

### ¿Fue una buena elección para paralelizar?

**Sí — por estas razones:**

1. **Demostró los límites reales** del paralelismo: la Ley de Amdahl predice el techo teórico, pero el overhead práctico es igualmente importante. Esta brecha entre teoría y práctica es la lección más valiosa del ejercicio.

2. **El patrón de datos era correcto** — búsquedas sobre colecciones de elementos independientes son el caso de uso canónico de `parallel for`. La estructura del proyecto lo permitía limpiamente.

3. **Mejoramos la calidad del código** más allá del rendimiento:
   - `ConfigHistorial` ahora es thread-safe con double-checked locking
   - Las funciones de búsqueda son más explícitas sobre sus dependencias de datos
   - El pipeline IO/cómputo en `cargarCSV` separa responsabilidades correctamente

4. **Revelamos una deuda técnica del curso original**: la prohibición de hilos en EIF207 protegía la complejidad del proyecto, pero dejó código con patrones que escalan mal. OpenMP mostró exactamente dónde.

5. **Con datos reales de producción** (50 000+ sitios), `busquedaMasiva` y `cargarArchivoSitiosWebCSV` mostrarían el speedup predicho de ~2×, justificando la paralelización en un contexto de despliegue real.

> El aprendizaje no estuvo en hacer el programa más rápido con 1 000 sitios —
> estuvo en entender **por qué** no fue más rápido, y **cuándo** sí lo sería.
