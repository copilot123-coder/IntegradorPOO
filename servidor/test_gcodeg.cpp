#include "GestorCodigoG.h"
#include <iostream>
#include <string>

void mostrarMenu() {
    std::cout << "\n=== GESTOR CÓDIGO G - MENU PRINCIPAL ===" << std::endl;
    std::cout << "1.  Conectar Robot" << std::endl;
    std::cout << "2.  Configurar Modo Manual" << std::endl;
    std::cout << "3.  Configurar Modo Automático" << std::endl;
    std::cout << "4.  Configurar Coordenadas Absolutas" << std::endl;
    std::cout << "5.  Configurar Coordenadas Relativas" << std::endl;
    std::cout << "6.  Ir a Posición Origen" << std::endl;
    std::cout << "7.  Mover Efector (con velocidad)" << std::endl;
    std::cout << "8.  Mover Efector (sin velocidad)" << std::endl;
    std::cout << "9.  Activar Efector Final" << std::endl;
    std::cout << "10. Desactivar Efector Final" << std::endl;
    std::cout << "11. Iniciar Aprendizaje Trayectoria" << std::endl;
    std::cout << "12. Agregar Paso a Trayectoria" << std::endl;
    std::cout << "13. Agregar Comando G a Trayectoria" << std::endl;
    std::cout << "14. Finalizar Aprendizaje" << std::endl;
    std::cout << "15. Ejecutar Comando G Directo" << std::endl;
    std::cout << "16. Mostrar Estado Robot" << std::endl;
    std::cout << "17. Mostrar Espacio de Trabajo" << std::endl;
    std::cout << "18. Consultar Posición Actual" << std::endl;
    std::cout << "19. Desconectar Robot" << std::endl;
    std::cout << "0.  Salir" << std::endl;
    std::cout << "Opción: ";
}

