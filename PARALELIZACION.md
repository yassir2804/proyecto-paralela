# Análisis de Paralelización — Navegador Web Simulator

## ¿Qué se hizo en esta rama de trabajo?

Se extendió el proyecto con un **dataset de 1000 sitios web**, dos programas de benchmark para medir tiempos reales, y se identificaron las funciones existentes del proyecto que pueden paralelizarse con **OpenMP** para obtener una mejora medible de rendimiento.

---

## 1. Cómo funciona el programa normalmente (versión secuencial)

El programa es un simulador de navegador web con menú interactivo. Cuando el usuario hace algo, el programa ejecuta las operaciones **de una en una, en un solo hilo**:

```
Usuario presiona tecla
    ↓
Controladora detecta la acción
    ↓
Navegador ejecuta la operación (buscar, filtrar, limpiar...)
    ↓                           ← todo esto en 1 hilo, uno por uno
Historial actualiza su lista
    ↓
Interfaz muestra el resultado
```

Con 16 sitios no se nota la lentitud. Con 1000 sitios, las funciones que recorren todos los datos se vuelven un cuello de botella.

---

## 2. Dataset de 1000 sitios

Se generó `Proyecto1Datos/sitiosWeb_1000.csv` con **1000 sitios en 10 categorías** (100 por categoría):

| Categoría     | Ejemplo de URL                          |
|---------------|-----------------------------------------|
| tecnologia    | https://www.tecnologia001.com           |
| noticias      | https://www.noticias050.com             |
| educacion     | https://www.educacion099.com            |
| entretenimiento | https://www.entretenimiento001.com    |
| compras       | https://www.compras025.com              |
| social        | https://www.social010.com               |
| gobierno      | https://www.gobierno001.com             |
| salud         | https://www.salud075.com                |
| deportes      | https://www.deportes100.com             |
| ciencia       | https://www.ciencia042.com              |

Para usar el programa normal con 1000 datos, el CSV ya reemplaza al original (el de 16 sitios se guardó como `sitiosWeb_16original.csv`).

---

## 3. Benchmark 1 — Funciones existentes del proyecto

**Archivo:** `Proyecto1Datos/benchmark.cpp`  
**Compilar y correr:** `compilar_benchmark.bat`

Mide las 5 funciones del proyecto tal como están (versión secuencial, sin modificar):

| # | Función | Tiempo promedio | Complejidad |
|---|---------|-----------------|-------------|
| 1 | `buscarPaginaWeb()` — 1000 búsquedas | **41 ms** | O(n²) total |
| 2 | `cargarArchivoSitiosWebCSV()` | **1.5 ms** | O(n) |
| 3 | `busquedaPalabraClave()` | **475 µs** | O(n) |
| 4 | `moverseAPrimeraCoincidencia()` | **107 µs** | O(n) |
| 5 | `limpiarSitiosViejos()` | **89 µs** | O(n) |

> Las funciones F3-F5 son tan rápidas en una sola pasada que la ganancia de paralelizarlas sería pequeña en la práctica. Por eso se creó el Benchmark 2.

---

## 4. Benchmark 2 — Funciones con tiempos significativos

**Archivo:** `Proyecto1Datos/benchmark2.cpp`  
**Compilar:** `g++ -std=c++14 -O2 -o benchmark2.exe benchmark2.cpp Historial.cpp SitioWeb.cpp ConfigHistorial.cpp Excepciones.cpp -I.`  
**Correr:** `benchmark2.exe`

Simula cargas de trabajo reales (50 pestañas, operaciones repetidas):

| # | Función / Escenario | Tiempo secuencial | Estimado con 4 hilos |
|---|---------------------|-------------------|----------------------|
| 1 | Ordenamiento masivo (100 × sort de 1000) | **244 ms** | ~61 ms |
| 2 | Serialización + deserialización binaria | **131 ms** | ~33 ms |
| 3 | Detección de duplicados O(n²) | **71 ms** | ~18 ms |
| 4 | Generación de reporte toString (×100) | **50 ms** | ~12 ms |
| 5 | Búsqueda en 50 pestañas simultáneas | **18 ms** | ~4 ms |

---

