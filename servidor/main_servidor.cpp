#include "ServidorRpc.h"       // Ya incluye todo lo que necesitamos (GestorBBDD, GestorCodigoG)
#include <iostream>
#include <csignal>
#include <thread>             // <--- Requerido para multi-hilo
#include <string>             // <--- Requerido para la CLI
#include <memory>             // <--- Requerido para el hilo

using namespace Rpc;

ServidorRpc* servidor = nullptr;

/**
 * @brief Función que se ejecutará en un hilo separado
 * para manejar el servidor XML-RPC (que es bloqueante).
 */
void correrServidorRpc(ServidorRpc* srv) {
    try {
        // Esta función (servidor->work) es bloqueante y ocupa el hilo
        srv->iniciarServidor();
    } catch (const std::exception& e) {
        std::cerr << "Error fatal en el hilo del servidor RPC: " << e.what() << std::endl;
    }
}

/**
 * @brief Muestra el menú de ayuda de la CLI local.
 */
void mostrarMenuCli() {
    std::cout << "\n--- PANEL DE CONTROL CLI (Admin) ---" << std::endl;
    std::cout << "Comandos disponibles:" << std::endl;
    std::cout << "  conectar      - Conecta al robot (simulador Arduino)" << std::endl;
    std::cout << "  desconectar   - Desconecta del robot" << std::endl;
    std::cout << "  home          - Mover robot a posicion de origen" << std::endl;
    std::cout << "  mover         - Mover robot a coordenadas X, Y, Z" << std::endl;
    std::cout << "  gripper_on    - Activar efector final (gripper)" << std::endl;
    std::cout << "  gripper_off   - Desactivar efector final (gripper)" << std::endl;
    std::cout << "  motores_on    - Activar motores" << std::endl;
    std::cout << "  motores_off   - Desactivar motores" << std::endl;
    std::cout << "  ayuda         - Muestra este menu" << std::endl;
    std::cout << "  salir         - Cierra la CLI y detiene el servidor" << std::endl;
    std::cout << "--------------------------------------" << std::endl;
}

/**
 * @brief Procesa un comando ingresado en la CLI local.
 * Llama directamente a los gestores del servidor.
 */
void ejecutarComandoCli(const std::string& cmd, ServidorRpc* srv) {
    // Usamos los gestores internos del objeto 'servidor'
    // para asegurar que usamos la misma lógica que el RPC
    
    if (cmd == "conectar") {
        if (srv->gestorRobot->conectarRobot())
            std::cout << ">> Robot conectado." << std::endl;
        else
            std::cout << ">> Error conectando robot." << std::endl;
    
    } else if (cmd == "desconectar") {
        srv->gestorRobot->desconectarRobot();
        std::cout << ">> Robot desconectado." << std::endl;
    
    } else if (cmd == "home") {
        if (srv->gestorRobot->irAPosicionOrigen())
            std::cout << ">> Robot en Home." << std::endl;
        else
            std::cout << ">> Error moviendo a Home (asegurese de 'conectar' primero)." << std::endl;
    
    } else if (cmd == "mover") {
        double x, y, z;
        std::cout << "  Ingrese X: "; std::cin >> x;
        std::cout << "  Ingrese Y: "; std::cin >> y;
        std::cout << "  Ingrese Z: "; std::cin >> z;
        if (srv->gestorRobot->moverEfectorSinVelocidad(x, y, z))
            std::cout << ">> Movimiento ejecutado." << std::endl;
        else
            std::cout << ">> Error en movimiento (fuera de rango o no conectado)." << std::endl;
    
    } else if (cmd == "gripper_on") {
        if (srv->gestorRobot->activarEfectorFinal())
            std::cout << ">> Efector activado." << std::endl;
        else
            std::cout << ">> Error activando efector." << std::endl;
    
    } else if (cmd == "gripper_off") {
        if (srv->gestorRobot->desactivarEfectorFinal())
            std::cout << ">> Efector desactivado." << std::endl;
        else
            std::cout << ">> Error desactivando efector." << std::endl;
    
    } else if (cmd == "motores_on") {
        // Asumiendo que M17 es el comando G-Code para activar motores
        if (srv->gestorRobot->ejecutarComandoGDirecto("M17")) 
            std::cout << ">> Motores activados." << std::endl;
        else
            std::cout << ">> Error activando motores." << std::endl;

    } else if (cmd == "motores_off") {
        // Asumiendo que M18 es el comando G-Code para desactivar motores
        if (srv->gestorRobot->ejecutarComandoGDirecto("M18"))
            std::cout << ">> Motores desactivados." << std::endl;
        else
            std::cout << ">> Error desactivando motores." << std::endl;

    } else if (cmd == "ayuda") {
        mostrarMenuCli();
    
    } else {
        std::cout << ">> Comando desconocido. Escriba 'ayuda'." << std::endl;
    }
}

