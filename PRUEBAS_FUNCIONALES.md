# Plan de Pruebas Funcionales - Simulador de Navegador Web

## Instrucciones Generales
- Ejecutar `compilar_y_ejecutar.bat` desde CMD (doble clic o terminal)
- Seguir cada caso de prueba en orden
- Marcar con [PASS] o [FAIL] cada verificacion
- Las teclas de navegacion son: flechas (izq/der/arriba/abajo) y numeros (1-9)

---

## CP-01: Inicio del Programa
**Objetivo:** Verificar que el programa arranca correctamente

| Paso | Accion | Resultado Esperado | Estado |
|------|--------|--------------------|--------|
| 1 | Ejecutar `compilar_y_ejecutar.bat` | Compila sin errores | [ ] |
| 2 | Observar la pantalla inicial | Se muestra "Modo incognito desactivado" | [ ] |
| 3 | Verificar mensaje de pestanias | Se muestra "No hay pestanias" | [ ] |
| 4 | Verificar menu | Se muestran las 9 opciones del menu | [ ] |

---

## CP-02: Crear Pestanias
**Objetivo:** Verificar la creacion de pestanias nuevas

| Paso | Accion | Resultado Esperado | Estado |
|------|--------|--------------------|--------|
| 1 | Presionar tecla `7` | Se crea una pestania nueva | [ ] |
| 2 | Verificar pantalla | Muestra "Pestania 1" y "No hay sitio actual disponible" | [ ] |
| 3 | Presionar `7` otra vez | Se crea segunda pestania | [ ] |
| 4 | Verificar pantalla | Muestra "Pestania 2" | [ ] |

---

## CP-03: Navegar a un Sitio Web
**Objetivo:** Verificar la navegacion a sitios web del CSV

| Paso | Accion | Resultado Esperado | Estado |
|------|--------|--------------------|--------|
| 1 | Presionar `1` | Aparece "Ingrese la URL:" | [ ] |
| 2 | Escribir `https://www.google.com` y Enter | Muestra URL: google, Titulo: Google, Dominio: google.com | [ ] |
| 3 | Presionar una tecla para continuar | Regresa al menu principal | [ ] |
| 4 | Presionar `1` de nuevo | Aparece "Ingrese la URL:" | [ ] |
| 5 | Escribir `https://www.youtube.com` y Enter | Muestra YouTube en la pestania | [ ] |
| 6 | Presionar `1` de nuevo | Aparece "Ingrese la URL:" | [ ] |
| 7 | Escribir `https://www.github.com` y Enter | Muestra GitHub en la pestania | [ ] |

---

## CP-04: URL Invalida (Error 404)
**Objetivo:** Verificar manejo de URLs no existentes

| Paso | Accion | Resultado Esperado | Estado |
|------|--------|--------------------|--------|
| 1 | Presionar `1` | Aparece "Ingrese la URL:" | [ ] |
| 2 | Escribir `https://www.noexiste.com` y Enter | Muestra "Error: 404 - Not Found" | [ ] |
| 3 | Presionar una tecla | Regresa al menu, la pestania no cambia | [ ] |

---

## CP-05: Navegacion Adelante/Atras en Historial
**Objetivo:** Verificar navegacion entre paginas visitadas
**Prerrequisito:** Haber completado CP-03 (tener Google, YouTube, GitHub en historial)

| Paso | Accion | Resultado Esperado | Estado |
|------|--------|--------------------|--------|
| 1 | Presionar `Flecha Izquierda` | Cambia a YouTube (pagina anterior) | [ ] |
| 2 | Presionar `Flecha Izquierda` | Cambia a Google (pagina anterior) | [ ] |
| 3 | Presionar `Flecha Izquierda` | Se queda en Google (no hay mas atras) | [ ] |
| 4 | Presionar `Flecha Derecha` | Cambia a YouTube (pagina siguiente) | [ ] |
| 5 | Presionar `Flecha Derecha` | Cambia a GitHub (pagina siguiente) | [ ] |
| 6 | Presionar `Flecha Derecha` | Se queda en GitHub (no hay mas adelante) | [ ] |

---

## CP-06: Navegacion entre Pestanias
**Objetivo:** Verificar cambio entre pestanias
**Prerrequisito:** Crear al menos 2 pestanias con `7`

| Paso | Accion | Resultado Esperado | Estado |
|------|--------|--------------------|--------|
| 1 | Presionar `7` para crear nueva pestania | Se crea pestania nueva (vacia) | [ ] |
| 2 | Presionar `1`, escribir `https://www.reddit.com` | Pestania nueva muestra Reddit | [ ] |
| 3 | Presionar `Flecha Abajo` | Cambia a pestania anterior (GitHub) | [ ] |
| 4 | Verificar pantalla | Se muestra el contenido de la pestania anterior | [ ] |
| 5 | Presionar `Flecha Arriba` | Regresa a la pestania con Reddit | [ ] |