## 5. Plan de paralelización con OpenMP

Estas son las **funciones que ya existen en el proyecto** que tu compañera tiene que modificar para agregar OpenMP. Son las de mayor impacto real.

### Compilación con OpenMP

Agregar `-fopenmp` al comando de compilación:

```bat
g++ -std=c++14 -fopenmp -o navegador.exe Source.cpp Controladora.cpp Navegador.cpp ^
    Interfaz.cpp Historial.cpp ListaPestanias.cpp Pestania.cpp PestaniaIncognito.cpp ^
    SitioWeb.cpp Marcador.cpp ConfigHistorial.cpp Excepciones.cpp -I.
```

---

### Función 1 — `Historial::busquedaPalabraClave()`
📁 **Archivo:** `Proyecto1Datos/Historial.cpp` — línea 156

**Qué hace:** Recorre todo el historial buscando una palabra clave en el título de cada sitio. Con 1000 entradas hace 1000 `tolower()` + 1000 `find()`.

**Código original (secuencial):**
```cpp
for (SitioWeb* sitio : historial) {
    std::string tituloSitio = sitio->getTitulo();
    transform(tituloSitio.begin(), tituloSitio.end(), tituloSitio.begin(), ::tolower);
    if (tituloSitio.find(palabraFiltrada) != std::string::npos) {
        s << " COINCIDENCIA # " << contador << std::endl;
        s << sitio->toString() << std::endl;
        contador++;
    }
}
```

**Versión paralela con OpenMP:**
```cpp
// Convertir list a vector para acceso por índice (OpenMP necesita índices)
std::vector<SitioWeb*> vec(historial.begin(), historial.end());
std::vector<std::string> partes(vec.size());  // cada hilo escribe su parte

#pragma omp parallel for schedule(dynamic)
for (int i = 0; i < (int)vec.size(); i++) {
    std::string titulo = vec[i]->getTitulo();
    std::transform(titulo.begin(), titulo.end(), titulo.begin(), ::tolower);
    if (titulo.find(palabraFiltrada) != std::string::npos) {
        std::ostringstream oss;
        oss << "------\n COINCIDENCIA\n" << vec[i]->toString() << "\n------\n";
        partes[i] = oss.str();   // escritura en posición propia → sin race condition
    }
}

// Merge secuencial (O(n), rápido)
for (const auto& parte : partes) s << parte;
```

**Por qué funciona:** cada hilo trabaja sobre una posición diferente del vector `partes`, sin pisar el trabajo de los otros.

---

### Función 2 — `Historial::limpiarSitiosViejos()`
📁 **Archivo:** `Proyecto1Datos/Historial.cpp` — línea 268

**Qué hace:** Recorre el historial comparando el timestamp de cada sitio con el tiempo máximo configurado. Si está vencido, lo elimina.

**Código original (secuencial):**
```cpp
for (auto it = historial.begin(); it != historial.end(); ) {
    double diff = duration_cast<seconds>(now() - (*it)->getTiempoDeIngreso()).count();
    if (diff > tiempoMaximo) {
        delete *it;
        it = historial.erase(it);
    } else {
        ++it;
    }
}
```

**Versión paralela con OpenMP (pipeline 2 fases):**
```cpp
// FASE 1: verificar timestamps en paralelo (lectura pura, sin escritura compartida)
std::vector<SitioWeb*> vec(historial.begin(), historial.end());
std::vector<bool> expirado(vec.size(), false);

#pragma omp parallel for
for (int i = 0; i < (int)vec.size(); i++) {
    double diff = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now() - vec[i]->getTiempoDeIngreso()
    ).count();
    expirado[i] = (diff > tiempoMaximo);   // escritura en índice propio
}

// FASE 2: eliminar los marcados (secuencial para preservar integridad de la list)
for (int i = 0; i < (int)vec.size(); i++) {
    if (expirado[i]) {
        delete vec[i];
        historial.remove(vec[i]);
        entradasBorradas = true;
    }
}
```

**Por qué 2 fases:** `std::list` no soporta acceso concurrente. La parte pesada (calcular timestamps) se hace en paralelo; el borrado se hace solo, pero es O(número borrados), no O(total).

