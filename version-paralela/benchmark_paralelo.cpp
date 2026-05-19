#include <sstream>
#include <string>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <vector>
#include <omp.h>
#include "Controladora.h"

// =====================================================================
// BENCHMARK - VERSION PARALELA (OpenMP, 4 hilos)
// Mide las mismas 5 funciones que benchmark_secuencial.cpp
// para una comparacion directa de speedup.
//
// Ley de Amdahl con p=0.85 y n=4 hilos:
//   S(4) = 1 / ((1-0.85) + 0.85/4) = 1 / (0.15 + 0.2125) = 2.76x teorico
// =====================================================================

struct Resultado {
    std::string funcion;
    int elementos;
    double tiempoMs;
};

template<typename F>
double medir(F func) {
    auto t0 = std::chrono::high_resolution_clock::now();
    func();
    auto t1 = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

void prepararDatos(Navegador* nav, int numPestanias, int sitiosPorPestania) {
    auto& sitios = *nav->getListaSitiosW();
    int total = (int)sitios.size();
    for (int i = 0; i < numPestanias; i++) {
        nav->agregarPestania();
        for (int j = 0; j < sitiosPorPestania; j++) {
            nav->agregarPaginaWeb(sitios[(i * sitiosPorPestania + j) % total]);
        }
    }
}

void imprimirReporte(const std::vector<Resultado>& resultados, int hilos) {
    const int W1 = 44, W2 = 12, W3 = 14;
    std::string sep(W1 + W2 + W3 + 4, '=');
    std::string lin(W1 + W2 + W3 + 4, '-');

    std::cout << "\n" << sep << "\n";
    std::cout << "   REPORTE DE RENDIMIENTO - VERSION PARALELA (" << hilos << " hilos OpenMP)\n";
    std::cout << sep << "\n";
    std::cout << std::left
              << std::setw(3)  << "#"
              << std::setw(W1) << "Funcion"
              << std::setw(W2) << "Elementos"
              << "Tiempo (ms)\n";
    std::cout << lin << "\n";

    double total = 0;
    for (int i = 0; i < (int)resultados.size(); i++) {
        const auto& r = resultados[i];
        std::cout << std::left
                  << std::setw(3)  << (i + 1)
                  << std::setw(W1) << r.funcion
                  << std::setw(W2) << r.elementos
                  << std::fixed << std::setprecision(4) << r.tiempoMs << "\n";
        total += r.tiempoMs;
    }

    std::cout << lin << "\n";
    std::cout << std::left
              << std::setw(3)  << ""
              << std::setw(W1) << "TIEMPO TOTAL PARALELO"
              << std::setw(W2) << ""
              << std::fixed << std::setprecision(4) << total << " ms\n";
    std::cout << sep << "\n";

    std::ofstream out("reporte_paralelo.txt");
    out << "REPORTE DE RENDIMIENTO - VERSION PARALELA (" << hilos << " hilos OpenMP)\n";
    out << lin << "\n";
    for (int i = 0; i < (int)resultados.size(); i++) {
        const auto& r = resultados[i];
        out << (i + 1) << ". " << r.funcion
            << " | " << r.elementos << " elementos"
            << " | " << std::fixed << std::setprecision(4) << r.tiempoMs << " ms\n";
    }
    out << lin << "\n";
    out << "TIEMPO TOTAL: " << std::fixed << std::setprecision(4) << total << " ms\n";
    out.close();
    std::cout << "\nReporte guardado en: reporte_paralelo.txt\n";
}

int main() {
    const int NUM_HILOS             = 4;
    const int NUM_PESTANIAS         = 10;
    const int SITIOS_POR_PESTANIA   = 100;

    omp_set_num_threads(NUM_HILOS);

    std::cout << "Compilado con OpenMP. Hilos activos: " << omp_get_max_threads() << "\n";
    std::cout << "Cargando datos...\n";

    Controladora* C0 = new Controladora();
    Navegador* nav = C0->getNavegador();

    const int TOTAL_BD       = (int)nav->getListaSitiosW()->size();
    const int TOTAL_HISTORIAL = NUM_PESTANIAS * SITIOS_POR_PESTANIA;

    prepararDatos(nav, NUM_PESTANIAS, SITIOS_POR_PESTANIA);
    std::cout << "Pestanias: " << NUM_PESTANIAS
              << " | Entradas en historial: " << TOTAL_HISTORIAL
              << " | Sitios en BD: " << TOTAL_BD << "\n\n";

    std::vector<Resultado> reporte;

    // -------------------------------------------------------------------
    // FUNCION 1: busquedaMasiva - busca en los 1000 sitios de la BD
    // -------------------------------------------------------------------
    std::cout << "[1/5] busquedaMasiva(\"noticias\") - " << TOTAL_BD << " sitios...";
    double t1 = medir([&]() { nav->busquedaMasiva("noticias"); });
    std::cout << "  " << std::fixed << std::setprecision(4) << t1 << " ms\n";
    reporte.push_back({"busquedaMasiva(\"noticias\")", TOTAL_BD, t1});

    // -------------------------------------------------------------------
    // FUNCION 2: busquedaPalabraClave - busca en el historial de la pestana activa
    // -------------------------------------------------------------------
    std::cout << "[2/5] busquedaPalabraClave(\"tech\") - " << SITIOS_POR_PESTANIA << " entradas...";
    double t2 = medir([&]() { nav->busquedaPalabraClave("tech"); });
    std::cout << "  " << std::fixed << std::setprecision(4) << t2 << " ms\n";
    reporte.push_back({"busquedaPalabraClave(\"tech\")", SITIOS_POR_PESTANIA, t2});

    // -------------------------------------------------------------------
    // FUNCION 3: limpiarSitiosViejos - verifica timestamps en paralelo
    // tiempoMaximo=3600: recorre todo pero no borra nada reciente
    // -------------------------------------------------------------------
    nav->setTiempoMaximo(3600);
    std::cout << "[3/5] limpiarSitiosViejos() - " << TOTAL_HISTORIAL << " entradas...";
    double t3 = medir([&]() { nav->limpiarSitiosViejos(); });
    nav->reiniciarConfiguraciones();
    std::cout << "  " << std::fixed << std::setprecision(4) << t3 << " ms\n";
    reporte.push_back({"limpiarSitiosViejos()", TOTAL_HISTORIAL, t3});

    // -------------------------------------------------------------------
    // FUNCION 4: cargarArchivoSitiosWebCSV - IO serial + parse paralelo
    // -------------------------------------------------------------------
    std::cout << "[4/5] cargarArchivoSitiosWebCSV() - " << TOTAL_BD << " registros...";
    double t4 = medir([&]() {
        std::vector<std::string> lineas;
        std::ifstream archivo("sitiosWeb.csv");
        std::string linea;
        while (std::getline(archivo, linea)) {
            if (!linea.empty()) lineas.push_back(linea);
        }
        std::vector<SitioWeb*> temp(lineas.size(), nullptr);
#pragma omp parallel for schedule(static)
        for (int i = 0; i < (int)lineas.size(); i++) {
            std::istringstream ss(lineas[i]);
            std::string url, titulo, dominio;
            if (std::getline(ss, url, ',') && std::getline(ss, titulo, ',') &&
                std::getline(ss, dominio)) {
                temp[i] = new SitioWeb(url, titulo, dominio);
            }
        }
        for (auto s : temp) delete s;
    });
    std::cout << "  " << std::fixed << std::setprecision(4) << t4 << " ms\n";
    reporte.push_back({"cargarArchivoSitiosWebCSV()", TOTAL_BD, t4});

    // -------------------------------------------------------------------
    // FUNCION 5: toString - genera string del historial en paralelo
    // -------------------------------------------------------------------
    std::cout << "[5/5] toString() - " << SITIOS_POR_PESTANIA << " entradas...";
    double t5 = medir([&]() { nav->mostrarPestaniaActual(); });
    std::cout << "  " << std::fixed << std::setprecision(4) << t5 << " ms\n";
    reporte.push_back({"toString() via mostrarPestaniaActual()", SITIOS_POR_PESTANIA, t5});

    imprimirReporte(reporte, NUM_HILOS);

    delete C0;
    return 0;
}