/**
 * @brief Manejador de señales (Ctrl+C) para cerrar todo.
 */
void signalHandler(int signum) {
    std::cout << "\nRecibida señal de interrupción (" << signum << "). Cerrando servidor..." << std::endl;
    if (servidor) {
        servidor->detenerServidor(); // Esto detiene el hilo de RPC
    }
    // El hilo principal (CLI) terminará solo
    exit(signum);
}

/**
 * @brief Función principal
 */
int main() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    try {
        std::cout << "=== SERVIDOR ROBOT RPC ===" << std::endl;
        std::cout << "Inicializando servidor..." << std::endl;
        
        servidor = new ServidorRpc();
        servidor->configurarPuerto(8080);
        
        // --- INICIAR SERVIDOR RPC EN HILO SEPARADO ---
        // Lanzamos el servidor RPC (bloqueante) en su propio hilo
        std::thread servidorThread(correrServidorRpc, servidor);
        servidorThread.detach(); // Dejamos que el hilo corra libremente
        
        std::cout << "Servidor RPC iniciado en background en puerto 8080." << std::endl;
        
        // --- INICIO DEL PANEL DE CONTROL CLI LOCAL ---
        std::string usuario, clave;
        std::cout << "\n--- PANEL DE CONTROL CLI LOCAL ---" << std::endl;
        std::cout << "Requiere validacion de Administrador local:" << std::endl;
        std::cout << "Usuario: ";
        std::cin >> usuario;
        std::cout << "Clave: ";
        std::cin >> clave;
        
        // Validamos al usuario contra el GestorBBDD
        auto usuarioObj = servidor->gestorBBDD->obtenerUsuarioPorNombre(usuario);
        if (usuarioObj && usuarioObj->validar(clave) && usuarioObj->getTipo() == "admin") {
            
            std::cout << "\nLogin de administrador exitoso. Bienvenido." << std::endl;
            mostrarMenuCli();
            
            // Bucle principal de la CLI
            std::string comando;
            while (true) {
                std::cout << "\nCLI Admin> ";
                std::cin >> comando;
                
                if (comando == "salir") {
                    std::cout << "Cerrando CLI y deteniendo servidor RPC..." << std::endl;
                    break; // Salimos del bucle
                }
                
                ejecutarComandoCli(comando, servidor);
            }

        } else {
            std::cout << "Error: Credenciales de administrador inválidas." << std::endl;
            std::cout << "El servidor RPC seguirá corriendo en background." << std::endl;
            std::cout << "Presione Ctrl+C para detener." << std::endl;
            // Si el login falla, el hilo principal queda "dormido"
            // pero el hilo del servidor RPC sigue vivo.
            std::this_thread::sleep_for(std::chrono::hours(1));
        }

    } catch (const std::exception& e) {
        std::cerr << "Error critico en main: " << e.what() << std::endl;
        return 1;
    }
    
    // Al salir del bucle (con 'salir')
    if (servidor) {
        servidor->detenerServidor(); // Le decimos al hilo RPC que pare
        delete servidor;
    }
    
    std::cout << "Servidor detenido limpiamente." << std::endl;
    return 0;
}