---

## CP-07: Agregar Bookmarks
**Objetivo:** Verificar el sistema de marcadores
**Prerrequisito:** Estar en una pestania con un sitio cargado

| Paso | Accion | Resultado Esperado | Estado |
|------|--------|--------------------|--------|
| 1 | Navegar a Google (si no esta) | Muestra Google | [ ] |
| 2 | Presionar `2` | Aparece "Ingrese un tag:" | [ ] |
| 3 | Escribir `buscador` y Enter | Se agrega el bookmark | [ ] |
| 4 | Presionar `4` | Muestra lista de bookmarks | [ ] |
| 5 | Verificar | Aparece Google con etiqueta "buscador" | [ ] |

---

## CP-08: Agregar Etiqueta a Bookmark Existente
**Objetivo:** Verificar que se puede anadir otra etiqueta al mismo bookmark

| Paso | Accion | Resultado Esperado | Estado |
|------|--------|--------------------|--------|
| 1 | Estar en Google (mismo sitio del CP-07) | Muestra Google | [ ] |
| 2 | Presionar `2` | Aparece "Ingrese un tag:" | [ ] |
| 3 | Escribir `favorito` y Enter | Se agrega etiqueta al bookmark existente | [ ] |
| 4 | Presionar `4` | Muestra bookmarks | [ ] |
| 5 | Verificar | Google aparece con etiquetas "buscador" y "favorito" | [ ] |

---

## CP-09: Busqueda por Palabra Clave
**Objetivo:** Verificar la busqueda en el historial
**Prerrequisito:** Tener varios sitios en el historial

| Paso | Accion | Resultado Esperado | Estado |
|------|--------|--------------------|--------|
| 1 | Presionar `5` | Muestra menu de busqueda y filtros | [ ] |
| 2 | Presionar `2` | Aparece "Ingrese la palabra clave:" | [ ] |
| 3 | Escribir `google` y Enter | Muestra "COINCIDENCIA # 1" con datos de Google | [ ] |
| 4 | Presionar una tecla | Regresa al menu de busqueda | [ ] |
| 5 | Presionar `2` | Aparece "Ingrese la palabra clave:" | [ ] |
| 6 | Escribir `GOOGLE` (mayusculas) y Enter | Misma coincidencia (busqueda case-insensitive) | [ ] |
| 7 | Presionar `4` para regresar | Regresa al menu principal | [ ] |

---

## CP-10: Filtro de Navegacion
**Objetivo:** Verificar que el filtro restringe la navegacion
**Prerrequisito:** Historial con Google, YouTube, GitHub

| Paso | Accion | Resultado Esperado | Estado |
|------|--------|--------------------|--------|
| 1 | Presionar `5` | Menu de busqueda y filtros | [ ] |
| 2 | Presionar `1` | Aparece "Ingrese la palabra clave para aplicarle el filtro:" | [ ] |
| 3 | Escribir `oo` y Enter | Se aplica filtro (coincide con Google y YouTube) | [ ] |
| 4 | Presionar `4` para regresar al menu principal | Regresa al menu | [ ] |
| 5 | Presionar `Flecha Izquierda` / `Flecha Derecha` | Solo navega entre sitios que contengan "oo" | [ ] |
| 6 | Presionar `5` luego `3` | Elimina el filtro | [ ] |
| 7 | Presionar `4` para regresar | Regresa al menu | [ ] |
| 8 | Presionar flechas | Navega entre todos los sitios sin restriccion | [ ] |

---

## CP-11: Modo Incognito
**Objetivo:** Verificar el modo incognito

| Paso | Accion | Resultado Esperado | Estado |
|------|--------|--------------------|--------|
| 1 | Presionar `6` | Se activa modo incognito | [ ] |
| 2 | Verificar pantalla | Muestra "Modo incognito activado" y "No hay pestanias" | [ ] |
| 3 | Presionar `7` | Crea pestania incognito | [ ] |
| 4 | Presionar `1`, escribir `https://www.netflix.com` | Muestra Netflix | [ ] |
| 5 | Presionar `1`, escribir `https://www.apple.com` | Muestra Apple (reemplaza, sin historial) | [ ] |
| 6 | Presionar `Flecha Izquierda` | NO retrocede (incognito no tiene historial) | [ ] |
| 7 | Presionar `6` | Desactiva modo incognito | [ ] |
| 8 | Verificar | Regresa al modo normal, pestanias originales intactas | [ ] |

---

