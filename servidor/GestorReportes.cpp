#include "GestorReportes.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

static std::vector<std::vector<std::string>> readCSV(const std::string &path) {
    std::vector<std::vector<std::string>> rows;
    std::ifstream ifs(path);
    if (!ifs.is_open()) return rows;
    std::string line;
    while (std::getline(ifs, line)) {
        std::vector<std::string> cols;
        std::istringstream ss(line);
        std::string cell;
        while (std::getline(ss, cell, ',')) cols.push_back(cell);
        rows.push_back(cols);
    }
    return rows;
}

static std::string csv_join_row(const std::vector<std::string> &r) {
    std::ostringstream oss;
    for (size_t i = 0; i < r.size(); ++i) {
        if (i) oss << ',';
        oss << r[i];
    }
    return oss.str();
}

std::string GestorReportes::ReporteGeneral(){
    // Leer users.csv y logs.csv (si existen) y construir un CSV resumen
    const std::string usersPath = "users.csv";
    const std::string logsPath = "logs.csv";

    auto users = readCSV(usersPath);
    auto logs = readCSV(logsPath);

    std::ostringstream out;
    out << "tipo,valor\n";
    out << "total_usuarios," << users.size() << "\n";
    out << "total_logs," << logs.size() << "\n";

    // Añadir cabecera lista de usuarios (id,nombre,privilegios,activo)
    out << "usuarios_id,nombre,privilegios,activo\n";
    for (const auto &u : users) {
        out << csv_join_row(u) << "\n";
    }

    return out.str();
}

std::string GestorReportes::ReporteAdmin(int idAdmin){
    const std::string usersPath = "users.csv";
    auto users = readCSV(usersPath);
    std::ostringstream out;
    out << "id_admin," << idAdmin << "\n";
    out << "usuarios_asociados\n";
    for (const auto &u : users) {
        // Asumimos que si la columna privilegios contiene "admin:" seguida
        // por el id o contiene 'admin' entonces está asociado.
        if (u.size() >= 3) {
            const std::string &priv = u[2];
            if (priv.find("admin") != std::string::npos || priv.find(std::to_string(idAdmin)) != std::string::npos) {
                out << csv_join_row(u) << "\n";
            }
        }
    }
    return out.str();
}

std::string GestorReportes::ReporteLog(const std::string &FechaDesde, const std::string &FechaHasta){
    const std::string logsPath = "logs.csv";
    auto logs = readCSV(logsPath);
    std::ostringstream out;
    out << "fecha,resto...\n";
    for (const auto &r : logs) {
        if (r.empty()) continue;
        const std::string &fecha = r[0];
        // Suponemos formato ISO YYYY-MM-DD para comparacion lexicografica
        if (fecha >= FechaDesde && fecha <= FechaHasta) {
            out << csv_join_row(r) << "\n";
        }
    }
    return out.str();
}

std::string GestorReportes::Filtrar(const std::string &filtro){
    std::ostringstream out;
    // Buscar en users.csv y logs.csv
    auto users = readCSV("users.csv");
    auto logs = readCSV("logs.csv");

    out << "matches_users\n";
    for (const auto &u : users) {
        std::string row = csv_join_row(u);
        if (row.find(filtro) != std::string::npos) out << row << "\n";
    }
    out << "matches_logs\n";
    for (const auto &r : logs) {
        std::string row = csv_join_row(r);
        if (row.find(filtro) != std::string::npos) out << row << "\n";
    }
    return out.str();
}
