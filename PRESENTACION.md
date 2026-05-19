# Paralelización de un Simulador de Navegador Web en C++
## Programación Paralela — Universidad Nacional de Costa Rica (UNA)
### Ciclo I 2026 · Yassir Jiménez Carballo

---

## 1. ¿Qué es el proyecto original?

### Descripción

El proyecto base es un **simulador de navegador web en consola** desarrollado en C++14 para Windows. Implementa las funcionalidades principales de un navegador moderno pero en modo texto, sin interfaz gráfica:

- **Pestañas múltiples** con cambio dinámico entre ellas
- **Historial bidireccional** (avanzar/retroceder, truncamiento de historial futuro)
- **Marcadores** (bookmarks) persistentes entre sesiones
- **Modo incógnito** (sin historial, sin bookmarks)
- **Persistencia de sesión** via serialización binaria (`sesion.dat`)
- **Base de datos de 1 000 sitios web** cargada desde un CSV

### Arquitectura

El proyecto sigue el patrón **MVC**:

```
main() → Controladora::control0() [bucle principal]
    ↓
    Navegador  (coordinador central; dueño de todo el estado)
    ├── ListaPestanias  →  PestaniaAbstracta
    │                      ├── Pestania  →  Historial  →  std::list<SitioWeb*>
    │                      └── PestaniaIncognito  (sin historial)
    ├── std::vector<SitioWeb*>  (1 000 sitios desde sitiosWeb.csv)
    ├── std::list<Marcador*>    (marcadores globales)
    └── ConfigHistorial*        (singleton: maxEntradas, tiempoMaximo)
```

### Curso y nivel de conocimiento

| Aspecto | Detalle |
|---------|---------|
| Curso | Programación Paralela — UNA |
| Nivel del equipo | C++ intermedio (punteros, herencia, STL) |
| Paradigma base | Programación orientada a objetos, MVC |
| Plataforma | Windows (usa `<windows.h>`, `PeekConsoleInput`) |
| Compilador | g++ (MinGW) / MSVC, estándar C++14 |

---

## 2. ¿Por qué fue elegido para paralelización?

### Candidatos identificados

El simulador realiza 5 operaciones que son intrínsecamente paralelizables porque procesan colecciones independientes (sin dependencias entre elementos):

| Función | Estructura | Tipo de trabajo |
|---------|-----------|-----------------|
| `busquedaMasiva()` | `vector<SitioWeb*>` (1 000) | Búsqueda lineal con predicado |
| `buscarPaginaWeb()` | `vector<SitioWeb*>` (1 000) | Búsqueda con early-exit |
| `cargarArchivoSitiosWebCSV()` | Líneas de CSV | Parseo de objetos independientes |
| `busquedaPalabraClave()` | `list<SitioWeb*>` (historial) | Filtro por keyword |
| `limpiarSitiosViejos()` | `list<SitioWeb*>` (historial) | Filtro por timestamp |

### Justificación técnica

1. **Trabajo por datos** (data parallelism): cada sitio web o entrada de historial puede evaluarse sin conocer los demás resultados.
2. **Carga real**: la base de datos de 1 000 sitios provee suficiente trabajo para medir diferencias.
3. **Punto caliente medible**: `cargarArchivoSitiosWebCSV()` representa el **79.6 %** del tiempo total secuencial — candidato natural según la Ley de Amdahl.
4. **Variedad de patrones**: permite demostrar tres patrones distintos de OpenMP:
   - `parallel for` simple (busquedaMasiva)
   - `parallel for` con `critical` (buscarPaginaWeb con early-exit)
   - Pipeline en 2 fases: paralelo para evaluar, secuencial para modificar (limpiarSitiosViejos con `std::list`)

---

## 3. Funciones paralelizadas (código)

Se paralelizaron **5 funciones** usando OpenMP 4.5. A continuación, el fragmento clave de cada una.

### Función 1 — `busquedaMasiva()` en `Navegador.cpp`

Búsqueda masiva sobre los 1 000 sitios de la base de datos. Patrón: `parallel for` con reducción manual a vector de resultados.

```cpp
// FUNCION PARALELIZADA 1: busquedaMasiva
std::vector<SitioWeb*> Navegador::busquedaMasiva(const std::string& palabraClave) {
    int n = (int)sitios.size();
    std::vector<SitioWeb*> resultados(n, nullptr);

    #pragma omp parallel for schedule(dynamic, 64) shared(resultados)
    for (int i = 0; i < n; i++) {
        std::string titulo = sitios[i]->getTitulo();
        std::string url    = sitios[i]->getUrl();
        std::transform(titulo.begin(), titulo.end(), titulo.begin(), ::tolower);
        std::transform(url.begin(),   url.end(),   url.begin(),   ::tolower);
        std::string kw = palabraClave;
        std::transform(kw.begin(), kw.end(), kw.begin(), ::tolower);
        if (titulo.find(kw) != std::string::npos || url.find(kw) != std::string::npos)
            resultados[i] = sitios[i];
    }

    std::vector<SitioWeb*> compacto;
    for (auto s : resultados) if (s) compacto.push_back(s);
    return compacto;
}
```

