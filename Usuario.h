#ifndef USUARIO_H
#define USUARIO_H

#include <string>

class Usuario {
public:
    int id;
    std::string nombre;
    std::string clave;
    std::string privilegios;
    bool activo;

    Usuario();
    Usuario(int id, const std::string &nombre, const std::string &clave, const std::string &privilegios);

    // Valida la clave proporcionada
    bool validar(const std::string &clave_a_comparar) const;

    void setActivo(bool a);
    bool isActivo() const;
};

#endif // USUARIO_H
