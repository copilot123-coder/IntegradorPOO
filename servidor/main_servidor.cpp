#include "ServidorRpc.h"
#include <iostream>
#include <csignal>

using namespace Rpc;

ServidorRpc* servidor = nullptr;

void signalHandler(int signum) {
    std::cout << "\nRecibida señal de interrupción (" << signum << "). Cerrando servidor..." << std::endl;
    if (servidor) {
        servidor->detenerServidor();
    }
    exit(signum);
}

int main() {
    // Configurar manejador de señales
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    try {
        std::cout << "=== SERVIDOR ROBOT RPC ===" << std::endl;
        std::cout << "Inicializando servidor..." << std::endl;
        
        servidor = new ServidorRpc();
        servidor->configurarPuerto(8080);
        
        std::cout << "Presiona Ctrl+C para detener el servidor" << std::endl;
        
        servidor->iniciarServidor();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    if (servidor) {
        delete servidor;
    }
    
    return 0;
}
