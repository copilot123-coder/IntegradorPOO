#include "Usuario.h"

Usuario::Usuario(int id, const std::string& nombre, const std::string& clave, const std::string& tipo)
    : id_(id), nombre_(nombre), clave_(clave), tipo_(tipo) {
}

Usuario::~Usuario() {
}

bool Usuario::validar(const std::string& clave) const {
    return clave_ == clave;
}