## CP-12: Restricciones en Modo Incognito
**Objetivo:** Verificar que bookmarks, busqueda y configuracion estan bloqueados

| Paso | Accion | Resultado Esperado | Estado |
|------|--------|--------------------|--------|
| 1 | Presionar `6` para activar incognito | Modo incognito activado | [ ] |
| 2 | Presionar `7` y luego `1` con una URL | Navegar a un sitio | [ ] |
| 3 | Presionar `2` (agregar bookmark) | Error: "No se pueden agregar bookmarks en modo incognito" | [ ] |
| 4 | Presionar `4` (mostrar bookmarks) | Error: "No se pueden mostrar bookmarks en modo incognito" | [ ] |
| 5 | Presionar `5` (busqueda y filtros) | Error: "No se pueden hacer busquedas ni filtros en modo incognito" | [ ] |
| 6 | Presionar `8` (configuracion) | Error: "No se puede configurar en modo incognito" | [ ] |
| 7 | Presionar `3` (importar/exportar) | Error: "No se pueden importar/exportar en modo incognito" | [ ] |
| 8 | Presionar `6` para desactivar incognito | Regresa a modo normal | [ ] |

---

## CP-13: Configuracion - Limite de Entradas del Historial
**Objetivo:** Verificar que el limite de entradas funciona

| Paso | Accion | Resultado Esperado | Estado |
|------|--------|--------------------|--------|
| 1 | Presionar `8` | Menu de configuracion | [ ] |
| 2 | Presionar `1` | Aparece "Ingrese la cantidad de entradas:" | [ ] |
| 3 | Escribir `2` y Enter | Se configura el limite a 2 | [ ] |
| 4 | Presionar `4` para regresar | Regresa al menu principal | [ ] |
| 5 | Navegar a 3 sitios diferentes (1+URL x3) | Solo quedan los 2 mas recientes en historial | [ ] |
| 6 | Verificar con flechas izq/der | Solo se puede navegar entre 2 sitios | [ ] |
| 7 | Presionar `8`, luego `3` | Reiniciar configuraciones | [ ] |
| 8 | Presionar `4` para regresar | Limite removido | [ ] |

---

## CP-14: Configuracion - Tiempo Maximo del Historial
**Objetivo:** Verificar que las entradas viejas se eliminan por tiempo

| Paso | Accion | Resultado Esperado | Estado |
|------|--------|--------------------|--------|
| 1 | Presionar `8` | Menu de configuracion | [ ] |
| 2 | Presionar `2` | Aparece "Ingrese la cantidad de segundos:" | [ ] |
| 3 | Escribir `5` y Enter | Tiempo maximo = 5 segundos | [ ] |
| 4 | Presionar `4` para regresar | Regresa al menu principal | [ ] |
| 5 | Navegar a un sitio (1 + URL) | Se carga el sitio | [ ] |
| 6 | Esperar ~6 segundos | El sitio debe desaparecer del historial automaticamente | [ ] |
| 7 | Verificar pantalla | Muestra "No hay sitio actual disponible" | [ ] |
| 8 | Presionar `8`, luego `3` | Reiniciar configuraciones | [ ] |

---

## CP-15: Configuracion - Validacion de Entrada Invalida
**Objetivo:** Verificar manejo de entradas incorrectas

| Paso | Accion | Resultado Esperado | Estado |
|------|--------|--------------------|--------|
| 1 | Presionar `8`, luego `1` | Pide cantidad de entradas | [ ] |
| 2 | Escribir `0` y Enter | Error: "La cantidad de entradas no puede ser negativa ni 0" | [ ] |
| 3 | Presionar `1` de nuevo | Pide cantidad de entradas | [ ] |
| 4 | Escribir `-5` y Enter | Error: "La cantidad de entradas no puede ser negativa ni 0" | [ ] |
| 5 | Presionar `4` para regresar | Regresa al menu principal | [ ] |

---

## CP-16: Exportar Sesion
**Objetivo:** Verificar la exportacion a archivo binario
**Prerrequisito:** Tener pestanias con historial y bookmarks

| Paso | Accion | Resultado Esperado | Estado |
|------|--------|--------------------|--------|
| 1 | Presionar `3` | Menu importar/exportar | [ ] |
| 2 | Presionar `1` | Aparece "Ingrese el nombre con el que quiere guardar:" | [ ] |
| 3 | Escribir `mi_sesion` y Enter | Muestra "Se ha exportado el historial con exito" | [ ] |
| 4 | Verificar | Se crea archivo `mi_sesion.bin` en la carpeta del proyecto | [ ] |
| 5 | Presionar `3` para regresar | Regresa al menu principal | [ ] |

---