---

### Función 3 — `Historial::toString()`
📁 **Archivo:** `Proyecto1Datos/Historial.cpp` — línea 206

**Qué hace:** Construye un string con la representación completa de todo el historial (URL + título + dominio de cada entrada).

**Código original (secuencial):**
```cpp
std::string Historial::toString() const {
    std::stringstream ss;
    ss << "Historial:\n";
    for (const auto sitio : historial) {
        ss << sitio->toString() << "\n";
    }
    return ss.str();
}
```

**Versión paralela con OpenMP:**
```cpp
std::string Historial::toString() const {
    std::vector<SitioWeb*> vec(historial.begin(), historial.end());
    std::vector<std::string> partes(vec.size());

    #pragma omp parallel for
    for (int i = 0; i < (int)vec.size(); i++) {
        partes[i] = vec[i]->toString() + "\n";   // cada hilo su índice
    }

    std::stringstream ss;
    ss << "Historial:\n";
    for (const auto& p : partes) ss << p;   // merge secuencial
    return ss.str();
}
```

---

### Función 4 — `Navegador::buscarPaginaWeb()`
📁 **Archivo:** `Proyecto1Datos/Navegador.cpp` — línea 217

**Qué hace:** Busca un sitio por URL exacta dentro del vector `sitios` (los 1000 sitios cargados del CSV). Recorre uno a uno hasta encontrarlo.

**Código original (secuencial):**
```cpp
SitioWeb* Navegador::buscarPaginaWeb(const std::string url) {
    auto it = std::find_if(sitios.begin(), sitios.end(), [&](SitioWeb* sitio) {
        return sitio->getUrl() == url;
    });
    if (it != sitios.end()) return *it;
    throw ExcepcionGenerica("404 - Not Found");
}
```

**Versión paralela con OpenMP:**
```cpp
SitioWeb* Navegador::buscarPaginaWeb(const std::string url) {
    SitioWeb* resultado = nullptr;
    int idx_encontrado = (int)sitios.size();   // posición del match (para quedarnos con el primero)

    #pragma omp parallel for shared(resultado, idx_encontrado)
    for (int i = 0; i < (int)sitios.size(); i++) {
        #pragma omp flush(idx_encontrado)
        if (i >= idx_encontrado) continue;   // ya encontramos uno antes, saltar

        if (sitios[i]->getUrl() == url) {
            #pragma omp critical
            {
                if (i < idx_encontrado) {
                    idx_encontrado = i;
                    resultado = sitios[i];
                }
            }
        }
    }

    if (resultado) return resultado;
    throw ExcepcionGenerica("404 - Not Found");
}
```

**Nota:** Hay que agregar `#include <omp.h>` al inicio de `Navegador.cpp`.

---

### Función 5 — `Navegador::cargarArchivoSitiosWebCSV()`
📁 **Archivo:** `Proyecto1Datos/Navegador.cpp` — línea 314

**Qué hace:** Lee el CSV línea por línea, parsea cada línea y crea un objeto `SitioWeb*`. Con 1000 líneas hace 1000 `new SitioWeb()`.

**Código original (secuencial):**
```cpp
void Navegador::cargarArchivoSitiosWebCSV(const std::string& rutaArchivo) {
    std::ifstream archivo(rutaArchivo);
    std::string linea;
    while (std::getline(archivo, linea)) {
        std::stringstream ss(linea);
        std::string url, titulo, dominio;
        if (std::getline(ss, url, ',') && std::getline(ss, titulo, ',') &&
            std::getline(ss, dominio)) {
            sitios.push_back(new SitioWeb(url, titulo, dominio));
        }
    }
}
```

