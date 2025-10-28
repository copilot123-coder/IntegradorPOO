#pragma once

#include <string>

class Usuario {
public:
    Usuario(int id, const std::string& nombre, const std::string& clave, const std::string& tipo);
    ~Usuario();

    bool validar(const std::string& clave) const;

    // Getters
    int getId() const { return id_; }
    std::string getNombre() const { return nombre_; }
    std::string getTipo() const { return tipo_; }

private:
    int id_;
    std::string nombre_;
    std::string clave_;
    std::string tipo_;  // "admin" o "normal"
};
