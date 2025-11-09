#ifndef USUARIO_H
#define USUARIO_H

#include <string>

class Usuario {
public:
    Usuario() {}
    Usuario(int id, const std::string &nombre, const std::string &claveHash, const std::string &tipo)
        : id(id), nombre(nombre), claveHash(claveHash), tipo(tipo) {}

    int getId() const { return id; }
    const std::string& getNombre() const { return nombre; }
    const std::string& getTipo() const { return tipo; }

    // Para simplicidad usamos comparación directa de texto para la clave
    bool validar(const std::string &clave) const {
        return clave == claveHash; // en producción sustituir por hash/salt
    }

private:
    int id = 0;
    std::string nombre;
    std::string claveHash;
    std::string tipo; // "admin" o "normal"
};

#endif // USUARIO_H
