#ifndef GESTORREPORTES_H
#define GESTORREPORTES_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

struct Orden {
    std::string detalle;
    std::string resultado; // código o texto
};

class GestorReportes {
public:
    explicit GestorReportes(const std::string &logPath);

    // Estado en memoria (para reportes por usuario)
    void actualizarEstadoConexion(const std::string &estado);
    void actualizarPosicion(const std::string &pos);
    void actualizarEstadoActividad(const std::string &act);

    // Registrar peticiones (persisten en CSV y también almacenan en memoria)
    void registrarPeticion(const std::string &detalle, const std::string &usuario,
                           const std::string &nodo, const std::string &codigo);

    // Registrar inicio de conexión para un usuario (resetea órdenes recientes)
    void registrarConexionUsuario(const std::string &usuario);

    // Reportes solicitados por test y por RPC
    std::string reporteGeneral(const std::string &usuario);
    std::string reporteAdmin();
    std::string reporteLog(const std::string &desde, const std::string &hasta,
                           const std::string &usuarioFilter = "", const std::string &codigoFilter = "");

    // Métodos de ayuda para administrador (filtros por usuario o código)
    std::string reporteAdminPorUsuario(const std::string &usuario);
    std::string reporteAdminPorCodigo(const std::string &codigo);
    // Registrar eventos de servidor (persisten en CSV)
    void registrarEvento(const std::string &mensaje, const std::string &usuario = "", const std::string &nodo = "", const std::string &modulo = "");

private:
    std::string logPath;
    std::mutex mtx;
    std::string estadoConexion;
    std::string posicion;
    std::string estadoActividad;
    std::string tiempoInicio; // fallback global
    // track per-user data
    std::unordered_map<std::string, std::vector<Orden>> ordenesPorUsuario;
    std::unordered_map<std::string, std::string> tiempoInicioPorUsuario;

    std::string nowTimestamp();
    void appendLogLine(const std::string &line);
};

#endif // GESTORREPORTES_H