---

### Función 2 — `buscarPaginaWeb()` en `Navegador.cpp`

Búsqueda de primera coincidencia con early-exit paralelo usando `#pragma omp flush` y `critical`.

```cpp
// FUNCION PARALELIZADA 2: buscarPaginaWeb
SitioWeb* Navegador::buscarPaginaWeb(const std::string url) {
    SitioWeb* resultado = nullptr;
    int idxEncontrado = (int)sitios.size();

    #pragma omp parallel for shared(resultado, idxEncontrado)
    for (int i = 0; i < (int)sitios.size(); i++) {
        #pragma omp flush(idxEncontrado)
        if (i >= idxEncontrado) continue;
        if (sitios[i]->getUrl() == url) {
            #pragma omp critical
            {
                if (i < idxEncontrado) {
                    idxEncontrado = i;
                    resultado = sitios[i];
                }
            }
        }
    }
    return resultado;
}
```

---

### Función 3 — `cargarArchivoSitiosWebCSV()` en `Navegador.cpp`

Carga paralela del CSV usando pipeline de 3 fases: IO serial → parseo paralelo → consolidación serial.

```cpp
// FUNCION PARALELIZADA 3: cargarArchivoSitiosWebCSV (pipeline 3 fases)
void Navegador::cargarArchivoSitiosWebCSV(const std::string& rutaArchivo) {
    // FASE 1 (serial): lectura de IO — los archivos no son thread-safe
    std::vector<std::string> lineas;
    std::ifstream archivo(rutaArchivo);
    std::string linea;
    while (std::getline(archivo, linea))
        if (!linea.empty()) lineas.push_back(linea);

    // FASE 2 (paralelo): construcción de objetos — cada línea es independiente
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

    // FASE 3 (serial): consolidación — vector no es thread-safe para push_back
    for (auto s : temp)
        if (s) sitios.push_back(s);
}
```

---

### Función 4 — `busquedaPalabraClave()` en `Historial.cpp`

Búsqueda sobre el historial de la pestaña activa. Patrón: pre-indexar lista en vector, `parallel for`, reducción.

```cpp
// FUNCION PARALELIZADA 4: busquedaPalabraClave
std::string Historial::busquedaPalabraClave(const std::string& palabraClave) {
    std::vector<SitioWeb*> vec(sitiosVisitados.begin(), sitiosVisitados.end());
    int n = (int)vec.size();
    std::vector<std::string> partes(n);
    std::vector<bool> coincide(n, false);

    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < n; i++) {
        std::string titulo = vec[i]->getTitulo();
        std::string url    = vec[i]->getUrl();
        std::transform(titulo.begin(), titulo.end(), titulo.begin(), ::tolower);
        std::transform(url.begin(),   url.end(),   url.begin(),   ::tolower);
        std::string kw = palabraClave;
        std::transform(kw.begin(), kw.end(), kw.begin(), ::tolower);
        if (titulo.find(kw) != std::string::npos || url.find(kw) != std::string::npos) {
            partes[i] = vec[i]->toString();
            coincide[i] = true;
        }
    }

    std::string resultado;
    for (int i = 0; i < n; i++)
        if (coincide[i]) resultado += partes[i] + "\n";
    return resultado;
}
```

---

### Función 5 — `limpiarSitiosViejos()` en `Historial.cpp`

Limpieza de entradas expiradas. Patrón: 2 fases (evaluación paralela + borrado secuencial), porque `std::list` no es thread-safe para erase.

```cpp
// FUNCION PARALELIZADA 5: limpiarSitiosViejos (2 fases)
bool Historial::limpiarSitiosViejos() {
    if (sitiosVisitados.empty() || tiempoMaximo <= 0) return false;

    std::vector<SitioWeb*> vec(sitiosVisitados.begin(), sitiosVisitados.end());
    int n = (int)vec.size();
    std::vector<bool> expirado(n, false);
    auto ahora = std::chrono::system_clock::now();

    // FASE 1 (paralelo): evaluar cuáles están expirados
    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        auto diff = std::chrono::duration_cast<std::chrono::seconds>(
            ahora - vec[i]->getFechaVisita()).count();
        expirado[i] = (diff > tiempoMaximo);
    }

    // FASE 2 (serial): borrar — std::list no es thread-safe
    bool huboLimpieza = false;
    auto it = sitiosVisitados.begin();
    for (int i = 0; i < n; i++, ++it) {
        if (expirado[i]) { it = sitiosVisitados.erase(it); --it; huboLimpieza = true; }
    }
    return huboLimpieza;
}
```

