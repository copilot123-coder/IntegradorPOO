#ifndef ADMIN_H
#define ADMIN_H

#include "Usuario.h"
#include <vector>

class Admin : public Usuario {
public:
    Admin() = default;
    Admin(int id, const std::string &nombre, const std::string &clave, const std::string &privilegios);

    // Habilita usuario por id en una lista (devuelve true si lo encontró y habilitó)
    bool HabilitarUsuario(int id, std::vector<Usuario> &listaUsuarios);
    bool DeshabilitarUsuario(int id, std::vector<Usuario> &listaUsuarios);
};

#endif // ADMIN_H