**Versión paralela con OpenMP (producer-consumer):**
```cpp
void Navegador::cargarArchivoSitiosWebCSV(const std::string& rutaArchivo) {
    // PASO 1: leer todas las líneas en memoria (IO secuencial, no paralelizable)
    std::ifstream archivo(rutaArchivo);
    std::vector<std::string> lineas;
    std::string linea;
    while (std::getline(archivo, linea)) {
        if (!linea.empty()) lineas.push_back(linea);
    }

    // PASO 2: parsear y crear SitioWeb* en paralelo
    std::vector<SitioWeb*> temp(lineas.size(), nullptr);

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < (int)lineas.size(); i++) {
        std::istringstream ss(lineas[i]);
        std::string url, titulo, dominio;
        if (std::getline(ss, url, ',') && std::getline(ss, titulo, ',') &&
            std::getline(ss, dominio)) {
            temp[i] = new SitioWeb(url, titulo, dominio);   // new es thread-safe
        }
    }

    // PASO 3: consolidar (mantiene el orden original)
    for (auto* s : temp) {
        if (s) sitios.push_back(s);
    }
}
```

**Nota:** Agregar `#include <sstream>` si no está ya en `Navegador.cpp`.

---

## 6. Cómo medir la diferencia (antes vs después)

### Paso 1 — Correr el benchmark ANTES de modificar nada

```bat
cd Proyecto1Datos
benchmark2.exe > resultados_ANTES.txt
```

Guarda los tiempos actuales (versión secuencial).

### Paso 2 — Hacer los cambios con OpenMP en las funciones indicadas

Modificar `Historial.cpp` y `Navegador.cpp` según el código de arriba.

### Paso 3 — Recompilar con `-fopenmp`

```bat
g++ -std=c++14 -O2 -fopenmp -o benchmark2.exe benchmark2.cpp ^
    Historial.cpp SitioWeb.cpp ConfigHistorial.cpp Excepciones.cpp -I.
```

### Paso 4 — Correr el benchmark DESPUÉS

```bat
benchmark2.exe > resultados_DESPUES.txt
```

### Paso 5 — Comparar

```bat
type resultados_ANTES.txt
type resultados_DESPUES.txt
```

La mejora esperada con 4 núcleos es aproximadamente **3×–4× más rápido** en cada función paralelizada.

---

## 7. Tabla resumen de ganancia esperada

| Función | Archivo | Tiempo secuencial | Esperado paralelo (4 hilos) | Patrón OpenMP |
|---------|---------|-------------------|-----------------------------|---------------|
| `busquedaPalabraClave()` | Historial.cpp:156 | 475 µs × 50 tabs = 18 ms | ~4 ms | `parallel for` + vector de partes |
| `limpiarSitiosViejos()` | Historial.cpp:268 | 89 µs | ~22 µs | `parallel for` fase check |
| `toString()` | Historial.cpp:206 | 50 ms (×100) | ~12 ms | `parallel for` + vector de partes |
| `buscarPaginaWeb()` | Navegador.cpp:217 | 41 ms (×1000) | ~10 ms | `parallel for` + `critical` |
| `cargarArchivoSitiosWebCSV()` | Navegador.cpp:314 | 1.5 ms | ~0.4 ms | `parallel for` parse fase |

---

## 8. Reglas importantes al usar OpenMP

1. **No modificar `std::list` dentro de un `parallel for`** — la lista no es thread-safe. Siempre convertir a `std::vector` primero.
2. **Usar `#pragma omp critical`** cuando varios hilos necesiten escribir en la misma variable.
3. **Cada hilo debe escribir en su propio índice** del vector de resultados — así no hay race conditions.
4. **`new SitioWeb()`** es thread-safe en g++ con MinGW — se puede llamar desde múltiples hilos.
5. **El singleton `ConfigHistorial::getInstancia()`** no es thread-safe — agregar un `std::mutex` antes de paralelizar cualquier función que lo llame.

---

## 9. Archivos de este trabajo

```
Proyecto1Datos/
├── sitiosWeb.csv                ← CSV activo (ahora 1000 sitios)
├── sitiosWeb_1000.csv           ← copia explícita de los 1000 sitios
├── sitiosWeb_16original.csv     ← backup de los 16 sitios originales
├── benchmark.cpp                ← Benchmark 1: funciones existentes, referencia
├── benchmark2.cpp               ← Benchmark 2: cargas de trabajo significativas
└── compilar_benchmark.bat       ← compila y corre benchmark.exe
```

Para volver a los 16 sitios originales:
```bat
copy Proyecto1Datos\sitiosWeb_16original.csv Proyecto1Datos\sitiosWeb.csv
```