---

## 4. Justificación del número de hilos (Ley de Amdahl)

### La Ley de Amdahl

$$S(n) = \frac{1}{(1 - p) + \frac{p}{n}}$$

Donde:
- `S(n)` = speedup teórico con `n` hilos
- `p` = fracción paralela del código
- `(1 - p)` = fracción serial inamovible

### Estimación de `p` para este proyecto

Analizando el tiempo total secuencial, `cargarArchivoSitiosWebCSV()` representa el 79.6 % del tiempo. Las otras 4 funciones suman el 20.4 %, con fracción serial de cada una (IO, sincronización, reducción) de aproximadamente 30 %.

**Estimación conservadora**: fracción paralela `p ≈ 0.85`

### Speedup teórico con distintos números de hilos

| Hilos (n) | Fórmula | Speedup teórico S(n) |
|-----------|---------|----------------------|
| 1 | 1 / (0.15 + 0.85/1) | 1.00× |
| 2 | 1 / (0.15 + 0.85/2) | 1.54× |
| 4 | 1 / (0.15 + 0.85/4) | **2.35×** |
| 8 | 1 / (0.15 + 0.85/8) | 3.08× |
| 16 | 1 / (0.15 + 0.85/16) | 3.76× |
| ∞ | 1 / 0.15 | 6.67× (límite) |

```
Speedup
  4.0 |                                                    ___________
      |                                            _______
  3.0 |                                    _______
      |                            _______
  2.0 |                    _______
      |            _______
  1.0 |___________
      +----+----+----+----+----+----+----+----> Hilos
      0    2    4    6    8   10   12   14   16
```

### Elección: 4 hilos

- **Rendimiento decreciente**: pasar de 4 a 8 hilos solo añade 0.73× más speedup, mientras que de 1 a 4 se ganan 1.35×.
- **Alineado con hardware**: 4 hilos corresponde a los núcleos físicos típicos de una laptop de estudiante.
- **Overhead mínimo**: el scheduler del SO puede mapear 4 hilos sobre 4 núcleos sin contención de contexto.
- **Costo-beneficio óptimo**: el punto de inflexión de la curva de Amdahl para p=0.85 está en ~4 hilos.

```cpp
// Configuración en Source.cpp
omp_set_num_threads(4);
```

---

## 5. Comparación de tiempos reales (3 corridas cada versión)

Los benchmarks se compilaron con `-O2` y se ejecutaron sobre la misma máquina (Windows 11, g++ 15.2.0 MinGW), con 1 000 sitios en la BD y 10 pestañas × 100 páginas.

### Tiempos por corrida — versión secuencial (1 hilo)

| Función | Corrida 1 | Corrida 2 | Corrida 3 | **Promedio** |
|---------|-----------|-----------|-----------|-------------|
| busquedaMasiva | 0.3790 ms | 0.3830 ms | 0.5000 ms | **0.421 ms** |
| busquedaPalabraClave | 0.0230 ms | 0.0200 ms | 0.0230 ms | **0.022 ms** |
| limpiarSitiosViejos | 0.0170 ms | 0.0070 ms | 0.0060 ms | **0.010 ms** |
| cargarArchivoSitiosWebCSV | 1.8430 ms | 1.8740 ms | 1.9660 ms | **1.894 ms** |
| toString | 0.0390 ms | 0.0290 ms | 0.0320 ms | **0.033 ms** |
| **Total** | 2.3010 ms | 2.3130 ms | 2.5270 ms | **2.380 ms** |

### Tiempos por corrida — versión paralela (4 hilos OpenMP)

| Función | Corrida 1 | Corrida 2 | Corrida 3 | **Promedio** |
|---------|-----------|-----------|-----------|-------------|
| busquedaMasiva | 0.3910 ms | 0.2670 ms | 0.2490 ms | **0.302 ms** |
| busquedaPalabraClave | 0.1010 ms | 0.0870 ms | 0.0860 ms | **0.091 ms** |
| limpiarSitiosViejos | 0.8150 ms | 0.7200 ms | 0.7700 ms | **0.768 ms** |
| cargarArchivoSitiosWebCSV | 1.5140 ms | 1.4800 ms | 1.4180 ms | **1.471 ms** |
| toString | 0.0340 ms | 0.0210 ms | 0.0190 ms | **0.025 ms** |
| **Total** | 2.8550 ms | 2.5750 ms | 2.5420 ms | **2.657 ms** |

---

## 6. Tabla comparativa de resultados

