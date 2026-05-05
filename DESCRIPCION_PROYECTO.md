# Simulador de Navegador Web - Descripcion del Proyecto

## 1. Informacion General

- **Nombre del proyecto:** Proyecto1Datos - Simulador de Navegador Web
- **Lenguaje:** C++ (C++14)
- **Plataforma:** Windows (usa Windows API para entrada de consola)
- **IDE original:** Visual Studio 2022 (MSVC v143)
- **Compilador alternativo:** g++ (MinGW)
- **Framework de pruebas:** Google Test (gtest)

---

## 2. Descripcion Funcional

El programa simula un **navegador web de consola** con las siguientes funcionalidades:

### 2.1 Navegacion por pestanias
- Crear nuevas pestanias (normales o incognito)
- Navegar entre pestanias usando flechas arriba/abajo
- Cada pestania normal mantiene su propio historial independiente
- Las pestanias incognito solo almacenan el sitio actual (sin historial)

### 2.2 Navegacion por paginas web
- Ingresar URLs para visitar sitios web (de una base de datos CSV con 16 sitios precargados)
- Navegar hacia atras y adelante en el historial con flechas izquierda/derecha
- Cada visita registra la URL, titulo, dominio y timestamp

### 2.3 Sistema de historial
- Almacena todas las paginas visitadas en orden cronologico
- Navegacion bidireccional (adelante/atras) mediante iteradores sobre std::list
- Elimina duplicados automaticamente al revisitar un sitio
- Configuracion de limite maximo de entradas
- Configuracion de tiempo maximo (entradas viejas se eliminan automaticamente)
- La limpieza por tiempo se ejecuta en tiempo real dentro del bucle principal

### 2.4 Marcadores (Bookmarks)
- Guardar el sitio actual como marcador con una etiqueta (tag)
- Si el sitio ya tiene marcador, se anade una nueva etiqueta al existente
- Visualizar todos los marcadores guardados
- No disponible en modo incognito

### 2.5 Busqueda y filtros
- **Busqueda por palabra clave:** busca coincidencias en los titulos de los sitios del historial (case-insensitive)
- **Filtro de navegacion:** aplica un filtro que restringe la navegacion adelante/atras solo a sitios que coincidan con el texto del filtro
- **Eliminar filtro:** remueve el filtro activo

### 2.6 Modo incognito
- Al activarse, se crea un espacio de pestanias separado
- No se guarda historial ni se permiten marcadores
- Al desactivar, se eliminan todas las pestanias incognito

### 2.7 Importar/Exportar sesion
- **Exportar:** guarda el estado completo del navegador (pestanias, historial, marcadores, configuracion) en un archivo binario (.bin)
- **Importar:** carga un archivo binario y reemplaza el navegador actual con la sesion guardada

### 2.8 Configuracion de historial
- Establecer cantidad maxima de entradas por historial
- Establecer tiempo maximo en segundos (las entradas mas viejas se eliminan automaticamente)
- Reiniciar configuraciones a valores por defecto

---

## 3. Arquitectura del Software

### 3.1 Patron de diseno: MVC (Modelo-Vista-Controlador)

```
                    +------------------+
                    |   Source.cpp     |  Punto de entrada (main)
                    +--------+---------+
                             |
                    +--------v---------+
                    |  Controladora    |  Enruta input del usuario
                    +--------+---------+
                             |
              +--------------+--------------+
              |                             |
    +---------v----------+       +----------v---------+
    |     Interfaz       |       |     Navegador      |
    |  (Vista - UI)      |       |  (Logica central)  |
    +--------------------+       +----------+---------+
                                            |
                         +------------------+------------------+
                         |                  |                  |
              +----------v---+    +---------v------+   +-------v--------+
              | ListPestanias|    |   Marcador     |   | ConfigHistorial|
              +------+-------+    +-------+--------+   |   (Singleton)  |
                     |                    |            +----------------+
          +----------+----------+         |
          |                     |    +----v------+
   +------v------+    +---------v-+  |  SitioWeb  |
   |  Pestania   |    | Pestania  |  +-----------+
   |  (normal)   |    | Incognito |
   +------+------+    +-----------+
          |
   +------v------+
   |  Historial  |
   +------+------+
          |
   +------v------+
   |  SitioWeb   |
   +-------------+
```

### 3.2 Clases y responsabilidades

| Clase | Archivo(s) | Responsabilidad |
|-------|-----------|-----------------|
| `SitioWeb` | SitioWeb.h/cpp | Modelo de datos: URL, titulo, dominio, timestamp |
| `Historial` | Historial.h/cpp | Lista enlazada de sitios visitados con navegacion bidireccional, filtros y limpieza |
| `PestaniaAbstracta` | PestaniaAbstracta.h | Clase base abstracta que define la interfaz de una pestania |
| `Pestania` | Pestania.h/cpp | Pestania normal que contiene un Historial completo |
| `PestaniaIncognito` | PestaniaIncognito.h/cpp | Pestania sin historial, solo almacena un sitio |
| `ListPestanias` | ListaPestanias.h/cpp | Gestiona la lista de pestanias con navegacion |
| `Marcador` | Marcador.h/cpp | Sitio web favorito con lista de etiquetas |
| `ConfigHistorial` | ConfigHistorial.h/cpp | Singleton para configuraciones globales del historial |
| `Navegador` | Navegador.h/cpp | Controlador principal: coordina pestanias, marcadores, sitios y config |
| `Interfaz` | Interfaz.h/cpp | Capa de presentacion: menus de consola y captura de teclado (Windows API) |
| `Controladora` | Controladora.h/cpp | Enruta las acciones del usuario a la logica correspondiente |
| `ExcepcionGenerica` | Excepciones.h/cpp | Clase de excepcion personalizada para manejo de errores |