## CP-17: Importar Sesion
**Objetivo:** Verificar la importacion desde archivo binario
**Prerrequisito:** Haber exportado en CP-16

| Paso | Accion | Resultado Esperado | Estado |
|------|--------|--------------------|--------|
| 1 | Presionar `3` | Menu importar/exportar | [ ] |
| 2 | Presionar `2` | Aparece "Ingrese el nombre de la sesion:" | [ ] |
| 3 | Escribir `mi_sesion` y Enter | Muestra "Se ha importado el historial con exito" | [ ] |
| 4 | Presionar `3` para regresar | Regresa al menu principal | [ ] |
| 5 | Verificar pestanias e historial | Debe coincidir con lo exportado | [ ] |

---

## CP-18: Importar Sesion Inexistente
**Objetivo:** Verificar manejo de error al importar archivo que no existe

| Paso | Accion | Resultado Esperado | Estado |
|------|--------|--------------------|--------|
| 1 | Presionar `3` | Menu importar/exportar | [ ] |
| 2 | Presionar `2` | Pide nombre de sesion | [ ] |
| 3 | Escribir `no_existe` y Enter | Error: "No se ha podido abrir el archivo" | [ ] |
| 4 | Presionar `3` para regresar | Regresa al menu, navegador sin cambios | [ ] |

---

## CP-19: Salir del Programa
**Objetivo:** Verificar salida limpia

| Paso | Accion | Resultado Esperado | Estado |
|------|--------|--------------------|--------|
| 1 | Presionar `9` | Muestra "GRACIAS POR NAVEGAR CON NOSOTROS :)" | [ ] |
| 2 | Verificar | El programa termina sin errores | [ ] |

---

## CP-20: Flujo Completo de Estres
**Objetivo:** Prueba de integracion - usar todas las funciones en secuencia

| # | Accion | Verificar |
|---|--------|-----------|
| 1 | Ejecutar programa | Menu visible |
| 2 | Crear 3 pestanias (`7` x3) | "Pestania 3" visible |
| 3 | En pestania 3: navegar a Google, YouTube, GitHub | GitHub visible |
| 4 | Flecha abajo -> pestania 2 | Pestania 2 esta vacia |
| 5 | Navegar a Reddit, Netflix, Amazon | Amazon visible |
| 6 | Flecha abajo -> pestania 1 | Pestania 1 esta vacia |
| 7 | Navegar a Facebook, Twitter | Twitter visible |
| 8 | Agregar bookmark a Twitter con tag "social" (`2`) | Se agrega |
| 9 | Flecha arriba -> pestania 2 | Amazon visible |
| 10 | Flecha izquierda x2 | Llega a Reddit |
| 11 | Agregar bookmark con tag "foro" | Se agrega |
| 12 | Ver bookmarks (`4`) | Twitter(social) y Reddit(foro) |
| 13 | Buscar "oo" (`5` -> `2`) | Coincidencias con Google y YouTube (de pestania 3) o Facebook (pestania 1) |
| 14 | Configurar limite 2 entradas (`8` -> `1` -> `2`) | Se aplica |
| 15 | Verificar historial con flechas | Solo 2 entradas por pestania |
| 16 | Exportar sesion (`3` -> `1` -> `test_estres`) | Exporta sin error |
| 17 | Activar incognito (`6`) | Modo incognito |
| 18 | Crear pestania, navegar a Apple | Apple visible |
| 19 | Intentar bookmark (`2`) | Error: no se puede en incognito |
| 20 | Desactivar incognito (`6`) | Regresa a modo normal, datos intactos |
| 21 | Reiniciar config (`8` -> `3`) | Configs reiniciadas |
| 22 | Importar sesion (`3` -> `2` -> `test_estres`) | Importa la sesion guardada |
| 23 | Verificar pestanias e historial | Coincide con lo exportado |
| 24 | Salir (`9`) | Mensaje de despedida, cierre limpio |

---

## Resumen de Cobertura

| Funcionalidad | Casos de Prueba |
|---------------|-----------------|
| Inicio del programa | CP-01 |
| Crear pestanias | CP-02 |
| Navegar a sitios web | CP-03, CP-04 |
| Historial adelante/atras | CP-05 |
| Cambio entre pestanias | CP-06 |
| Bookmarks | CP-07, CP-08 |
| Busqueda por palabra clave | CP-09 |
| Filtro de navegacion | CP-10 |
| Modo incognito | CP-11, CP-12 |
| Config limite entradas | CP-13, CP-15 |
| Config tiempo maximo | CP-14 |
| Exportar sesion | CP-16 |
| Importar sesion | CP-17, CP-18 |
| Salir | CP-19 |
| Integracion completa | CP-20 |
