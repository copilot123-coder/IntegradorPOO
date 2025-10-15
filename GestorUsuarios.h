#ifndef GESTOR_USUARIOS_H
#define GESTOR_USUARIOS_H

#include <string>
#include <vector>
#include "Usuario.h"

class GestorUsuarios {
public:
    GestorUsuarios();

    // Agrega usuario desde un string con formato "nombre,clave,privilegios"
    bool Agregar(const std::string &usuario);
    bool Eliminar(int id);
    bool Validar(const std::string &nombre, const std::string &clave);
    // Devuelve información del usuario en formato CSV "id,nombre,privilegios,activo" o string vacío
    std::string BuscarPorNombre(const std::string &nombre) const;

    // Utiles internos
    std::vector<Usuario> ObtenerTodos() const;

private:
    std::vector<Usuario> usuarios;
    int siguienteId;
};

#endif // GESTOR_USUARIOS_H
