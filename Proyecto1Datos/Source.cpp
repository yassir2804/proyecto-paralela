#include <sstream>
#include <string>
#include <iostream>
#include <chrono>
#include "Controladora.h"

void ejecutarBenchmarkSecuencial(Navegador* nav) {
    const std::string palabraClave = "noticias";
    int totalSitios = (int)nav->getListaSitiosW()->size();

    std::cout << "\n========================================" << std::endl;
    std::cout << "   BENCHMARK - BUSQUEDA SECUENCIAL" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Palabra clave : \"" << palabraClave << "\"" << std::endl;
    std::cout << "Total de sitios: " << totalSitios << std::endl;
    std::cout << "Ejecutando busqueda..." << std::endl;

    auto inicio = std::chrono::high_resolution_clock::now();
    std::vector<SitioWeb*> resultados = nav->busquedaMasiva(palabraClave);
    auto fin = std::chrono::high_resolution_clock::now();

    double tiempoMs = std::chrono::duration<double, std::milli>(fin - inicio).count();

    std::cout << "Resultados encontrados: " << resultados.size() << std::endl;
    std::cout << "Tiempo secuencial     : " << tiempoMs << " ms" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nPresione ENTER para iniciar el navegador...";
    std::cin.get();
}

int main() {
    Controladora* C0 = new Controladora();
    ejecutarBenchmarkSecuencial(C0->getNavegador());
    C0->control0();
    delete C0;
    return 0;
}