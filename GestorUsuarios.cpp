#include "GestorUsuarios.h"
#include <sstream>
#include <algorithm>

GestorUsuarios::GestorUsuarios() : siguienteId(1) {}

bool GestorUsuarios::Agregar(const std::string &usuario) {
    // formato esperado: nombre,clave,privilegios
    std::istringstream ss(usuario);
    std::string nombre, clave, privilegios;
    if (!std::getline(ss, nombre, ',')) return false;
    if (!std::getline(ss, clave, ',')) return false;
    if (!std::getline(ss, privilegios, ',')) {
        privilegios = "";
    }

    // Verificar duplicados por nombre
    for (const auto &u : usuarios) if (u.nombre == nombre) return false;

    Usuario u(siguienteId++, nombre, clave, privilegios);
    usuarios.push_back(u);
    return true;
}

bool GestorUsuarios::Eliminar(int id) {
    auto it = std::remove_if(usuarios.begin(), usuarios.end(), [id](const Usuario &u){ return u.id == id; });
    if (it != usuarios.end()) {
        usuarios.erase(it, usuarios.end());
        return true;
    }
    return false;
}

bool GestorUsuarios::Validar(const std::string &nombre, const std::string &clave) {
    for (const auto &u : usuarios) {
        if (u.nombre == nombre && u.validar(clave) && u.isActivo()) return true;
    }
    return false;
}

std::string GestorUsuarios::BuscarPorNombre(const std::string &nombre) const {
    for (const auto &u : usuarios) {
        if (u.nombre == nombre) {
            std::ostringstream oss;
            oss << u.id << "," << u.nombre << "," << u.privilegios << "," << (u.isActivo() ? "1" : "0");
            return oss.str();
        }
    }
    return std::string();
}

std::vector<Usuario> GestorUsuarios::ObtenerTodos() const { return usuarios; }
