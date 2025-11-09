#include <iostream>
#include "GestorBBDD.h"

int main() {
    GestorBBDD g("servidor_users.db");
    g.inicializar();
    auto u = g.obtenerUsuarioPorNombre("admin");
    if (!u) {
        std::cout << "No se encontró usuario admin\n";
        return 1;
    }
    std::cout << "Usuario encontrado: " << u->getNombre() << " tipo=" << u->getTipo() << "\n";
    std::cout << "Validar clave admin123: " << (u->validar("admin123") ? "OK" : "FAIL") << "\n";
    std::cout << "Validar clave wrong: " << (u->validar("wrong") ? "OK" : "FAIL") << "\n";
    return 0;
}
