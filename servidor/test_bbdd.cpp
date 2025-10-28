#include "GestorBBDD.h"
#include "Usuario.h"
#include <iostream>

int main() {
    // Crear gestor de BD
    GestorBBDD gestor("usuarios.db");
    
    if (!gestor.inicializar()) {
        std::cerr << "Error al inicializar BD" << std::endl;
        return 1;
    }
    
    std::cout << "BD inicializada correctamente" << std::endl;
    
    // Crear usuarios de prueba
    Usuario admin(1, "admin", "admin123", "admin");
    Usuario user1(2, "juan", "juan123", "normal");
    
    // Agregar usuarios
    if (gestor.agregarUsuario(admin)) {
        std::cout << "Usuario admin agregado" << std::endl;
    }
    
    if (gestor.agregarUsuario(user1)) {
        std::cout << "Usuario juan agregado" << std::endl;
    }
    
    // Verificar que existe admin
    if (gestor.existeAdmin()) {
        std::cout << "Admin existe en la BD" << std::endl;
    }
    
    // Obtener usuario por nombre
    auto usuario = gestor.obtenerUsuarioPorNombre("admin");
    if (usuario) {
        std::cout << "Usuario encontrado: " << usuario->getNombre() 
                  << " - Tipo: " << usuario->getTipo() << std::endl;
    }
    
    // Listar todos los usuarios
    auto usuarios = gestor.obtenerTodosUsuarios();
    std::cout << "Total usuarios: " << usuarios.size() << std::endl;
    
    for (const auto& u : usuarios) {
        std::cout << "ID: " << u.getId() << ", Nombre: " << u.getNombre() 
                  << ", Tipo: " << u.getTipo() << std::endl;
    }
    
    return 0;
}
