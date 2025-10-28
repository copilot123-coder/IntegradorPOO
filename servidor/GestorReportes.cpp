#include "GestorReportes.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>

GestorReportes::GestorReportes(const std::string& archivoLog) 
    : gestorArchivos_(archivoLog), archivoLog_(archivoLog) {
    
    // Inicializar estado del servidor
    estadoActual_.estadoConexion = "Desconectado";
    estadoActual_.posicionActual = "0,0,0";
    estadoActual_.estadoActividad = "Inactivo";
    estadoActual_.inicioActividad = obtenerTimestamp();
    estadoActual_.totalOrdenes = 0;
    estadoActual_.ordenesConError = 0;
    
    // Crear archivo log si no existe con encabezados
    if (!gestorArchivos_.exist()) {
        gestorArchivos_.open("w");
        gestorArchivos_.write("Timestamp,Tipo,Detalle,Usuario,Nodo,Codigo,Modulo,Mensaje\n");
        gestorArchivos_.close();
    }
}

GestorReportes::~GestorReportes() {
    gestorArchivos_.close();
}

std::string GestorReportes::obtenerTimestamp() const {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void GestorReportes::registrarPeticion(const std::string& detalle, const std::string& usuario, 
                                     const std::string& nodo, const std::string& codigoRespuesta) {
    gestorArchivos_.open("a");
    std::ostringstream linea;
    linea << obtenerTimestamp() << ",PETICION," << detalle << "," << usuario << "," 
          << nodo << "," << codigoRespuesta << ",,\n";
    gestorArchivos_.write(linea.str());
    gestorArchivos_.close();
    
    // Actualizar contadores
    incrementarOrdenes(codigoRespuesta != "200" && codigoRespuesta != "OK");
}

void GestorReportes::registrarEvento(const std::string& modulo, const std::string& mensaje) {
    gestorArchivos_.open("a");
    std::ostringstream linea;
    linea << obtenerTimestamp() << ",EVENTO,,,,,," << modulo << "," << mensaje << "\n";
    gestorArchivos_.write(linea.str());
    gestorArchivos_.close();
}

void GestorReportes::actualizarEstadoConexion(const std::string& estado) {
    estadoActual_.estadoConexion = estado;
    registrarEvento("CONEXION", "Estado cambiado a: " + estado);
}

void GestorReportes::actualizarPosicion(const std::string& posicion) {
    estadoActual_.posicionActual = posicion;
}

void GestorReportes::actualizarEstadoActividad(const std::string& estado) {
    if (estadoActual_.estadoActividad != estado) {
        estadoActual_.estadoActividad = estado;
        if (estado == "Activo") {
            estadoActual_.inicioActividad = obtenerTimestamp();
            estadoActual_.totalOrdenes = 0;
            estadoActual_.ordenesConError = 0;
        }
        registrarEvento("ACTIVIDAD", "Estado cambiado a: " + estado);
    }
}

void GestorReportes::incrementarOrdenes(bool esError) {
    estadoActual_.totalOrdenes++;
    if (esError) {
        estadoActual_.ordenesConError++;
    }
}

std::vector<RegistroPeticion> GestorReportes::leerPeticiones(const std::string& usuario) const {
    std::vector<RegistroPeticion> peticiones;
    
    if (!gestorArchivos_.exist()) {
        return peticiones;
    }
    
    GestorArchivos lector(archivoLog_);
    lector.open("r");
    
    std::string linea;
    bool primeraLinea = true;
    
    while (!(linea = lector.getLine()).empty()) {
        if (primeraLinea) {
            primeraLinea = false;
            continue; // Saltar encabezados
        }
        
        // Parsear CSV simple
        std::stringstream ss(linea);
        std::string item;
        std::vector<std::string> tokens;
        
        while (std::getline(ss, item, ',')) {
            tokens.push_back(item);
        }
        
        if (tokens.size() >= 6 && tokens[1] == "PETICION") {
            RegistroPeticion reg;
            reg.timestamp = tokens[0];
            reg.detalle = tokens[2];
            reg.usuario = tokens[3];
            reg.nodo = tokens[4];
            reg.codigoRespuesta = tokens[5];
            
            // Filtrar por usuario si se especifica
            if (usuario.empty() || reg.usuario == usuario) {
                peticiones.push_back(reg);
            }
        }
    }
    
    lector.close();
    return peticiones;
}

std::vector<RegistroEvento> GestorReportes::leerEventos() const {
    std::vector<RegistroEvento> eventos;
    
    if (!gestorArchivos_.exist()) {
        return eventos;
    }
    
    GestorArchivos lector(archivoLog_);
    lector.open("r");
    
    std::string linea;
    bool primeraLinea = true;
    
    while (!(linea = lector.getLine()).empty()) {
        if (primeraLinea) {
            primeraLinea = false;
            continue;
        }
        
        std::stringstream ss(linea);
        std::string item;
        std::vector<std::string> tokens;
        
        while (std::getline(ss, item, ',')) {
            tokens.push_back(item);
        }
        
        if (tokens.size() >= 8 && tokens[1] == "EVENTO") {
            RegistroEvento reg;
            reg.timestamp = tokens[0];
            reg.modulo = tokens[6];
            reg.mensaje = tokens[7];
            eventos.push_back(reg);
        }
    }
    
    lector.close();
    return eventos;
}

std::string GestorReportes::reporteGeneral(const std::string& usuario) {
    std::ostringstream reporte;
    
    reporte << "=== REPORTE GENERAL - Usuario: " << usuario << " ===\n\n";
    
    // Estado actual del servidor
    reporte << "ESTADO DEL SERVIDOR:\n";
    reporte << "- Conexión: " << estadoActual_.estadoConexion << "\n";
    reporte << "- Posición actual: " << estadoActual_.posicionActual << "\n";
    reporte << "- Estado de actividad: " << estadoActual_.estadoActividad << "\n";
    reporte << "- Inicio de actividad: " << estadoActual_.inicioActividad << "\n";
    reporte << "- Total órdenes desde última conexión: " << estadoActual_.totalOrdenes << "\n";
    reporte << "- Órdenes con error: " << estadoActual_.ordenesConError << "\n\n";
    
    // Detalle de peticiones del usuario
    auto peticiones = leerPeticiones(usuario);
    reporte << "DETALLE DE ÓRDENES DEL USUARIO:\n";
    reporte << "Total de órdenes registradas: " << peticiones.size() << "\n\n";
    
    if (!peticiones.empty()) {
        reporte << "Últimas 10 órdenes:\n";
        reporte << std::setw(20) << "Timestamp" << " | "
                << std::setw(30) << "Detalle" << " | "
                << std::setw(10) << "Nodo" << " | "
                << std::setw(8) << "Código" << "\n";
        reporte << std::string(80, '-') << "\n";
        
        int count = 0;
        for (auto it = peticiones.rbegin(); it != peticiones.rend() && count < 10; ++it, ++count) {
            reporte << std::setw(20) << it->timestamp << " | "
                    << std::setw(30) << it->detalle.substr(0, 28) << " | "
                    << std::setw(10) << it->nodo << " | "
                    << std::setw(8) << it->codigoRespuesta << "\n";
        }
        
        // Mostrar errores específicos
        reporte << "\nÓRDENES CON ERROR:\n";
        bool hayErrores = false;
        for (const auto& pet : peticiones) {
            if (pet.codigoRespuesta != "200" && pet.codigoRespuesta != "OK") {
                if (!hayErrores) {
                    reporte << std::setw(20) << "Timestamp" << " | "
                            << std::setw(30) << "Detalle" << " | "
                            << std::setw(8) << "Error" << "\n";
                    reporte << std::string(60, '-') << "\n";
                    hayErrores = true;
                }
                reporte << std::setw(20) << pet.timestamp << " | "
                        << std::setw(30) << pet.detalle.substr(0, 28) << " | "
                        << std::setw(8) << pet.codigoRespuesta << "\n";
            }
        }
        if (!hayErrores) {
            reporte << "No hay órdenes con error.\n";
        }
    }
    
    return reporte.str();
}

std::string GestorReportes::reporteAdmin() {
    std::ostringstream reporte;
    
    reporte << "=== REPORTE ADMINISTRATIVO ===\n\n";
    
    // Estado del servidor (igual que reporte general)
    reporte << "ESTADO DEL SERVIDOR:\n";
    reporte << "- Conexión: " << estadoActual_.estadoConexion << "\n";
    reporte << "- Posición actual: " << estadoActual_.posicionActual << "\n";
    reporte << "- Estado de actividad: " << estadoActual_.estadoActividad << "\n";
    reporte << "- Inicio de actividad: " << estadoActual_.inicioActividad << "\n";
    reporte << "- Total órdenes desde última conexión: " << estadoActual_.totalOrdenes << "\n";
    reporte << "- Órdenes con error: " << estadoActual_.ordenesConError << "\n\n";
    
    // Todas las peticiones (sin restricción de usuario)
    auto todasPeticiones = leerPeticiones();
    reporte << "RESUMEN GENERAL DE PETICIONES:\n";
    reporte << "Total de peticiones registradas: " << todasPeticiones.size() << "\n";
    
    // Estadísticas por usuario
    std::map<std::string, int> peticionesPorUsuario;
    std::map<std::string, int> erroresPorUsuario;
    
    for (const auto& pet : todasPeticiones) {
        peticionesPorUsuario[pet.usuario]++;
        if (pet.codigoRespuesta != "200" && pet.codigoRespuesta != "OK") {
            erroresPorUsuario[pet.usuario]++;
        }
    }
    
    reporte << "\nESTADÍSTICAS POR USUARIO:\n";
    reporte << std::setw(15) << "Usuario" << " | "
            << std::setw(10) << "Peticiones" << " | "
            << std::setw(8) << "Errores" << "\n";
    reporte << std::string(40, '-') << "\n";
    
    for (const auto& pair : peticionesPorUsuario) {
        reporte << std::setw(15) << pair.first << " | "
                << std::setw(10) << pair.second << " | "
                << std::setw(8) << erroresPorUsuario[pair.first] << "\n";
    }
    
    // Estadísticas por nodo
    std::map<std::string, int> peticionesPorNodo;
    for (const auto& pet : todasPeticiones) {
        peticionesPorNodo[pet.nodo]++;
    }
    
    reporte << "\nESTADÍSTICAS POR NODO:\n";
    reporte << std::setw(15) << "Nodo" << " | " << std::setw(10) << "Peticiones" << "\n";
    reporte << std::string(28, '-') << "\n";
    
    for (const auto& pair : peticionesPorNodo) {
        reporte << std::setw(15) << pair.first << " | " << std::setw(10) << pair.second << "\n";
    }
    
    return reporte.str();
}

std::string GestorReportes::reporteLog(const std::string& fechaDesde, const std::string& fechaHasta) {
    std::ostringstream reporte;
    
    reporte << "=== REPORTE DE LOG DEL SERVIDOR ===\n";
    reporte << "Período: " << fechaDesde << " a " << fechaHasta << "\n\n";
    
    auto eventos = leerEventos();
    auto eventosFiltered = filtrarEventosPorFecha(eventos, fechaDesde, fechaHasta);
    
    reporte << "Total de eventos en el período: " << eventosFiltered.size() << "\n\n";
    
    // Estadísticas por módulo
    std::map<std::string, int> eventosPorModulo;
    for (const auto& evento : eventosFiltered) {
        eventosPorModulo[evento.modulo]++;
    }
    
    reporte << "EVENTOS POR MÓDULO:\n";
    reporte << std::setw(15) << "Módulo" << " | " << std::setw(8) << "Cantidad" << "\n";
    reporte << std::string(26, '-') << "\n";
    
    for (const auto& pair : eventosPorModulo) {
        reporte << std::setw(15) << pair.first << " | " << std::setw(8) << pair.second << "\n";
    }
    
    reporte << "\nDETALLE DE EVENTOS:\n";
    reporte << std::setw(20) << "Timestamp" << " | "
            << std::setw(12) << "Módulo" << " | "
            << std::setw(40) << "Mensaje" << "\n";
    reporte << std::string(75, '-') << "\n";
    
    for (const auto& evento : eventosFiltered) {
        reporte << std::setw(20) << evento.timestamp << " | "
                << std::setw(12) << evento.modulo << " | "
                << std::setw(40) << evento.mensaje.substr(0, 38) << "\n";
    }
    
    return reporte.str();
}

std::string GestorReportes::reporteAdminPorUsuario(const std::string& usuario) {
    auto peticiones = leerPeticiones(usuario);
    return formatearReporteAdmin(peticiones);
}

std::string GestorReportes::reporteAdminPorCodigo(const std::string& codigo) {
    auto todasPeticiones = leerPeticiones();
    std::vector<RegistroPeticion> peticionesFiltradas;
    
    for (const auto& pet : todasPeticiones) {
        if (pet.codigoRespuesta == codigo) {
            peticionesFiltradas.push_back(pet);
        }
    }
    
    return formatearReporteAdmin(peticionesFiltradas);
}

std::vector<RegistroEvento> GestorReportes::filtrarEventosPorFecha(
    const std::vector<RegistroEvento>& eventos,
    const std::string& fechaDesde,
    const std::string& fechaHasta) const {
    
    std::vector<RegistroEvento> filtrados;
    
    for (const auto& evento : eventos) {
        if (evento.timestamp >= fechaDesde && evento.timestamp <= fechaHasta) {
            filtrados.push_back(evento);
        }
    }
    
    return filtrados;
}

std::string GestorReportes::formatearReporteAdmin(const std::vector<RegistroPeticion>& peticiones) const {
    std::ostringstream reporte;
    
    reporte << "=== REPORTE FILTRADO ===\n";
    reporte << "Total de registros: " << peticiones.size() << "\n\n";
    
    if (!peticiones.empty()) {
        reporte << std::setw(20) << "Timestamp" << " | "
                << std::setw(15) << "Usuario" << " | "
                << std::setw(25) << "Detalle" << " | "
                << std::setw(10) << "Nodo" << " | "
                << std::setw(8) << "Código" << "\n";
        reporte << std::string(85, '-') << "\n";
        
        for (const auto& pet : peticiones) {
            reporte << std::setw(20) << pet.timestamp << " | "
                    << std::setw(15) << pet.usuario << " | "
                    << std::setw(25) << pet.detalle.substr(0, 23) << " | "
                    << std::setw(10) << pet.nodo << " | "
                    << std::setw(8) << pet.codigoRespuesta << "\n";
        }
    }
    
    return reporte.str();
}
