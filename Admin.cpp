#include "Admin.h"
#include <algorithm>

Admin::Admin(int id, const std::string &nombre, const std::string &clave, const std::string &privilegios)
    : Usuario(id, nombre, clave, privilegios) {}

bool Admin::HabilitarUsuario(int id, std::vector<Usuario> &listaUsuarios) {
    auto it = std::find_if(listaUsuarios.begin(), listaUsuarios.end(), [id](const Usuario &u){ return u.id == id; });
    if (it != listaUsuarios.end()) {
        it->setActivo(true);
        return true;
    }
    return false;
}

bool Admin::DeshabilitarUsuario(int id, std::vector<Usuario> &listaUsuarios) {
    auto it = std::find_if(listaUsuarios.begin(), listaUsuarios.end(), [id](const Usuario &u){ return u.id == id; });
    if (it != listaUsuarios.end()) {
        it->setActivo(false);
        return true;
    }
    return false;
}
