#ifndef GESTORREPORTES_H
#define GESTORREPORTES_H

#include <string>
#include <vector>
#include <map>
#include "GestorArchivos.h"

struct RegistroPeticion {
    std::string timestamp;
    std::string detalle;
    std::string usuario;
    std::string nodo;
    std::string codigoRespuesta;
};

struct RegistroEvento {
    std::string timestamp;
    std::string modulo;
    std::string mensaje;
};

struct EstadoServidor {
    std::string estadoConexion;
    std::string posicionActual;
    std::string estadoActividad;
    std::string inicioActividad;
    int totalOrdenes;
    int ordenesConError;
};

class GestorReportes {
private:
    GestorArchivos gestorArchivos_;
    std::string archivoLog_;
    EstadoServidor estadoActual_;
    
    // Métodos auxiliares
    std::string obtenerTimestamp() const;
    std::vector<RegistroPeticion> leerPeticiones(const std::string& usuario = "") const;
    std::vector<RegistroEvento> leerEventos() const;
    std::vector<RegistroPeticion> filtrarPeticionesPorFecha(
        const std::vector<RegistroPeticion>& peticiones,
        const std::string& fechaDesde,
        const std::string& fechaHasta) const;
    std::vector<RegistroEvento> filtrarEventosPorFecha(
        const std::vector<RegistroEvento>& eventos,
        const std::string& fechaDesde,
        const std::string& fechaHasta) const;
    std::string formatearReporteGeneral(const std::vector<RegistroPeticion>& peticiones) const;
    std::string formatearReporteAdmin(const std::vector<RegistroPeticion>& peticiones) const;
    std::string formatearReporteLog(const std::vector<RegistroEvento>& eventos) const;

public:
    explicit GestorReportes(const std::string& archivoLog = "servidor_log.csv");
    ~GestorReportes();
    
    // Métodos para registrar eventos
    void registrarPeticion(const std::string& detalle, const std::string& usuario, 
                          const std::string& nodo, const std::string& codigoRespuesta);
    void registrarEvento(const std::string& modulo, const std::string& mensaje);
    
    // Métodos para actualizar estado del servidor
    void actualizarEstadoConexion(const std::string& estado);
    void actualizarPosicion(const std::string& posicion);
    void actualizarEstadoActividad(const std::string& estado);
    void incrementarOrdenes(bool esError = false);
    
    // Métodos de reportes requeridos
    std::string reporteGeneral(const std::string& usuario);
    std::string reporteAdmin();
    std::string reporteLog(const std::string& fechaDesde, const std::string& fechaHasta);
    
    // Métodos adicionales de filtrado para admin
    std::string reporteAdminPorUsuario(const std::string& usuario);
    std::string reporteAdminPorCodigo(const std::string& codigo);
};

#endif
