#include "Usuario.h"

Usuario::Usuario() : id(-1), nombre(""), clave(""), privilegios(""), activo(false) {}

Usuario::Usuario(int id, const std::string &nombre, const std::string &clave, const std::string &privilegios)
    : id(id), nombre(nombre), clave(clave), privilegios(privilegios), activo(true) {}

bool Usuario::validar(const std::string &clave_a_comparar) const {
    return clave == clave_a_comparar;
}

void Usuario::setActivo(bool a) { activo = a; }
bool Usuario::isActivo() const { return activo; }
