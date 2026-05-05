/*
 * benchmark.cpp
 * ---------------------------------------------------------------
 * Mide el tiempo de las 5 funciones con mayor potencial de
 * paralelizacion en el Navegador Web Simulator sobre 1000 sitios.
 *
 * Compilar (desde Proyecto1Datos/):
 *   g++ -std=c++14 -O2 -o benchmark.exe benchmark.cpp ^
 *       Historial.cpp SitioWeb.cpp ConfigHistorial.cpp Excepciones.cpp -I.
 *
 * Requiere: sitiosWeb_1000.csv en el mismo directorio que benchmark.exe
 * ---------------------------------------------------------------
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <iomanip>

#include "SitioWeb.h"
#include "Historial.h"
#include "ConfigHistorial.h"

// ─────────────────────────────────────────────────────────────────
// Temporizador de alta resolucion
// ─────────────────────────────────────────────────────────────────
using Clock = std::chrono::high_resolution_clock;
using Ns    = std::chrono::nanoseconds;

template<typename F>
long long medir(F&& fn) {
    auto t0 = Clock::now();
    fn();
    auto t1 = Clock::now();
    return std::chrono::duration_cast<Ns>(t1 - t0).count();
}

// ─────────────────────────────────────────────────────────────────
// Helpers de presentacion
// ─────────────────────────────────────────────────────────────────
static void sep(char c = '-', int n = 64) {
    std::cout << std::string(n, c) << '\n';
}

static std::string fmtNs(long long ns) {
    std::ostringstream oss;
    if      (ns >= 1000000LL) oss << (ns / 1000000LL) << " ms  (" << (ns / 1000LL) << " us)";
    else if (ns >= 1000LL)    oss << (ns / 1000LL)    << " us";
    else                      oss << ns                << " ns";
    return oss.str();
}

// ─────────────────────────────────────────────────────────────────
// Carga de CSV — espejo exacto de Navegador::cargarArchivoSitiosWebCSV
// ─────────────────────────────────────────────────────────────────
std::vector<SitioWeb*> cargarCSV(const std::string& ruta) {
    std::vector<SitioWeb*> sitios;
    std::ifstream arch(ruta);
    if (!arch.is_open()) return sitios;

    std::string linea;
    while (std::getline(arch, linea)) {
        if (linea.empty()) continue;
        if (!linea.empty() && linea.back() == '\r') linea.pop_back();  // CRLF

        std::istringstream ss(linea);
        std::string url, titulo, dominio;
        if (std::getline(ss, url,    ',') &&
            std::getline(ss, titulo, ',') &&
            std::getline(ss, dominio))
        {
            sitios.push_back(new SitioWeb(url, titulo, dominio));
        }
    }
    return sitios;
}

// ─────────────────────────────────────────────────────────────────
// F1: buscarPaginaWeb — O(n) linear scan en vector<SitioWeb*>
//     (logica identica a Navegador::buscarPaginaWeb)
// ─────────────────────────────────────────────────────────────────
SitioWeb* buscarPorUrl(const std::vector<SitioWeb*>& sitios,
                       const std::string& url) {
    auto it = std::find_if(sitios.begin(), sitios.end(),
                           [&](SitioWeb* s){ return s->getUrl() == url; });
    return (it != sitios.end()) ? *it : nullptr;
}

// ─────────────────────────────────────────────────────────────────
// Estructura de resultado
// ─────────────────────────────────────────────────────────────────
struct Resultado {
    std::string nombre;
    long long   ns_promedio;
    std::string estrategia_paralela;
};

// ═══════════════════════════════════════════════════════════════════
int main() {
    const std::string CSV  = "sitiosWeb_1000.csv";
    const int         REPS = 10;   // repeticiones para estabilizar medicion

    sep('=');
    std::cout << "  BENCHMARK DE PARALELIZACION — Navegador Web Simulator\n";
    std::cout << "  Dataset: 1000 sitios web  |  Repeticiones/prueba: " << REPS << "\n";
    sep('=');

    // ── Carga inicial del CSV ────────────────────────────────────
    std::cout << "\n[*] Cargando " << CSV << " ...\n";
    std::vector<SitioWeb*> sitios;
    long long t_carga_fria = medir([&]{ sitios = cargarCSV(CSV); });

    if (sitios.empty()) {
        std::cerr << "[ERROR] No se cargaron sitios. "
                     "Verifique que " << CSV << " exista.\n";
        return 1;
    }
    std::cout << "[*] " << sitios.size() << " sitios cargados  "
              << "(carga fria: " << fmtNs(t_carga_fria) << ")\n";

    // ── Configurar singleton ─────────────────────────────────────
    // tiempoMaximo muy alto: nada expira durante F3 → scan completo
    ConfigHistorial::getInstancia()->setTiempoMaximo(999999);

    // ── Poblar Historial ─────────────────────────────────────────
    std::cout << "[*] Poblando historial con " << sitios.size() << " entradas...\n";
    Historial* hist = new Historial();
    for (auto* s : sitios) hist->add(s);
    std::cout << "[*] Historial listo: " << hist->size() << " entradas\n\n";

    std::vector<Resultado> resultados;

    // ═══════════════════════════════════════════════════════════════
    // F1: buscarPaginaWeb
    // ---------------------------------------------------------------
    // Patron: std::find_if secuencial sobre vector<SitioWeb*>
    // Paralelizar: dividir vector en N chunks, un hilo por chunk;
    //              atomic<bool> como flag de halt al encontrar match.
    // ═══════════════════════════════════════════════════════════════
    {
        sep();
        std::cout << "[F1] buscarPaginaWeb — busqueda exacta por URL\n";
        std::cout << "     " << sitios.size() << " busquedas x " << REPS
                  << " reps sobre vector de " << sitios.size() << " sitios\n";

        long long total = 0;
        volatile int dummy = 0;   // evita que el compilador elimine el loop

        for (int r = 0; r < REPS; ++r) {
            total += medir([&]{
                for (auto* s : sitios) {
                    if (buscarPorUrl(sitios, s->getUrl())) dummy++;
                }
            });
        }
        long long prom = total / REPS;

        std::cout << "     Promedio por corrida (1000 busquedas) : " << fmtNs(prom) << "\n";
        std::cout << "     Promedio por busqueda individual       : "
                  << fmtNs(prom / (long long)sitios.size()) << "\n";

        resultados.push_back({
            "F1: buscarPaginaWeb            (1000 busquedas)", prom,
            "parallel_find_if  con atomic<bool> halt-flag"
        });
    }

    // ═══════════════════════════════════════════════════════════════
    // F2: busquedaPalabraClave
    // ---------------------------------------------------------------
    // Patron: for-each sobre list<SitioWeb*> con tolower + find
    // Paralelizar: map-reduce — cada hilo procesa su particion de la
    //              lista, acumula coincidencias, merge final de strings.
    // ═══════════════════════════════════════════════════════════════
    {
        sep();
        std::cout << "[F2] busquedaPalabraClave — keyword search en historial\n";
        std::cout << "     Busca sobre " << hist->size() << " entradas del historial\n";

        const std::vector<std::string> kws = {
            "portal", "tecnologia", "noticias", "educacion", "entretenimiento",
            "compras", "social",    "gobierno", "salud",     "ciencia"
        };
        long long   total = 0;
        std::string last_result;

        for (int r = 0; r < REPS; ++r) {
            total += medir([&]{
                last_result = hist->busquedaPalabraClave(kws[r % (int)kws.size()]);
            });
        }
        long long prom = total / REPS;

        std::cout << "     Ultima keyword : \"" << kws[(REPS - 1) % kws.size()]
                  << "\"  —  " << (last_result.empty() ? "sin coincidencias" : "con coincidencias")
                  << "\n";
        std::cout << "     Promedio       : " << fmtNs(prom) << "\n";

        resultados.push_back({
            "F2: busquedaPalabraClave       (hist. 1000 ent.)", prom,
            "map-reduce: N hilos en particiones, merge de resultados"
        });
    }

    // ═══════════════════════════════════════════════════════════════
    // F3: limpiarSitiosViejos
    // ---------------------------------------------------------------
    // Patron: for-each comparando system_clock::now() - tiempoDeIngreso
    // Paralelizar: fase check en paralelo (marca entradas expiradas),
    //              fase delete secuencial para preservar integridad de list.
    // tiempoMaximo=999999 s → scan completo, cero borrados.
    // ═══════════════════════════════════════════════════════════════
    {
        sep();
        std::cout << "[F3] limpiarSitiosViejos — escaneo de timestamps\n";
        std::cout << "     Escanea " << hist->size()
                  << " entradas  (tiempoMax=999999 s → ningun sitio expira)\n";

        long long total = 0;
        for (int r = 0; r < REPS; ++r) {
            total += medir([&]{ hist->limpiarSitiosViejos(); });
        }
        long long prom = total / REPS;

        std::cout << "     Promedio       : " << fmtNs(prom) << "\n";

        resultados.push_back({
            "F3: limpiarSitiosViejos        (hist. 1000 ent.)", prom,
            "pipeline 2 fases: check paralelo, borrado secuencial"
        });
    }

    // ═══════════════════════════════════════════════════════════════
    // F4: cargarArchivoSitiosWebCSV
    // ---------------------------------------------------------------
    // Patron: IO secuencial + parse linea a linea + new SitioWeb()
    // Paralelizar: producer-consumer — 1 hilo lee lineas al buffer,
    //              N hilos parsean y construyen SitioWeb* en paralelo.
    // ═══════════════════════════════════════════════════════════════
    {
        sep();
        std::cout << "[F4] cargarArchivoSitiosWebCSV — carga y parseo del CSV\n";
        std::cout << "     Archivo: " << CSV << "  (" << sitios.size() << " lineas)\n";

        long long total = 0;
        for (int r = 0; r < REPS; ++r) {
            std::vector<SitioWeb*> tmp;
            total += medir([&]{ tmp = cargarCSV(CSV); });
            for (auto* s : tmp) delete s;
        }
        long long prom = total / REPS;

        std::cout << "     Carga fria (primera vez)         : " << fmtNs(t_carga_fria) << "\n";
        std::cout << "     Promedio (cache OS ya caliente)  : " << fmtNs(prom) << "\n";

        resultados.push_back({
            "F4: cargarArchivoSitiosWebCSV  (1000 lineas)", prom,
            "producer-consumer: IO hilo unico, parse/new en N hilos"
        });
    }

    // ═══════════════════════════════════════════════════════════════
    // F5: moverseAPrimeraCoincidencia (con setFiltro)
    // ---------------------------------------------------------------
    // Patron: iteracion lineal por list<SitioWeb*> hasta primer match
    // Paralelizar: parallel prefix scan — N hilos buscan en segmentos,
    //              se queda con el indice minimo que tiene match.
    // ═══════════════════════════════════════════════════════════════
    {
        sep();
        std::cout << "[F5] setFiltro + moverseAPrimeraCoincidencia — scan de filtro\n";
        std::cout << "     Escanea hasta " << hist->size()
                  << " entradas para hallar la primera coincidencia\n";

        const std::vector<std::string> filtros = {
            "portal",   "tecnologia", "noticias", "educacion", "entretenimiento",
            "compras",  "social",     "gobierno", "salud",     "deportes"
        };
        long long total = 0;

        for (int r = 0; r < REPS; ++r) {
            total += medir([&]{
                hist->setFiltro(filtros[r % (int)filtros.size()]);
                hist->moverseAPrimeraCoincidencia();
            });
        }
        long long prom = total / REPS;

        std::cout << "     Promedio       : " << fmtNs(prom) << "\n";

        resultados.push_back({
            "F5: moverseAPrimeraCoincidencia (hist. 1000 ent.)", prom,
            "parallel prefix scan con atomic<int> para indice minimo"
        });
    }

    // ═══════════════════════════════════════════════════════════════
    // RESUMEN FINAL
    // ═══════════════════════════════════════════════════════════════
    sep('=');
    std::cout << "\n  TOP 5 FUNCIONES PARA PARALELIZAR\n";
    std::cout << "  (mayor tiempo promedio = mayor ganancia potencial al paralelizar)\n";
    sep('=');

    std::sort(resultados.begin(), resultados.end(),
              [](const Resultado& a, const Resultado& b){
                  return a.ns_promedio > b.ns_promedio;
              });

    std::cout << "\n  " << std::left << std::setw(52) << "Funcion"
              << std::right << std::setw(14) << "Tiempo prom."
              << "\n";
    sep();
    int rank = 1;
    for (auto& r : resultados) {
        std::cout << "  #" << rank++ << "  "
                  << std::left << std::setw(50) << r.nombre
                  << std::right << std::setw(14) << fmtNs(r.ns_promedio)
                  << "\n";
    }
    sep();

    std::cout << "\n  ESTRATEGIAS DE PARALELIZACION RECOMENDADAS\n";
    sep();
    rank = 1;
    for (auto& r : resultados) {
        std::cout << "  #" << rank++ << " " << r.nombre << "\n"
                  << "     -> " << r.estrategia_paralela << "\n\n";
    }

    sep('=');
    std::cout << "  ns = nanoseg | us = microseg | ms = miliseg\n";
    sep('=');

    // Limpieza de memoria
    hist->limpiarHistorial();
    delete hist;
    for (auto* s : sitios) delete s;

    return 0;
}