### 3.3 Patrones de diseno utilizados

1. **Herencia y polimorfismo:** `PestaniaAbstracta` -> `Pestania` / `PestaniaIncognito`
2. **Singleton:** `ConfigHistorial` (una unica instancia de configuracion global)
3. **Composicion:** `Navegador` contiene `ListPestanias`, `vector<SitioWeb*>`, `list<Marcador*>`
4. **Delegacion:** `Controladora` delega a `Interfaz`, que delega a `Navegador`

### 3.4 Estructuras de datos utilizadas

- **`std::list<SitioWeb*>`** - Historial de navegacion (permite insercion/eliminacion O(1) y navegacion con iteradores)
- **`std::list<PestaniaAbstracta*>`** - Lista de pestanias
- **`std::list<Marcador*>`** - Lista de marcadores
- **`std::list<std::string>`** - Etiquetas de cada marcador
- **`std::vector<SitioWeb*>`** - Base de datos de sitios web disponibles (acceso O(1) por indice)
- **Iteradores** - Para posicion actual tanto en historial como en pestanias

---

## 4. Flujo de Ejecucion

1. `main()` crea una `Controladora`
2. La `Controladora` crea un `Navegador`, que: carga los 16 sitios web del CSV, inicializa listas de pestanias vacias, obtiene la instancia del Singleton de configuracion
3. Se invoca `control0()` que entra en el bucle principal
4. El bucle principal (`Interfaz::detectarTecla`) usa la Windows API para capturar teclas en tiempo real
5. Dentro del bucle, se verifica continuamente si hay sitios viejos para limpiar
6. Segun la tecla presionada, se invoca el metodo de control correspondiente (control1-control13)
7. El ciclo se repite hasta que el usuario presiona '9' (Salir)

---

## 5. Persistencia de Datos

### Formatos de archivo:
- **CSV** (`sitiosWeb.csv`): Base de datos de sitios web precargados (URL, titulo, dominio)
- **Binario** (`.bin`): Exportacion/importacion de sesiones completas

### Serializacion binaria:
Cada clase implementa metodos `guardarArchivo*()` y `cargarArchivo*()` que escriben/leen los campos en formato binario usando `ofstream::write` y `ifstream::read`.

---

## 6. Entrada del Usuario

La deteccion de teclas usa la **Windows Console API**:
- `GetStdHandle(STD_INPUT_HANDLE)` - obtiene handle de la consola
- `GetNumberOfConsoleInputEvents()` - verifica si hay eventos pendientes
- `PeekConsoleInput()` - lee eventos sin consumirlos
- `FlushConsoleInputBuffer()` - limpia el buffer al detectar tecla valida

**Teclas soportadas:**
| Tecla | Accion |
|-------|--------|
| Flecha izquierda | Pagina anterior |
| Flecha derecha | Pagina siguiente |
| Flecha arriba | Pestania siguiente |
| Flecha abajo | Pestania anterior |
| 1-9 | Opciones del menu |

---

## 7. Archivos del Proyecto

### Archivos activos (usados en compilacion):
```
Source.cpp              - Punto de entrada (main)
Controladora.h/cpp      - Controlador de flujo
Navegador.h/cpp         - Logica principal del navegador
Interfaz.h/cpp          - Interfaz de usuario (consola)
Historial.h/cpp         - Gestion del historial
ListaPestanias.h/cpp    - Gestion de lista de pestanias
Pestania.h/cpp          - Pestania normal
PestaniaAbstracta.h     - Clase abstracta base
PestaniaIncognito.h/cpp - Pestania incognito
SitioWeb.h/cpp          - Modelo de sitio web
Marcador.h/cpp          - Modelo de marcador
ConfigHistorial.h/cpp   - Configuracion singleton
Excepciones.h/cpp       - Excepciones personalizadas
sitiosWeb.csv           - Base de datos de sitios web
```

### Archivos huerfanos/sin uso:
```
Sesion.cpp              - Clase vacia, no tiene header, no se usa en ningun lado
AdminPestanias.h        - Archivo vacio (1 linea)
ListPestanias.cpp       - Archivo vacio (duplicado de ListaPestanias.cpp)
```

### Pruebas:
```
UnitTest/test.cpp       - Suite de pruebas con Google Test
```

---

## 8. Base de Datos de Sitios Web (sitiosWeb.csv)

El programa viene con 16 sitios web precargados:

| URL | Titulo | Dominio |
|-----|--------|---------|
| https://www.google.com | Google | google.com |
| https://www.wikipedia.org | Wikipedia | wikipedia.org |
| https://www.github.com | GitHub | github.com |
| https://www.stackoverflow.com | Stack Overflow | stackoverflow.com |
| https://www.reddit.com | Reddit | reddit.com |
| https://www.linkedin.com | LinkedIn | linkedin.com |
| https://www.youtube.com | YouTube | youtube.com |
| https://www.amazon.com | Amazon | amazon.com |
| https://www.twitter.com | Twitter | twitter.com |
| https://www.netflix.com | Netflix | netflix.com |
| https://www.facebook.com | Facebook | facebook.com |
| https://www.instagram.com | Instagram | instagram.com |
| https://www.microsoft.com | Microsoft | microsoft.com |
| https://www.apple.com | Apple | apple.com |
| https://www.nytimes.com | The New York Times | nytimes.com |
| https://www.twitch.tv | Twitch | twitch.tv |
