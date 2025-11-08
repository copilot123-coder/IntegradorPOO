#ifndef GESTORREPORTES_H
#define GESTORREPORTES_H

#include <string>
#include <fstream>
#include <vector> // <--- AÑADIR ESTE INCLUDE

class GestorReportes {
private:
    std::string archivoLog;
    std::ofstream fileStream;

public:
    GestorReportes(const std::string& archivo = "servidor_log.csv");
    ~GestorReportes();

    void registrarEvento(const std::string& evento, const std::string& usuario, const std::string& nodo, const std::string& modulo = "ServidorRPC");

    // --- NUEVA FUNCION A AGREGAR ---
    /**
     * @brief Filtra el archivo de log CSV y devuelve las líneas coincidentes.
     * @param filtro1 Criterio de búsqueda 1 (ej: "admin")
     * @param filtro2 Criterio de búsqueda 2 (ej: "Error")
     * @return Vector de strings, cada string es una línea del CSV.
     */
    std::vector<std::string> filtrarLog(const std::string& filtro1, const std::string& filtro2);
    // --- FIN DE LA NUEVA FUNCION ---
};

#endif // GESTORREPORTES_H