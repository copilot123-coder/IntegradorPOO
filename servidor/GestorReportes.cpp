#include "GestorReportes.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <iostream>

GestorReportes::GestorReportes(const std::string &logPath) : logPath(logPath) {
    tiempoInicio = nowTimestamp();
    fileStream.open(this->logPath, std::ios::app);
    if (!fileStream.is_open()) {
        std::cerr << "Warning: could not open log file: " << this->logPath << std::endl;
    }
}

GestorReportes::~GestorReportes() {
    if (fileStream.is_open()) fileStream.close();
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
    if (fileStream.is_open()) {
        fileStream << line << "\n";
        fileStream.flush();
    } else {
        // Fallback: write directly
        std::ofstream ofs(logPath, std::ios::app);
        if (ofs.is_open()) ofs << line << "\n";
    }
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
    std::ostringstream line;
    line << "\"" << nowTimestamp() << "\",\"REQUEST\",\"" << detalle << "\",\"" << usuario << "\",\"" << nodo << "\",\"" << codigo << "\"";
    appendLogLine(line.str());

    std::lock_guard<std::mutex> lk(mtx);
    Orden o{detalle, codigo};
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
    auto itInicio = tiempoInicioPorUsuario.find(usuario);
    if (itInicio != tiempoInicioPorUsuario.end()) inicio = itInicio->second;
    out << "inicioActividad," << inicio << "\n";

    int total = 0, errores = 0;
    out << "orden_detalle,resultado\n";
    auto it = ordenesPorUsuario.find(usuario);
    if (it != ordenesPorUsuario.end()) {
        for (const auto &o : it->second) {
            out << '"' << o.detalle << "\"," << o.resultado << "\n";
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
        if (!usuarioFilter.empty() && line.find(std::string("\"") + usuarioFilter + "\"") == std::string::npos) continue;
        if (!codigoFilter.empty() && line.find(std::string("\"") + codigoFilter + "\"") == std::string::npos) continue;
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
    std::ostringstream line;
    line << "\"" << nowTimestamp() << "\",\"EVENTO\",\"" << mensaje << "\",\"" << usuario << "\",\"" << nodo << "\",\"\"";
    if (!modulo.empty()) {
        line << "," << modulo;
    }
    appendLogLine(line.str());
}

std::vector<std::string> GestorReportes::filtrarLog(const std::string &filtro1, const std::string &filtro2) {
    std::vector<std::string> resultados;
    std::ifstream ifs(logPath);
    if (!ifs.is_open()) return resultados;
    std::string linea;
    std::string f1 = filtro1; std::string f2 = filtro2;
    std::transform(f1.begin(), f1.end(), f1.begin(), ::tolower);
    std::transform(f2.begin(), f2.end(), f2.begin(), ::tolower);
    while (std::getline(ifs, linea)) {
        std::string low = linea;
        std::transform(low.begin(), low.end(), low.begin(), ::tolower);
        bool m1 = f1.empty() || (low.find(f1) != std::string::npos);
        bool m2 = f2.empty() || (low.find(f2) != std::string::npos);
        if (m1 && m2) resultados.push_back(linea);
    }
    return resultados;
}