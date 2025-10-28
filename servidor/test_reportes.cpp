#include "GestorReportes.h"
#include <iostream>

int main() {
    std::cout << "=== TEST GESTOR REPORTES ===" << std::endl;
    
    // Crear gestor de reportes
    GestorReportes gestor("servidor_log.csv");
    
    // Simular algunos estados del servidor
    gestor.actualizarEstadoConexion("Conectado");
    gestor.actualizarPosicion("10,5,2");
    gestor.actualizarEstadoActividad("Activo");
    
    // Simular algunas peticiones adicionales
    gestor.registrarPeticion("G1 X15 Y25 - Test move", "testuser", "192.168.1.105", "200");
    gestor.registrarPeticion("M106 S255 - Fan on", "testuser", "192.168.1.105", "200");
    gestor.registrarPeticion("G28 - Invalid home", "testuser", "192.168.1.105", "ERROR");
    
    std::cout << "\n1. REPORTE GENERAL (usuario: admin)" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << gestor.reporteGeneral("admin") << std::endl;
    
    std::cout << "\n2. REPORTE GENERAL (usuario: juan)" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << gestor.reporteGeneral("juan") << std::endl;
    
    std::cout << "\n3. REPORTE ADMINISTRATIVO" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << gestor.reporteAdmin() << std::endl;
    
    std::cout << "\n4. REPORTE DE LOG (filtrado por fecha)" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << gestor.reporteLog("2024-10-28 08:00:00", "2024-10-28 08:15:00") << std::endl;
    
    std::cout << "\n5. REPORTE ADMIN FILTRADO POR USUARIO (maria)" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << gestor.reporteAdminPorUsuario("maria") << std::endl;
    
    std::cout << "\n6. REPORTE ADMIN FILTRADO POR CÓDIGO (200)" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << gestor.reporteAdminPorCodigo("200") << std::endl;
    
    return 0;
}