int main() {
    GestorCodigoG gestor;
    int opcion;
    
    std::cout << "=== TEST GESTOR CÓDIGO G ===" << std::endl;
    std::cout << gestor.obtenerEspacioTrabajoInfo() << std::endl;
    
    do {
        mostrarMenu();
        std::cin >> opcion;
        
        switch (opcion) {
            case 1: {
                std::cout << "Conectando robot..." << std::endl;
                if (gestor.conectarRobot()) {
                    std::cout << "✓ Robot conectado exitosamente" << std::endl;
                } else {
                    std::cout << "✗ Error conectando robot" << std::endl;
                }
                break;
            }
            
            case 2: {
                if (gestor.configurarModoTrabajo(ModoTrabajo::MANUAL)) {
                    std::cout << "✓ Modo MANUAL configurado" << std::endl;
                } else {
                    std::cout << "✗ Error configurando modo manual" << std::endl;
                }
                break;
            }
            
            case 3: {
                if (gestor.configurarModoTrabajo(ModoTrabajo::AUTOMATICO)) {
                    std::cout << "✓ Modo AUTOMÁTICO configurado" << std::endl;
                } else {
                    std::cout << "✗ Error configurando modo automático" << std::endl;
                }
                break;
            }
            
            case 4: {
                if (gestor.configurarModoCoordenadas(ModoCoordenas::ABSOLUTO)) {
                    std::cout << "✓ Coordenadas ABSOLUTAS configuradas" << std::endl;
                } else {
                    std::cout << "✗ Error configurando coordenadas absolutas" << std::endl;
                }
                break;
            }
            
            case 5: {
                if (gestor.configurarModoCoordenadas(ModoCoordenas::RELATIVO)) {
                    std::cout << "✓ Coordenadas RELATIVAS configuradas" << std::endl;
                } else {
                    std::cout << "✗ Error configurando coordenadas relativas" << std::endl;
                }
                break;
            }
            
            case 6: {
                std::cout << "Enviando robot a posición origen..." << std::endl;
                if (gestor.irAPosicionOrigen()) {
                    std::cout << "✓ Robot en posición origen" << std::endl;
                } else {
                    std::cout << "✗ Error moviendo a origen" << std::endl;
                }
                break;
            }
            
            case 7: {
                double x, y, z, vel;
                std::cout << "Ingrese X Y Z Velocidad: ";
                std::cin >> x >> y >> z >> vel;
                
                if (gestor.moverEfectorConVelocidad(x, y, z, vel)) {
                    std::cout << "✓ Movimiento ejecutado" << std::endl;
                } else {
                    std::cout << "✗ Error en movimiento" << std::endl;
                }
                break;
            }
            
            case 8: {
                double x, y, z;
                std::cout << "Ingrese X Y Z: ";
                std::cin >> x >> y >> z;
                
                if (gestor.moverEfectorSinVelocidad(x, y, z)) {
                    std::cout << "✓ Movimiento ejecutado" << std::endl;
                } else {
                    std::cout << "✗ Error en movimiento" << std::endl;
                }
                break;
            }
            
            case 9: {
                if (gestor.activarEfectorFinal()) {
                    std::cout << "✓ Efector final activado" << std::endl;
                } else {
                    std::cout << "✗ Error activando efector" << std::endl;
                }
                break;
            }
            
            case 10: {
                if (gestor.desactivarEfectorFinal()) {
                    std::cout << "✓ Efector final desactivado" << std::endl;
                } else {
                    std::cout << "✗ Error desactivando efector" << std::endl;
                }
                break;
            }
            
            case 11: {
                std::string nombre;
                std::cout << "Nombre de la trayectoria: ";
                std::cin >> nombre;
                
                if (gestor.iniciarAprendizajeTrayectoria(nombre)) {
                    std::cout << "✓ Aprendizaje iniciado: " << nombre << std::endl;
                } else {
                    std::cout << "✗ Error iniciando aprendizaje" << std::endl;
                }
                break;
            }
            
            case 12: {
                double x, y, z, vel;
                std::cout << "Ingrese X Y Z Velocidad (0 para usar actual): ";
                std::cin >> x >> y >> z >> vel;
                
                if (gestor.agregarPasoTrayectoria(x, y, z, vel)) {
                    std::cout << "✓ Paso agregado a trayectoria" << std::endl;
                } else {
                    std::cout << "✗ Error agregando paso" << std::endl;
                }
                break;
            }
            
            case 13: {
                std::string comando;
                std::cout << "Comando G-Code: ";
                std::cin.ignore();
                std::getline(std::cin, comando);
                
                if (gestor.agregarComandoGTrayectoria(comando)) {
                    std::cout << "✓ Comando agregado a trayectoria" << std::endl;
                } else {
                    std::cout << "✗ Error agregando comando" << std::endl;
                }
                break;
            }
            
            case 14: {
                if (gestor.finalizarAprendizajeTrayectoria()) {
                    std::cout << "✓ Trayectoria guardada exitosamente" << std::endl;
                } else {
                    std::cout << "✗ Error guardando trayectoria" << std::endl;
                }
                break;
            }
            
            case 15: {
                std::string comando;
                std::cout << "Comando G-Code directo: ";
                std::cin.ignore();
                std::getline(std::cin, comando);
                
                if (gestor.ejecutarComandoGDirecto(comando)) {
                    std::cout << "✓ Comando ejecutado" << std::endl;
                } else {
                    std::cout << "✗ Error ejecutando comando" << std::endl;
                }
                break;
            }
            
            case 16: {
                std::cout << "\n" << gestor.obtenerEstadoRobot() << std::endl;
                break;
            }
            
            case 17: {
                std::cout << "\n" << gestor.obtenerEspacioTrabajoInfo() << std::endl;
                break;
            }
            
            case 18: {
                Posicion pos = gestor.obtenerPosicionActual();
                std::cout << "Posición actual: X" << pos.x << " Y" << pos.y << " Z" << pos.z << std::endl;
                break;
            }
            
            case 19: {
                gestor.desconectarRobot();
                std::cout << "✓ Robot desconectado" << std::endl;
                break;
            }
            
            case 0: {
                std::cout << "Saliendo..." << std::endl;
                gestor.desconectarRobot();
                break;
            }
            
            default: {
                std::cout << "Opción inválida" << std::endl;
                break;
            }
        }
        
    } while (opcion != 0);
    
    return 0;
}
