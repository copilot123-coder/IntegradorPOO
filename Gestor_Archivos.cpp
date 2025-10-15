#include "Gestor_Archivos.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <sys/stat.h>

Gestor_Archivos::Gestor_Archivos(const std::string &filename, const std::string &owner)
    : filename(filename), owner(owner) {}

bool Gestor_Archivos::open(const std::string &path) {
    if (fs.is_open()) fs.close();
    fs.clear();
    fs.open(path, std::ios::in | std::ios::out | std::ios::app);
    // si no existe, intentamos crear en modo salida
    if (!fs.is_open()) {
        std::ofstream ofs(path);
        ofs.close();
        fs.open(path, std::ios::in | std::ios::out | std::ios::app);
    }
    return fs.is_open();
}

void Gestor_Archivos::close() {
    if (fs.is_open()) fs.close();
}

std::string Gestor_Archivos::ReadAll() {
    if (!fs.is_open()) return std::string();
    fs.clear();
    fs.seekg(0, std::ios::beg);
    std::ostringstream ss;
    ss << fs.rdbuf();
    return ss.str();
}

bool Gestor_Archivos::WriteLine(const std::string &line) {
    if (!fs.is_open()) return false;
    fs.clear();
    fs.seekp(0, std::ios::end);
    fs << line << '\n';
    fs.flush();
    return true;
}

bool Gestor_Archivos::FileExists(const std::string &path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

bool Gestor_Archivos::ImprimirTablaCSV(const std::string &CSVpath, int N) {
    std::ifstream ifs(CSVpath);
    if (!ifs.is_open()) return false;

    std::string line;
    while (std::getline(ifs, line)) {
        std::vector<std::string> cols;
        std::istringstream ss(line);
        std::string cell;
        while (std::getline(ss, cell, ',')) {
            cols.push_back(cell);
        }

        if (N > 0 && (int)cols.size() < N) cols.resize(N);

        // Imprimir en formato tabular
        for (size_t i = 0; i < cols.size(); ++i) {
            std::cout << std::left << std::setw(15) << cols[i];
        }
        std::cout << std::endl;
    }
    ifs.close();
    return true;
}

Gestor_Archivos::~Gestor_Archivos() {
    if (fs.is_open()) fs.close();
}
