/*
 * benchmark2.cpp
 * ---------------------------------------------------------------
 * Funciones con tiempos significativos (rango de milisegundos)
 * para demostrar ganancia real al paralelizar.
 *
 * Compilar (desde Proyecto1Datos/):
 *   g++ -std=c++14 -O2 -o benchmark2.exe benchmark2.cpp ^
 *       Historial.cpp SitioWeb.cpp ConfigHistorial.cpp Excepciones.cpp -I.
 *
 * Requiere: sitiosWeb_1000.csv en el mismo directorio
 * ---------------------------------------------------------------
 *
 * Las 5 funciones elegidas y por que son lentas:
 *
 *  F6 — busquedaPalabraClave en 50 pestanas simultaneas
 *        → 50 × 1000 entradas = 50,000 tolower() + find() por corrida
 *
 *  F7 — Deteccion de duplicados O(n²)
 *        → 1000 × 1000 = 1,000,000 comparaciones de strings
 *
 *  F8 — Ordenamiento masivo del historial (100 veces)
 *        → 100 × sort(1000) con comparacion de strings completos
 *
 *  F9 — Generacion de reporte toString() completo (100 veces)
 *        → 100 × 1000 = 100,000 llamadas a toString() + concatenacion
 *
 *  F10 — Serializacion + deserializacion binaria en archivo
 *        → Escritura y lectura de 1000 SitioWeb al disco
 * ---------------------------------------------------------------
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <list>
#include <string>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <cstdio>

#include "SitioWeb.h"
#include "Historial.h"
#include "ConfigHistorial.h"

// ─────────────────────────────────────────────────────────────────
// Temporizador
// ─────────────────────────────────────────────────────────────────
using Clock = std::chrono::high_resolution_clock;
using Ns    = std::chrono::nanoseconds;

template<typename F>
long long medir(F&& fn) {
    auto t0 = Clock::now();
    fn();
    return std::chrono::duration_cast<Ns>(Clock::now() - t0).count();
}

// ─────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────
static void sep(char c = '-', int n = 64) {
    std::cout << std::string(n, c) << '\n';
}

static std::string fmtNs(long long ns) {
    std::ostringstream o;
    if      (ns >= 1000000LL) o << (ns/1000000LL) << " ms  (" << (ns/1000LL) << " us)";
    else if (ns >= 1000LL)    o << (ns/1000LL)    << " us";
    else                      o << ns              << " ns";
    return o.str();
}

// ─────────────────────────────────────────────────────────────────
// Carga de CSV
// ─────────────────────────────────────────────────────────────────
std::vector<SitioWeb*> cargarCSV(const std::string& ruta) {
    std::vector<SitioWeb*> sitios;
    std::ifstream arch(ruta);
    if (!arch.is_open()) return sitios;
    std::string linea;
    while (std::getline(arch, linea)) {
        if (linea.empty()) continue;
        if (linea.back() == '\r') linea.pop_back();
        std::istringstream ss(linea);
        std::string url, titulo, dominio;
        if (std::getline(ss, url, ',') &&
            std::getline(ss, titulo, ',') &&
            std::getline(ss, dominio))
        {
            sitios.push_back(new SitioWeb(url, titulo, dominio));
        }
    }
    return sitios;
}

// ─────────────────────────────────────────────────────────────────
// Resultado
// ─────────────────────────────────────────────────────────────────
struct Resultado {
    std::string nombre;
    long long   ns_promedio;
    std::string por_que_es_lenta;
    std::string estrategia_paralela;
};

// ═══════════════════════════════════════════════════════════════════
int main() {
    const std::string CSV    = "sitiosWeb_1000.csv";
    const int         REPS   = 10;
    const int         N_TABS = 50;    // pestanas para F6

    sep('=');
    std::cout << "  BENCHMARK 2 — Funciones con tiempos significativos\n";
    std::cout << "  Dataset: 1000 sitios  |  " << N_TABS
              << " pestanas  |  Reps: " << REPS << "\n";
    sep('=');

    // ── Carga inicial ─────────────────────────────────────────────
    std::cout << "\n[*] Cargando " << CSV << " ...\n";
    std::vector<SitioWeb*> sitios = cargarCSV(CSV);
    if (sitios.empty()) {
        std::cerr << "[ERROR] No se cargaron sitios.\n";
        return 1;
    }
    std::cout << "[*] " << sitios.size() << " sitios cargados\n";

    ConfigHistorial::getInstancia()->setTiempoMaximo(999999);

    // ── Historial base (1 pestaña, 1000 entradas) ─────────────────
    Historial* histBase = new Historial();
    for (auto* s : sitios) histBase->add(s);
    std::cout << "[*] Historial base: " << histBase->size() << " entradas\n";

    // ── 50 pestanas, cada una con 1000 entradas ───────────────────
    std::cout << "[*] Creando " << N_TABS << " pestanas con 1000 entradas c/u...\n";
    std::vector<Historial*> pestanas;
    pestanas.reserve(N_TABS);
    for (int i = 0; i < N_TABS; ++i) {
        auto* h = new Historial();
        for (auto* s : sitios) h->add(s);
        pestanas.push_back(h);
    }
    std::cout << "[*] " << N_TABS << " pestanas listas ("
              << N_TABS * 1000 << " entradas en total)\n\n";

    std::vector<Resultado> resultados;

    // ═══════════════════════════════════════════════════════════════
    // F6: busquedaPalabraClave en 50 pestanas simultaneas
    // ---------------------------------------------------------------
    // Hace busquedaPalabraClave(keyword) en las 50 pestanas.
    // Cada llamada recorre 1000 entradas con tolower() + find().
    // Total por corrida: 50 × 1000 = 50,000 operaciones de busqueda.
    // Paralelizar: cada pestaña es independiente → 50 tareas paralelas.
    // ═══════════════════════════════════════════════════════════════
    {
        sep();
        std::cout << "[F6] busquedaPalabraClave en " << N_TABS << " pestanas\n";
        std::cout << "     " << N_TABS << " historiales × 1000 entradas = "
                  << N_TABS * 1000 << " operaciones por corrida\n";

        const std::vector<std::string> kws = {
            "portal","tecnologia","noticias","educacion","entretenimiento",
            "compras","social","gobierno","salud","ciencia"
        };
        long long total = 0;
        std::string last;

        for (int r = 0; r < REPS; ++r) {
            total += medir([&]{
                for (int t = 0; t < N_TABS; ++t) {
                    last = pestanas[t]->busquedaPalabraClave(
                               kws[t % (int)kws.size()]);
                }
            });
        }
        long long prom = total / REPS;
        std::cout << "     Promedio por corrida : " << fmtNs(prom) << "\n";
        std::cout << "     Con 4 hilos esperado : ~" << fmtNs(prom / 4) << "\n";

        resultados.push_back({
            "F6: busquedaPalabraClave (50 tabs × 1000)",
            prom,
            "50 tareas independientes, cada una recorre 1000 entradas",
            "embarrassingly parallel: 1 hilo por pestana (thread pool)"
        });
    }

    // ═══════════════════════════════════════════════════════════════
    // F7: Deteccion de duplicados O(n²)
    // ---------------------------------------------------------------
    // Para cada sitio compara su URL contra todos los demas.
    // 1000 × 999 / 2 ≈ 500,000 comparaciones de strings por corrida.
    // El programa actual llama eliminarSitioSiExiste() en add(), que
    // es O(n) y se llama n veces al cargar → O(n²) en total.
    // Paralelizar: dividir filas del triangulo entre N hilos.
    // ═══════════════════════════════════════════════════════════════
    {
        sep();
        std::cout << "[F7] Deteccion de duplicados O(n^2)\n";
        std::cout << "     " << sitios.size() << " × " << sitios.size()
                  << " ≈ " << (sitios.size() * sitios.size() / 2)
                  << " comparaciones de strings por corrida\n";

        long long total = 0;
        volatile int dups = 0;

        for (int r = 0; r < REPS; ++r) {
            total += medir([&]{
                int d = 0;
                for (size_t i = 0; i < sitios.size(); ++i) {
                    for (size_t j = i + 1; j < sitios.size(); ++j) {
                        if (sitios[i]->getUrl() == sitios[j]->getUrl()) ++d;
                    }
                }
                dups = d;
            });
        }
        long long prom = total / REPS;
        std::cout << "     Duplicados encontrados : " << dups << "\n";
        std::cout << "     Promedio por corrida   : " << fmtNs(prom) << "\n";
        std::cout << "     Con 4 hilos esperado   : ~" << fmtNs(prom / 4) << "\n";

        resultados.push_back({
            "F7: deteccion de duplicados O(n^2)    ",
            prom,
            "n*(n-1)/2 comparaciones de strings, complejidad cuadratica",
            "triangulo de comparaciones dividido en N bandas horizontales"
        });
    }

    // ═══════════════════════════════════════════════════════════════
    // F8: Ordenamiento masivo del historial (100 veces por corrida)
    // ---------------------------------------------------------------
    // Ordena la lista de 1000 sitios por URL alfabeticamente.
    // std::sort es O(n log n) ≈ 10,000 comparaciones de string.
    // Se repite 100 veces por corrida para hacerlo medible.
    // Paralelizar: parallel merge sort (divide-and-conquer).
    // ═══════════════════════════════════════════════════════════════
    {
        sep();
        const int SORTS = 100;
        std::cout << "[F8] Ordenamiento por URL (" << SORTS << " veces por corrida)\n";
        std::cout << "     sort(1000) × " << SORTS << " = ~"
                  << SORTS * 10000 << " comparaciones de strings por corrida\n";

        long long total = 0;

        for (int r = 0; r < REPS; ++r) {
            total += medir([&]{
                for (int k = 0; k < SORTS; ++k) {
                    std::vector<SitioWeb*> copia = sitios;   // copia del vector
                    std::sort(copia.begin(), copia.end(),
                              [](SitioWeb* a, SitioWeb* b){
                                  return a->getUrl() < b->getUrl();
                              });
                }
            });
        }
        long long prom = total / REPS;
        std::cout << "     Promedio por corrida ("
                  << SORTS << " sorts) : " << fmtNs(prom) << "\n";
        std::cout << "     Promedio por sort individual : "
                  << fmtNs(prom / SORTS) << "\n";
        std::cout << "     Con 4 hilos esperado         : ~"
                  << fmtNs(prom / 4) << "\n";

        resultados.push_back({
            "F8: ordenamiento masivo (100× sort 1000) ",
            prom,
            "100 sorts O(n log n) con comparacion de strings",
            "parallel merge sort: dividir en chunks, merge paralelo"
        });
    }

    // ═══════════════════════════════════════════════════════════════
    // F9: Generacion de reporte toString() completo (100 veces)
    // ---------------------------------------------------------------
    // Construye un string con toda la informacion del historial.
    // Internamente Historial::toString() itera los 1000 SitioWeb*
    // y cada SitioWeb::toString() crea un ostringstream.
    // Se llama 100 veces por corrida para hacerlo medible.
    // Paralelizar: cada hilo genera su segmento, concatenacion final.
    // ═══════════════════════════════════════════════════════════════
    {
        sep();
        const int REPS_TOSTR = 100;
        std::cout << "[F9] Generacion de reporte toString() ("
                  << REPS_TOSTR << " veces por corrida)\n";
        std::cout << "     " << REPS_TOSTR << " × toString(1000 entradas)"
                  << " = " << REPS_TOSTR * 1000 << " llamadas toString()\n";

        long long total = 0;
        std::string last_report;

        for (int r = 0; r < REPS; ++r) {
            total += medir([&]{
                for (int k = 0; k < REPS_TOSTR; ++k) {
                    last_report = histBase->toString();
                }
            });
        }
        long long prom = total / REPS;
        std::cout << "     Tamanio del reporte generado : "
                  << last_report.size() << " bytes\n";
        std::cout << "     Promedio por corrida ("
                  << REPS_TOSTR << " reportes) : " << fmtNs(prom) << "\n";
        std::cout << "     Promedio por reporte individual : "
                  << fmtNs(prom / REPS_TOSTR) << "\n";
        std::cout << "     Con 4 hilos esperado            : ~"
                  << fmtNs(prom / 4) << "\n";

        resultados.push_back({
            "F9: toString masivo (100× reporte 1000) ",
            prom,
            "100 construcciones de string con 1000 entradas cada una",
            "cada hilo genera su segmento del reporte, merge al final"
        });
    }

    // ═══════════════════════════════════════════════════════════════
    // F10: Serializacion + deserializacion binaria al disco
    // ---------------------------------------------------------------
    // guardarArchivoHistorial() escribe los 1000 SitioWeb al disco
    // en formato binario (url/titulo/dominio/timestamp por entrada).
    // cargarArchivoHistorial() los vuelve a leer y reconstruye.
    // Paralelizar: escribir/leer chunks en hilos separados (I/O async).
    // ═══════════════════════════════════════════════════════════════
    {
        sep();
        const char* TEMP = "_bench_sesion.bin";
        std::cout << "[F10] Serializacion + deserializacion binaria\n";
        std::cout << "      Escribe y lee 1000 SitioWeb al disco\n";

        long long total = 0;

        for (int r = 0; r < REPS; ++r) {
            total += medir([&]{
                // Guardar
                std::ofstream out(TEMP, std::ios::binary);
                histBase->guardarArchivoHistorial(out);
                out.close();

                // Cargar
                std::ifstream in(TEMP, std::ios::binary);
                Historial* h2 = Historial::cargarArchivoHistorial(in);
                in.close();
                if (h2) { h2->limpiarHistorial(); delete h2; }
            });
        }
        long long prom = total / REPS;

        // Obtener tamano del archivo generado
        std::ifstream tmp(TEMP, std::ios::ate | std::ios::binary);
        long long tam = tmp.tellg();

        std::cout << "      Archivo binario generado : " << tam << " bytes\n";
        std::cout << "      Promedio por corrida      : " << fmtNs(prom) << "\n";
        std::cout << "      Con 4 hilos esperado      : ~" << fmtNs(prom / 4) << "\n";

        std::remove(TEMP);

        resultados.push_back({
            "F10: serializacion binaria (1000 sitios)",
            prom,
            "I/O de disco: write + read de 1000 SitioWeb en binario",
            "I/O asincrono: chunks escritos/leidos en hilos separados"
        });
    }

    // ═══════════════════════════════════════════════════════════════
    // RESUMEN FINAL
    // ═══════════════════════════════════════════════════════════════
    sep('=');
    std::cout << "\n  RESUMEN — FUNCIONES CON MAYOR GANANCIA AL PARALELIZAR\n";
    std::cout << "  (ordenadas de mayor a menor tiempo → mayor beneficio)\n";
    sep('=');

    std::sort(resultados.begin(), resultados.end(),
              [](const Resultado& a, const Resultado& b){
                  return a.ns_promedio > b.ns_promedio;
              });

    // Tabla de tiempos
    std::cout << "\n  " << std::left << std::setw(43) << "Funcion"
              << std::right << std::setw(16) << "Tiempo promedio"
              << std::setw(16) << "Estimado x4 hilos" << "\n";
    sep();
    for (auto& r : resultados) {
        std::cout << "  " << std::left  << std::setw(43) << r.nombre
                  << std::right << std::setw(16) << fmtNs(r.ns_promedio)
                  << std::setw(16) << fmtNs(r.ns_promedio / 4) << "\n";
    }
    sep();

    // Estrategias
    std::cout << "\n  ESTRATEGIAS DE PARALELIZACION\n";
    sep();
    int rank = 1;
    for (auto& r : resultados) {
        std::cout << "  #" << rank++ << " " << r.nombre << "\n";
        std::cout << "     Por que es lenta : " << r.por_que_es_lenta << "\n";
        std::cout << "     Estrategia       : " << r.estrategia_paralela << "\n\n";
    }
    sep('=');
    std::cout << "  ns=nanoseg | us=microseg | ms=miliseg\n";
    sep('=');

    // Limpieza
    for (auto* h : pestanas) { h->limpiarHistorial(); delete h; }
    histBase->limpiarHistorial();
    delete histBase;
    for (auto* s : sitios) delete s;

    return 0;
}
