#include "Historial.h"
#include "ConfigHistorial.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <omp.h>
#include <sstream>
#include <unordered_set>
#include <vector>


Historial::Historial() : posicionActual(historial.end()), filtro("") 
{
}

Historial::~Historial() 
{
    limpiarHistorial();
  
}

int Historial::size() const 
{
    return (int)historial.size();
}

void Historial::add(SitioWeb* sitioWeb)
{

    //El proceso de aniadir es el siguiente
    //Si la lista est� vac�a, se a�ade el sitio web de manera directa

    if (historial.empty()) {
        historial.push_back(new SitioWeb(*sitioWeb));
        posicionActual = std::prev(historial.end());
        filtro = "";
        return;
    }

    //si no esta vac�a mediante este metodo se le busca y elimina si ya existe

    eliminarSitioSiExiste(sitioWeb);

    //Se obtiene el maximo de entradas permitidas en el historial llamando a una instancia de ConfigHistorial

    int maxEntradas = ConfigHistorial::getInstancia()->getMaxEntradas();

    if (historial.size() >= maxEntradas) {
        delete historial.front();// Se elimina el primer sitio web ingresado, primero liberamos la memoria, debido a que una lista de punteros
        historial.pop_front();//Seguidamente le hacemos pop de la lista de historial
    }

    //Una vez se elimina el primer sitio web (o no en caso de que no haya maximo de entradas o no se halla llegado al maximo de entradas)
    //Se a�ade el sitio web al final de la lista de historial

    historial.push_back(new SitioWeb(*sitioWeb));
    posicionActual = std::prev(historial.end());
    filtro = "";
}

void Historial::eliminarSitioSiExiste(SitioWeb* site) 
{
    //Se recorre la lista de historial, si el sitio web ya existe, se elimina de la lista de historial

    for (auto iter = historial.begin(); iter != historial.end(); ++iter) {
        if ((*iter)->getUrl() == site->getUrl()) {
            delete* iter;        
            historial.erase(iter); 
            return;
        }
    }
}


void Historial::retroceder()
{
    //En el retroceder se verifica primero si la lista esta vacia
    //o en su caso si la posicion actual es igual a la primera posicion de la lista 
    //si es asi pues no se hace nada

    if (historial.empty() || posicionActual == historial.begin()) {
        return; 
    }

    //Si no tiene filtro se retrocede el iterador una vez


    if (filtro.empty()) {

        --posicionActual; 
        return;
    }

    //Si tiene filtro entonces vemos a la posicion anterior

    auto posicionAnterior = std::prev(posicionActual);

    while (posicionAnterior != historial.begin()) {

        // verificamos mientras la posicionanterior no coincida con el filtro
        // entonces se retrocede el iterador (todo esto mientras no se llegue al inicio de la lista)

        if ((*posicionAnterior)->getTitulo().find(filtro) != std::string::npos || //cabe aclarar que si el find es diferente de npos significa que si se encontro la coincidencia
            (*posicionAnterior)->getUrl().find(filtro) != std::string::npos) {//esto significa que es una posicion invalida o fuera de rango de la cadena
            posicionActual = posicionAnterior; //si coincide la posicion actual apuntara a la coincidencia
            return;
        }
        posicionAnterior = --posicionAnterior; 
    }

    //Si no se encontro ninguna coincidencia entonces la posicion actual sera el inicio de la lista en caso de que esta coincida con el filtro

    if ((*historial.begin())->getTitulo().find(filtro) != std::string::npos ||
        (*historial.begin())->getUrl().find(filtro) != std::string::npos) {
        posicionActual = historial.begin(); 
    }
}

void Historial::avanzar()
{

    //Mismo caso que en retroceder se verifica si esta vacia pero la diferencia es ahora si es el final
    if (historial.empty() || posicionActual == historial.end()) {
        return; 
    }

    if (filtro.empty()) {

        if (posicionActual != prev(historial.end()))
            ++posicionActual;
        return;
    }

    //En este caso se toma la posicion siguiente
    auto posicionSiguiente = std::next(posicionActual);


    while (posicionSiguiente != historial.end()) {

        //Se avanza mientras sea diferente de end

        if ((*posicionSiguiente)->getTitulo().find(filtro) != std::string::npos ||
            (*posicionSiguiente)->getUrl().find(filtro) != std::string::npos) {
            posicionActual = posicionSiguiente;// si coincide la posicion actual apuntara a la coincidencia
            return;
        }
        ++posicionSiguiente; 
    }

}

void Historial::limpiarHistorial() 
{
    for (SitioWeb* sitio : historial) {
        delete sitio;
    }
    historial.clear();
    posicionActual = historial.end();
}

