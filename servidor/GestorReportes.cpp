#include "GestorReportes.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

// Para la nueva función
#include <vector>
#include <fstream>

GestorReportes::GestorReportes(const std::string& archivo) : archivoLog(archivo) {
// ... (código existente del constructor) ...
// ... (DEJA EL CONSTRUCTOR COMO ESTÁ) ...
    fileStream.open(archivoLog, std::ios_base::app); // Abrir en modo 'append'
    if (!fileStream.is_open()) {
        std::cerr << "Error al abrir el archivo de log: " << archivoLog << std::endl;
    }
}

GestorReportes::~GestorReportes() {
// ... (código existente del destructor) ...
// ... (DEJA EL DESTRUCTOR COMO ESTÁ) ...
    if (fileStream.is_open()) {
        fileStream.close();
    }
}

void GestorReportes::registrarEvento(const std::string& evento, const std::string& usuario, const std::string& nodo, const std::string& modulo) {
// ... (código existente de registrarEvento) ...
// ... (DEJA LA FUNCIÓN COMO ESTÁ) ...
    if (!fileStream.is_open()) {
        std::cerr << "Error: El stream del log no está abierto." << std::endl;
        return;
    }

    // Obtener timestamp actual
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::ostringstream ss_time;
    ss_time << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    
    // Formatear la línea CSV
    std::ostringstream ss_csv;
    ss_csv << "\"" << ss_time.str() << "\","
           << "\"" << modulo << "\","
           << "\"" << evento << "\","
           << "\"" << usuario << "\","
           << "\"" << nodo << "\"";

    fileStream << ss_csv.str() << std::endl;
}

// --- IMPLEMENTACION DE LA NUEVA FUNCION (AÑADIR AL FINAL) ---
std::vector<std::string> GestorReportes::filtrarLog(const std::string& filtro1, const std::string& filtro2) {
    std::vector<std::string> resultados;
    
    // Cerramos temporalmente el stream de escritura para poder leer
    if (fileStream.is_open()) {
        fileStream.close();
    }

    std::ifstream logFile(this->archivoLog);
    if (!logFile.is_open()) {
        // No podemos usar registrarEvento aquí porque cerraría el logFile que acabamos de fallar en abrir
        std::cerr << "Error al abrir el log para lectura: " << this->archivoLog << std::endl;
        
        // Reabrimos para escritura
        fileStream.open(this->archivoLog, std::ios_base::app);
        return resultados; // Retorna vector vacío
    }

    std::string linea;
    while (std::getline(logFile, linea)) {
        // Lógica de filtro simple: la línea debe contener ambos filtros
        // (Si un filtro está vacío, se ignora)
        // Convertimos todo a minúsculas para un filtro case-insensitive
        
        std::string linea_lower = linea;
        std::string f1_lower = filtro1;
        std::string f2_lower = filtro2;
        
        // (Esta es una forma simple de hacerlo, idealmente se usaría std::transform)
        for (char &c : linea_lower) c = std::tolower(c);
        for (char &c : f1_lower) c = std::tolower(c);
        for (char &c : f2_lower) c = std::tolower(c);

        bool match1 = filtro1.empty() || (linea_lower.find(f1_lower) != std::string::npos);
        bool match2 = filtro2.empty() || (linea_lower.find(f2_lower) != std::string::npos);

        if (match1 && match2) {
            resultados.push_back(linea);
        }
    }

    logFile.close();
    
    // Reabrimos el stream para futuras escrituras
    fileStream.open(this->archivoLog, std::ios_base::app);
    
    return resultados;
}
// --- FIN DE LA IMPLEMENTACION ---