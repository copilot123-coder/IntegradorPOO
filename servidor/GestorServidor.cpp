#include "Gestor_Servidor.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

Gestor_Servidor::Gestor_Servidor()
: lines_(), path_(), idx_(0), manualMode_(false),
  sender_([](const std::string& s){ std::cout << "[SIM SEND] " << s << std::endl; return true; })
{}

Gestor_Servidor::Gestor_Servidor(Sender s)
: lines_(), path_(), idx_(0), manualMode_(false), sender_(std::move(s))
{}

Gestor_Servidor::~Gestor_Servidor() = default;

std::string Gestor_Servidor::trim(const std::string& s) {
    std::size_t i = 0, j = s.size();
    while (i < j && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
    while (j > i && std::isspace(static_cast<unsigned char>(s[j-1]))) --j;
    return s.substr(i, j - i);
}

bool Gestor_Servidor::CargarArchivoGCode(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(f, line)) {
        // eliminar retorno de carro si existe
        if (!line.empty() && line.back() == '\r') line.pop_back();

        // quitar comentarios que empiezan con ';' (y contenido entre paréntesis sencillo)
        // simplificado: recorto desde ';'
        auto pos = line.find(';');
        if (pos != std::string::npos) line = line.substr(0, pos);

        // eliminar paréntesis con contenido simple (no anidado) --> remove (...) occurrences
        std::string tmp;
        bool inpar = false;
        for (char c : line) {
            if (c == '(') { inpar = true; continue; }
            if (c == ')') { inpar = false; continue; }
            if (!inpar) tmp.push_back(c);
        }
        line = tmp;

        line = trim(line);
        if (line.empty()) continue;
        lines.push_back(line);
    }

    std::lock_guard<std::mutex> lg(mtx_);
    lines_.swap(lines);
    path_ = path;
    idx_ = 0;
    return true;
}

bool Gestor_Servidor::EnviarLinea(const std::string& gcode) {
    Sender s;
    {
        std::lock_guard<std::mutex> lg(mtx_);
        s = sender_;
    }
    if (!s) return false;
    return s(gcode);
}

std::string Gestor_Servidor::ListaComandos() const {
    std::lock_guard<std::mutex> lg(mtx_);
    std::ostringstream oss;
    for (std::size_t i = 0; i < lines_.size(); ++i) {
        oss << lines_[i];
        if (i + 1 < lines_.size()) oss << '\n';
    }
    return oss.str();
}

void Gestor_Servidor::ModoManual(bool habilitar) {
    manualMode_.store(habilitar ? true : false);
}

bool Gestor_Servidor::ModoAutomatico() {
    // envía las líneas restantes secuencialmente hasta terminar o hasta que
    // manualMode_ sea true. Devuelve true si llegó al final.
    while (true) {
        if (manualMode_.load()) return false;
        std::string line;
        {
            std::lock_guard<std::mutex> lg(mtx_);
            if (idx_ >= lines_.size()) return true; // ya terminó
            line = lines_[idx_];
        }
        bool ok = EnviarLinea(line);
        if (!ok) return false; // fallo en envío
        {
            std::lock_guard<std::mutex> lg(mtx_);
            ++idx_;
        }
    }
}

void Gestor_Servidor::Reset() {
    std::lock_guard<std::mutex> lg(mtx_);
    idx_ = 0;
}

std::size_t Gestor_Servidor::LineasTotales() const {
    std::lock_guard<std::mutex> lg(mtx_);
    return lines_.size();
}

std::size_t Gestor_Servidor::LineaActual() const {
    std::lock_guard<std::mutex> lg(mtx_);
    return idx_;
}

void Gestor_Servidor::SetSender(Sender s) {
    std::lock_guard<std::mutex> lg(mtx_);
    sender_ = std::move(s);
}