std::string Historial::busquedaPalabraClave(const std::string& palabraClave) const
{
    auto startTime = std::chrono::high_resolution_clock::now();
    std::stringstream s;
    int contador = 1;
    std::string palabraFiltrada = palabraClave;

    // Obtenemos la palabra clave, y con el transform lo convertimos a minusculas 
    // para que no importe si la palabra clave esta en mayusculas o minusculas

    transform(palabraFiltrada.begin(), palabraFiltrada.end(), palabraFiltrada.begin(), ::tolower);

    std::vector<SitioWeb*> vec(historial.begin(), historial.end());
    std::vector<std::string> partes(vec.size());
    std::vector<bool> coincide(vec.size(), false);

#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < (int)vec.size(); i++) {
        SitioWeb* sitio = vec[i];
        if (sitio == nullptr) {
            continue;
        }

        std::string tituloSitio = sitio->getTitulo();
        std::transform(tituloSitio.begin(), tituloSitio.end(), tituloSitio.begin(), ::tolower);

        if (tituloSitio.find(palabraFiltrada) != std::string::npos) {
            partes[i] = sitio->toString();
            coincide[i] = true;
        }
    }

    for (size_t i = 0; i < partes.size(); i++) {
        if (coincide[i]) {
            s << "------------------------------------------------------" << std::endl;
            s << " COINCIDENCIA # " << contador << std::endl;
            s << partes[i] << std::endl;
            contador++;
            s << "------------------------------------------------------" << std::endl;
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    std::cout << "[Tiempo] Historial::busquedaPalabraClave: " << elapsedUs << " us" << std::endl;

    return s.str();
}

std::list<SitioWeb*> Historial::getHistorial() const 
{
    return historial;
}

SitioWeb* Historial::getSitioActual() const
{
    if (posicionActual != historial.end() && !historial.empty()) {
        return *posicionActual;
    }
    return nullptr;
}

std::string Historial::toString() const {
    auto startTime = std::chrono::high_resolution_clock::now();
    std::stringstream ss;
    ss << "Historial:\n";

    std::vector<SitioWeb*> vec(historial.begin(), historial.end());
    std::vector<std::string> partes(vec.size());

#pragma omp parallel for
    for (int i = 0; i < (int)vec.size(); i++) {
        if (vec[i] != nullptr) {
            partes[i] = vec[i]->toString() + "\n";
        }
    }

    for (const auto& parte : partes) {
        ss << parte;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    std::cout << "[Tiempo] Historial::toString: " << elapsedUs << " us" << std::endl;

    return ss.str();
}

void Historial::ajustarTamanoHistorial()
{
    // Si el historial esta vacio no hacemos nada
    if (historial.empty()) {
        return;
    }

    // Obtenemos el maximo de entradas que puede tener el historial
    // con la configuracion del navegador
    int maxEntradas = ConfigHistorial::getInstancia()->getMaxEntradas();
    bool posicionActualEliminada = false;

    // Si el maximo de entradas es menor o igual a 0, no hacemos nada
    if (maxEntradas <= 0) {
        return;
    }

    filtro = "";

    // Mientras el historial tenga mas entradas que el maximo permitido
    // vamos a eliminar las entradas mas antiguas esto para que el historial
    // no tenga mas entradas de las que se permiten y si si tiene que borre las mas antiguas

    while (historial.size() > maxEntradas) {
        if (posicionActual == historial.begin()) {
            posicionActualEliminada = true;
        }

        // Borramos la entrada mas antigua tanto como del historial como de la memoria
        delete historial.front();
        historial.pop_front();

        // Si la posicion actual es la primera, la movemos a la siguiente
        // ya que la primera entrada fue eliminada
        if (posicionActual == historial.begin()) {

            ++posicionActual;
        }
    }

    // Si el historial quedo vacio, la posicion actual la ponemos en el final
    if (historial.empty()) {
        posicionActual = historial.end(); // Ponemos la posicion actual al final
        return;
    }
    // Si la posicion actual fue eliminada, la movemos al final
    if (posicionActualEliminada || posicionActual == historial.end()) {
        posicionActual = --historial.end();  // Movemos la posicion actual al�final
    }
}

bool Historial::limpiarSitiosViejos()
{
    auto startTime = std::chrono::high_resolution_clock::now();
    if (historial.empty()) {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
        std::cout << "[Tiempo] Historial::limpiarSitiosViejos: " << elapsedUs << " us" << std::endl;
        return false; 
    }

    //Obtenemos el tiempo configurado para eliminar las entradas del historial
    //Si este esta en su calor predeterminado (-1) significa que esto no esta configurado entonces no se hace nada

    int tiempoMaximo = ConfigHistorial::getInstancia()->getTiempoMaximo();
    if (tiempoMaximo <= 0) {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
        std::cout << "[Tiempo] Historial::limpiarSitiosViejos: " << elapsedUs << " us" << std::endl;
        return false; 
    }

    //Este es un valor bool que se retornara para saber si se eliminaron entradas del historial
    bool entradasBorradas = false;

    bool posicionActualEliminada = false;

    //Guardamos la posicion actual debido a que si se elimina el sitio al que esta apuntando
    //Puede quedar en una posicion ivalida asi que guardamos la posicion actual para luego asignarla

    std::vector<SitioWeb*> vec(historial.begin(), historial.end());
    std::vector<bool> expirado(vec.size(), false);

    auto now = std::chrono::system_clock::now();

#pragma omp parallel for
    for (int i = 0; i < (int)vec.size(); i++) {
        SitioWeb* sitio = vec[i];
        if (sitio == nullptr) {
            continue;
        }

        double diff = static_cast<double>(std::chrono::duration_cast<std::chrono::seconds>(now - sitio->getTiempoDeIngreso()).count());
        expirado[i] = (diff > tiempoMaximo);
    }

    std::unordered_set<SitioWeb*> expirados;
    expirados.reserve(vec.size());
    for (size_t i = 0; i < vec.size(); i++) {
        if (expirado[i]) {
            expirados.insert(vec[i]);
        }
    }

    for (auto it = historial.begin(); it != historial.end(); ) {
        SitioWeb* sitio = *it;
        if (expirados.find(sitio) != expirados.end()) {
            if (it == posicionActual) {
                posicionActualEliminada = true;
                posicionActual = historial.end();
            }
            delete sitio;
            it = historial.erase(it);
            entradasBorradas = true;
            continue;
        }
        ++it;
    }


    if (historial.empty()) {
        posicionActual = historial.end(); //Si el historial esta vacio la posicion actual sera el final
    }
    else if (posicionActualEliminada || posicionActual == historial.end()) {
        posicionActual = --historial.end(); //Si la posicion actual es el final o fue eliminada entonces se le asigna el ultimo sitio web
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    std::cout << "[Tiempo] Historial::limpiarSitiosViejos: " << elapsedUs << " us" << std::endl;

    return entradasBorradas; 
}

std::string Historial::getFiltro()
{
    return filtro;
}

void Historial::setFiltro(std::string filtro)
{
	this->filtro = filtro;
}


void Historial::moverseAPrimeraCoincidencia()
{
    // Si el historial esta vacio no hacemos nada
    if (historial.empty()) {
        return;
    }

    // Si el filtro esta vacio no hacemos nada
    if (filtro.empty()) {
        return;
    }

    // Creamos una variable para saber si se encontro una coincidencia

    bool coincidenciaEncontrada = false;

    // Este ciclo recorre el historial y busca la primera coincidencia
    for (auto it = historial.begin(); it != historial.end(); ++it) {
        if ((*it)->getTitulo().find(filtro) != std::string::npos ||
            (*it)->getUrl().find(filtro) != std::string::npos) { 
            posicionActual = it; 
            coincidenciaEncontrada = true; 
            break;
        }
    }

    // Si no se encontro una coincidencia movemos la posicion actual al final

    if (!coincidenciaEncontrada) {
        posicionActual = historial.end();
    }
}

Historial* Historial::cargarArchivoHistorial(std::ifstream& in)
{
    Historial* nuevoHistorial = new Historial();

    size_t historialSize;
    in.read(reinterpret_cast<char*>(&historialSize), sizeof(historialSize));

    for (size_t i = 0; i < historialSize; ++i) {
        SitioWeb* sitio = SitioWeb::cargarArchivoSitioWeb(in);
        nuevoHistorial->add(sitio);
        delete sitio;
    }

    size_t posicionActualIdx;
    in.read(reinterpret_cast<char*>(&posicionActualIdx), sizeof(posicionActualIdx));

    if (posicionActualIdx < nuevoHistorial->historial.size()) {
        nuevoHistorial->posicionActual = nuevoHistorial->historial.begin();
        std::advance(nuevoHistorial->posicionActual, posicionActualIdx);
    }
    else {
        nuevoHistorial->posicionActual = nuevoHistorial->historial.end();
    }

    size_t filtroLength;
    in.read(reinterpret_cast<char*>(&filtroLength), sizeof(filtroLength));
    nuevoHistorial->filtro.resize(filtroLength);
    in.read(&nuevoHistorial->filtro[0], filtroLength);


    return nuevoHistorial;
}

void Historial::guardarArchivoHistorial(std::ofstream& out)
{
    size_t historialSize = historial.size();
    out.write(reinterpret_cast<const char*>(&historialSize), sizeof(historialSize));

    for (SitioWeb* sitio : historial) {
        if (sitio) {
            sitio->guardarArchivoSitioWeb(out); 
        }
    }

    auto posicionActualIdx = std::distance(historial.begin(), posicionActual);
    out.write(reinterpret_cast<const char*>(&posicionActualIdx), sizeof(posicionActualIdx));

    size_t filtroLength = filtro.size();
    out.write(reinterpret_cast<const char*>(&filtroLength), sizeof(filtroLength));
    out.write(filtro.c_str(), filtroLength);  
}


std::string Historial::getUrlActual() const
{
    if (posicionActual != historial.end()) {
        return (*posicionActual)->getUrl();
    }
    return "";
}

std::string Historial::getTituloActual() const
{
    if (posicionActual != historial.end()) {
		return (*posicionActual)->getTitulo();
	}
    return "";
}

std::string Historial::getDominioActual() const
{
    if (posicionActual != historial.end()) {
		return (*posicionActual)->getDominio();
	}
	return "";
}




