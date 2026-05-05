#include <sstream>
#include <string>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <vector>
#include "Controladora.h"

// ============================================================
//   ESTRUCTURA PARA GUARDAR RESULTADOS DEL BENCHMARK
// ============================================================
struct Resultado {
    std::string funcion;
    int elementos;
    double tiempoMs;
};

// Mide el tiempo de cualquier funcion en milisegundos
template<typename F>
double medir(F func) {
    auto t0 = std::chrono::high_resolution_clock::now();
    func();
    auto t1 = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

// Crea N pestanias y agrega M sitios a cada una usando la API publica del navegador
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

// Imprime el reporte en consola y lo guarda en archivo de texto
void imprimirReporte(const std::vector<Resultado>& resultados) {
    const int W1 = 42, W2 = 12, W3 = 14;
    std::string sep(W1 + W2 + W3 + 4, '=');
    std::string lin(W1 + W2 + W3 + 4, '-');

    std::cout << "\n" << sep << "\n";
    std::cout << "      REPORTE DE RENDIMIENTO - VERSION SECUENCIAL\n";
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
              << std::setw(W1) << "TIEMPO TOTAL SECUENCIAL"
              << std::setw(W2) << ""
              << std::fixed << std::setprecision(4) << total << " ms\n";
    std::cout << sep << "\n";

    // Guardar en archivo para comparar despues con la version paralela
    std::ofstream out("reporte_secuencial.txt");
    out << "REPORTE DE RENDIMIENTO - VERSION SECUENCIAL\n";
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
    std::cout << "\nReporte guardado en: reporte_secuencial.txt\n";
}

int main() {
    const int NUM_PESTANIAS      = 10;
    const int SITIOS_POR_PESTANIA = 100;

    std::cout << "Cargando datos..." << std::endl;
    Controladora* C0 = new Controladora();
    Navegador* nav = C0->getNavegador();

    const int TOTAL_BD        = (int)nav->getListaSitiosW()->size();
    const int TOTAL_HISTORIAL = NUM_PESTANIAS * SITIOS_POR_PESTANIA;

    prepararDatos(nav, NUM_PESTANIAS, SITIOS_POR_PESTANIA);
    std::cout << "Pestanias: " << NUM_PESTANIAS
              << " | Entradas en historial: " << TOTAL_HISTORIAL
              << " | Sitios en BD: " << TOTAL_BD << "\n\n";

    std::vector<Resultado> reporte;

    // ----------------------------------------------------------------
    // FUNCION 1: busquedaMasiva - recorre vector<SitioWeb*> completo
    // ----------------------------------------------------------------
    std::cout << "[1/5] busquedaMasiva(\"noticias\") - " << TOTAL_BD << " sitios...";
    double t1 = medir([&]() { nav->busquedaMasiva("noticias"); });
    std::cout << " " << std::fixed << std::setprecision(4) << t1 << " ms\n";
    reporte.push_back({"busquedaMasiva(\"noticias\")", TOTAL_BD, t1});

    // ----------------------------------------------------------------
    // FUNCION 2: busquedaPalabraClave - busca en historial de pestana actual
    // ----------------------------------------------------------------
    std::cout << "[2/5] busquedaPalabraClave(\"tech\") - " << SITIOS_POR_PESTANIA << " entradas...";
    double t2 = medir([&]() { nav->busquedaPalabraClave("tech"); });
    std::cout << " " << std::fixed << std::setprecision(4) << t2 << " ms\n";
    reporte.push_back({"busquedaPalabraClave(\"tech\")", SITIOS_POR_PESTANIA, t2});

    // ----------------------------------------------------------------
    // FUNCION 3: limpiarSitiosViejos - recorre TODAS las pestanias
    //            tiempoMaximo=3600: recorre todo pero no borra nada reciente
    // ----------------------------------------------------------------
    nav->setTiempoMaximo(3600);
    std::cout << "[3/5] limpiarSitiosViejos() - " << TOTAL_HISTORIAL << " entradas...";
    double t3 = medir([&]() { nav->limpiarSitiosViejos(); });
    nav->reiniciarConfiguraciones();
    std::cout << " " << std::fixed << std::setprecision(4) << t3 << " ms\n";
    reporte.push_back({"limpiarSitiosViejos()", TOTAL_HISTORIAL, t3});

    // ----------------------------------------------------------------
    // FUNCION 4: cargarArchivoSitiosWebCSV - carga y construye objetos
    // ----------------------------------------------------------------
    std::cout << "[4/5] cargarArchivoSitiosWebCSV() - " << TOTAL_BD << " registros...";
    double t4 = medir([&]() {
        std::vector<SitioWeb*> temp;
        std::ifstream archivo("sitiosWeb.csv");
        std::string linea;
        while (std::getline(archivo, linea)) {
            std::stringstream ss(linea);
            std::string url, titulo, dominio;
            if (std::getline(ss, url, ',') &&
                std::getline(ss, titulo, ',') &&
                std::getline(ss, dominio))
                temp.push_back(new SitioWeb(url, titulo, dominio));
        }
        for (auto s : temp) delete s;
    });
    std::cout << " " << std::fixed << std::setprecision(4) << t4 << " ms\n";
    reporte.push_back({"cargarArchivoSitiosWebCSV()", TOTAL_BD, t4});

    // ----------------------------------------------------------------
    // FUNCION 5: ajustarTamanoHistorial - recorta historial en todas las pestanias
    //            maxEntradas=80: recorta de 100 a 80 entradas por pestana
    // ----------------------------------------------------------------
    nav->setMaxEntradas(80);
    std::cout << "[5/5] ajustarTamanoHistorial() - " << TOTAL_HISTORIAL << " entradas...";
    double t5 = medir([&]() { nav->ajustarTamanoHistorial(); });
    nav->reiniciarConfiguraciones();
    std::cout << " " << std::fixed << std::setprecision(4) << t5 << " ms\n";
    reporte.push_back({"ajustarTamanoHistorial()", TOTAL_HISTORIAL, t5});

    // ----------------------------------------------------------------
    // REPORTE FINAL
    // ----------------------------------------------------------------
    imprimirReporte(reporte);

    std::cout << "\nPresione ENTER para iniciar el navegador...";
    std::cin.get();

    C0->control0();
    delete C0;
    return 0;
}