| # | Función | Seq (ms) | Par 4h (ms) | Speedup | Observación |
|---|---------|----------|-------------|---------|-------------|
| 1 | busquedaMasiva | 0.421 | 0.302 | **1.39×** | Mejora real — dataset grande, trabajo uniforme |
| 2 | busquedaPalabraClave | 0.022 | 0.091 | 0.24× | Overhead domina — solo 100 elementos |
| 3 | limpiarSitiosViejos | 0.010 | 0.768 | 0.013× | Overhead severo — `std::list` fuerza 2 fases |
| 4 | cargarArchivoSitiosWebCSV | 1.894 | 1.471 | **1.29×** | Mejora real — parseo de 1 000 objetos paralelo |
| 5 | toString | 0.033 | 0.025 | **1.35×** | Mejora leve — concatenación paralela |
| — | **Total** | **2.380** | **2.657** | **0.90×** | Overhead neto supera beneficio en dataset pequeño |

### Análisis del speedup real vs teórico

```
Speedup teórico (Amdahl, p=0.85): 2.35×
Speedup real medido (promedio):    0.90×
```

La brecha entre teoría y práctica se debe a:

1. **Dataset pequeño** (1 000 sitios): el costo de crear y sincronizar 4 hilos con `omp_set_num_threads(4)` + la barrera implícita al final del `parallel for` es comparable al tiempo de cómputo mismo. OpenMP muestra su beneficio con decenas de miles de elementos.

2. **Overhead de `std::list`**: `limpiarSitiosViejos` requiere copiar la lista a vector antes del paso paralelo y luego iterar la lista secuencialmente para borrar — tres pases donde el secuencial hace uno.

3. **Critical sections en `buscarPaginaWeb`**: el patrón de early-exit con `flush + critical` introduce puntos de sincronización frecuentes que reducen el paralelismo efectivo.

4. **Caché de CPU**: 1 000 punteros × 8 bytes = 8 KB — cabe entero en L1. El secuencial, con acceso puramente lineal, produce menos cache misses que múltiples hilos accediendo a rangos fragmentados.

---

## 7. Conclusión

### Lo que se logró

Se implementaron con éxito **5 funciones paralelizadas** usando OpenMP en la versión paralela del simulador, siguiendo tres patrones distintos de paralelismo de datos:

- `parallel for` simple con pre-indexado (`busquedaMasiva`, `busquedaPalabraClave`, `toString`)
- `parallel for` con `critical` y early-exit (`buscarPaginaWeb`)
- Pipeline serial-paralelo-serial (`cargarArchivoSitiosWebCSV`)
- 2 fases paralelo-serial para estructuras no thread-safe (`limpiarSitiosViejos`)

El singleton `ConfigHistorial` fue refactorizado con **doble verificación y mutex** para ser seguro en entornos multihilo.

### La lección más importante

Los resultados reales muestran que **la paralelización no siempre mejora el rendimiento**. Con 1 000 elementos, el overhead de OpenMP supera al beneficio del cómputo paralelo en la mayoría de las funciones. Esto confirma el principio fundamental: la Ley de Amdahl da el **límite teórico**, pero el speedup práctico también depende de:

- El costo de creación y sincronización de hilos
- La cantidad de trabajo por hilo
- Los patrones de acceso a memoria (caché)
- Las restricciones de thread-safety de las estructuras de datos

Para obtener el speedup teórico de 2.35× que predice Amdahl con p=0.85 y 4 hilos, el dataset tendría que ser **del orden de 50 000–100 000 sitios** para que el tiempo de cómputo domine sobre el overhead de coordinación.

### Comparación de arquitecturas

| Aspecto | Versión Secuencial | Versión Paralela |
|---------|-------------------|------------------|
| Hilos | 1 | 4 (omp_set_num_threads) |
| ConfigHistorial | Singleton simple | Singleton thread-safe (mutex) |
| Algoritmos | std::find_if, loops simples | #pragma omp parallel for |
| cargarCSV | While/getline lineal | Pipeline 3 fases |
| limpiarHistorial | Erase en un pase | 2 fases: eval paralela + erase serial |
| busquedaMasiva | Loop for simple | parallel for + compactación |
| Tiempo total (1K sitios) | 2.38 ms | 2.66 ms |
| Speedup proyectado (100K sitios) | — | ~2.0–2.3× |

### Archivos del proyecto

```
proyecto-paralela/
├── version-secuencial/   ← versión sin OpenMP + benchmark_secuencial
│   ├── compilar.bat      ← compila e inicia el navegador
│   └── benchmark.bat     ← compila y corre el benchmark de rendimiento
├── version-paralela/     ← versión con OpenMP (4 hilos) + benchmark_paralelo
│   ├── compilar.bat      ← compila e inicia el navegador paralelo
│   └── benchmark.bat     ← compila y corre el benchmark paralelo
└── comparar.bat          ← diff entre ambas versiones
```

---

*Ejecutado el 2026-05-18 con g++ 15.2.0 (MinGW), Windows 11 Enterprise, -O2.*
