#include "GestorReportes.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>

GestorReportes::GestorReportes(const std::string &logPath) : logPath(logPath) {
    tiempoInicio = nowTimestamp();
}

std::string GestorReportes::nowTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void GestorReportes::appendLogLine(const std::string &line) {
    std::lock_guard<std::mutex> lk(mtx);
    std::ofstream ofs(logPath, std::ios::app);
    if (!ofs.is_open()) return;
    ofs << line << "\n";
}

void GestorReportes::actualizarEstadoConexion(const std::string &estado) {
    std::lock_guard<std::mutex> lk(mtx);
    estadoConexion = estado;
    tiempoInicio = nowTimestamp();
}

void GestorReportes::actualizarPosicion(const std::string &pos) {
    std::lock_guard<std::mutex> lk(mtx);
    posicion = pos;
}

void GestorReportes::actualizarEstadoActividad(const std::string &act) {
    std::lock_guard<std::mutex> lk(mtx);
    estadoActividad = act;
}

void GestorReportes::registrarPeticion(const std::string &detalle, const std::string &usuario,
                                       const std::string &nodo, const std::string &codigo) {
    std::ostringstream csv;
    csv << "\"" << nowTimestamp() << "\",\"REQUEST\",\"" << detalle << "\",\"" << usuario << "\",\"" << nodo << "\",\"" << codigo << "\"";
    appendLogLine(csv.str());

    std::lock_guard<std::mutex> lk(mtx);
    Orden o; o.detalle = detalle; o.resultado = codigo;
    // store per-user
    ordenesPorUsuario[usuario].push_back(o);
}

std::string GestorReportes::reporteGeneral(const std::string &usuario) {
    std::lock_guard<std::mutex> lk(mtx);
    std::ostringstream out;
    out << "usuario," << usuario << "\n";
    out << "estadoConexion," << estadoConexion << "\n";
    out << "posicion," << posicion << "\n";
    out << "estadoActividad," << estadoActividad << "\n";
    std::string inicio = tiempoInicio;
    if (tiempoInicioPorUsuario.find(usuario) != tiempoInicioPorUsuario.end()) inicio = tiempoInicioPorUsuario[usuario];
    out << "inicioActividad," << inicio << "\n";

    int total = 0, errores = 0;
    out << "orden_detalle,resultado\n";
    auto it = ordenesPorUsuario.find(usuario);
    if (it != ordenesPorUsuario.end()) {
        for (const auto &o : it->second) {
            out << "\"" << o.detalle << "\"," << o.resultado << "\n";
            ++total;
            if (o.resultado != "200" && o.resultado != "OK") ++errores;
        }
    }
    out << "total_ordenes," << total << "\n";
    out << "ordenes_erroneas," << errores << "\n";
    return out.str();
}

void GestorReportes::registrarConexionUsuario(const std::string &usuario) {
    std::lock_guard<std::mutex> lk(mtx);
    tiempoInicioPorUsuario[usuario] = nowTimestamp();
    ordenesPorUsuario[usuario].clear();
}

std::string GestorReportes::reporteAdmin() {
    std::ifstream ifs(logPath);
    if (!ifs.is_open()) return "error,missing_log\n";
    std::ostringstream ss;
    std::string line;
    while (std::getline(ifs, line)) ss << line << "\n";
    return ss.str();
}

std::string GestorReportes::reporteLog(const std::string &desde, const std::string &hasta,
                                       const std::string &usuarioFilter, const std::string &codigoFilter) {
    std::ifstream ifs(logPath);
    if (!ifs.is_open()) return "error,missing_log\n";
    std::ostringstream out;
    std::string line;
    while (std::getline(ifs, line)) {
        size_t p1 = line.find('"');
        if (p1 == std::string::npos) continue;
        size_t p2 = line.find('"', p1+1);
        if (p2 == std::string::npos) continue;
        std::string ts = line.substr(p1+1, p2-p1-1);
        if (ts < desde || ts > hasta) continue;
        if (!usuarioFilter.empty() && line.find("\"" + usuarioFilter + "\"") == std::string::npos) continue;
        if (!codigoFilter.empty() && line.find("\"" + codigoFilter + "\"") == std::string::npos) continue;
        out << line << "\n";
    }
    return out.str();
}

std::string GestorReportes::reporteAdminPorUsuario(const std::string &usuario) {
    return reporteLog("0000-00-00 00:00:00", "9999-12-31 23:59:59", usuario, "");
}

std::string GestorReportes::reporteAdminPorCodigo(const std::string &codigo) {
    return reporteLog("0000-00-00 00:00:00", "9999-12-31 23:59:59", "", codigo);
}

void GestorReportes::registrarEvento(const std::string &mensaje, const std::string &usuario, const std::string &nodo, const std::string &modulo) {
    std::ostringstream csv;
    csv << "\"" << nowTimestamp() << "\",";
    csv << "\"EVENTO\",";
    csv << "\"" << mensaje << "\",";
    csv << "\"" << usuario << "\",";
    csv << "\"" << nodo << "\",";
    csv << "\"\""; // codigo vacio
    if (!modulo.empty()) {
        csv << "," << modulo;
    }
    appendLogLine(csv.str());
}
