#include "ServidorRpc.h"       // Ya incluye todo lo que necesitamos (GestorBBDD, GestorCodigoG)
#include <iostream>
#include <csignal>
#include <thread>             // <--- Requerido para multi-hilo
#include <string>             // <--- Requerido para la CLI
#include <memory>             // <--- Requerido para el hilo
#include <iomanip>            // <--- Para std::setw (reportes)
#include <vector>             // <--- Para std::vector (reportes)

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
 * @brief Muestra el menú de ayuda de la CLI local. (ACTUALIZADO)
 */
void mostrarMenuCli() {
    std::cout << "\n--- PANEL DE CONTROL CLI (Admin) ---" << std::endl;
    std::cout << "Comandos disponibles:" << std::endl;
    std::cout << "  --- Control de Conexión ---" << std::endl;
    std::cout << "  conectar          - Conecta al robot (simulador Arduino)" << std::endl;
    std::cout << "  desconectar       - Desconecta del robot" << std::endl;
    std::cout << "  acceso            - Habilita/Deshabilita el acceso remoto (RPC)" << std::endl;
    std::cout << "  --- Control de Movimiento ---" << std::endl;
    std::cout << "  home              - Mover robot a posicion de origen" << std::endl;
    std::cout << "  mover             - Mover robot a coordenadas X, Y, Z" << std::endl;
    std::cout << "  gripper_on        - Activar efector final (gripper)" << std::endl;
    std::cout << "  gripper_off       - Desactivar efector final (gripper)" << std::endl;
    std::cout << "  motores_on        - Activar motores (M17)" << std::endl;
    std::cout << "  motores_off       - Desactivar motores (M18)" << std::endl;
    std::cout << "  modo              - Configura modo (manual/auto, abs/rel)" << std::endl;
    std::cout << "  --- Modo Automático y Aprendizaje ---" << std::endl;
    std::cout << "  ejecutar          - Ejecuta un archivo G-Code local del servidor" << std::endl;
    std::cout << "  aprender          - Inicia el sub-menu de aprendizaje de trayectoria" << std::endl;
    std::cout << "  --- Reportes ---" << std::endl;
    std::cout << "  reporte_sesiones  - Muestra las sesiones RPC activas" << std::endl;
    std::cout << "  reporte_log       - Filtra y muestra el log CSV del servidor" << std::endl;
    std::cout << "  --- Utilidad ---" << std::endl;
    std::cout << "  ayuda             - Muestra este menu" << std::endl;
    std::cout << "  salir             - Cierra la CLI y detiene el servidor" << std::endl;
    std::cout << "--------------------------------------" << std::endl;
}

/**
 * @brief Limpia el buffer de entrada de std::cin (necesario después de '>>')
 */
void limpiarBufferCin() {
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

/**
 * @brief Procesa un comando ingresado en la CLI local. (ACTUALIZADO)
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
        if (srv->gestorRobot->ejecutarComandoGDirecto("M17")) 
            std::cout << ">> Motores activados." << std::endl;
        else
            std::cout << ">> Error activando motores." << std::endl;

    } else if (cmd == "motores_off") {
        if (srv->gestorRobot->ejecutarComandoGDirecto("M18"))
            std::cout << ">> Motores desactivados." << std::endl;
        else
            std::cout << ">> Error desactivando motores." << std::endl;

    } else if (cmd == "ayuda") {
        mostrarMenuCli();
    
    } 
    
    // --- NUEVOS COMANDOS AÑADIDOS ---

    else if (cmd == "acceso") {
        std::string op;
        std::cout << "  Acceso remoto (habilitar/deshabilitar): ";
        std::cin >> op;
        bool habilitar = (op == "habilitar");
        srv->accesoRemotoHabilitado = habilitar;
        std::string msg = habilitar ? "Acceso remoto HABILITADO" : "Acceso remoto DESHABILITADO";
        std::cout << ">> " << msg << std::endl;
        srv->registrarEvento(msg, "admin_cli", "localhost");
    }

    else if (cmd == "modo") {
        std::string modoT, modoC;
        std::cout << "  Modo de Trabajo (manual/automatico): "; std::cin >> modoT;
        std::cout << "  Modo de Coordenadas (absoluto/relativo): "; std::cin >> modoC;

        bool exito = true;
        if (modoT == "manual") exito &= srv->gestorRobot->configurarModoTrabajo(ModoTrabajo::MANUAL);
        else if (modoT == "automatico") exito &= srv->gestorRobot->configurarModoTrabajo(ModoTrabajo::AUTOMATICO);
        else exito = false;

        if (modoC == "absoluto") exito &= srv->gestorRobot->configurarModoCoordenadas(ModoCoordenas::ABSOLUTO);
        else if (modoC == "relativo") exito &= srv->gestorRobot->configurarModoCoordenadas(ModoCoordenas::RELATIVO);
        else exito = false;
        
        if (exito) std::cout << ">> Modo configurado: " << modoT << " / " << modoC << std::endl;
        else std::cout << ">> Error: Valores de modo invalidos." << std::endl;
    }

    else if (cmd == "ejecutar") {
        std::string nombreArchivo;
        std::cout << "  Nombre del archivo G-Code a ejecutar (ej: mi_trayectoria.gcode): ";
        std::cin >> nombreArchivo;
        if (srv->gestorRobot->cargarArchivoGCode(nombreArchivo))
            std::cout << ">> Archivo '" << nombreArchivo << "' ejecutado." << std::endl;
        else
            std::cout << ">> Error ejecutando archivo (no encontrado o robot no en modo auto)." << std::endl;
    }

    else if (cmd == "aprender") {
        std::string subCmd;
        std::cout << "  --- Sub-menu Aprendizaje ---" << std::endl;
        std::cout << "  iniciar | agregar | finalizar" << std::endl;
        std::cout << "  Sub-comando: "; std::cin >> subCmd;

        if (subCmd == "iniciar") {
            std::string nombre;
            std::cout << "    Nombre de la trayectoria: "; std::cin >> nombre;
            if(srv->gestorRobot->iniciarAprendizajeTrayectoria(nombre))
                std::cout << ">> Aprendizaje iniciado. Archivo: " << nombre << "_admin_cli.gcode" << std::endl;
            else
                std::cout << ">> Error al iniciar aprendizaje." << std::endl;
        
        } else if (subCmd == "agregar") {
            double x, y, z, vel;
            std::cout << "    Ingrese X: "; std::cin >> x;
            std::cout << "    Ingrese Y: "; std::cin >> y;
            std::cout << "    Ingrese Z: "; std::cin >> z;
            std::cout << "    Ingrese Vel (F): "; std::cin >> vel;
            if(srv->gestorRobot->agregarPasoTrayectoria(x, y, z, vel))
                std::cout << ">> Paso agregado a la trayectoria." << std::endl;
            else
                std::cout << ">> Error al agregar paso (aprendizaje no iniciado?)." << std::endl;

        } else if (subCmd == "finalizar") {
            if(srv->gestorRobot->finalizarAprendizajeTrayectoria())
                std::cout << ">> Aprendizaje finalizado y archivo guardado." << std::endl;
            else
                std::cout << ">> Error al finalizar." << std::endl;
        } else {
            std::cout << ">> Sub-comando de aprendizaje desconocido." << std::endl;
        }
    }

    else if (cmd == "reporte_sesiones") {
        std::cout << ">> --- Reporte de Sesiones RPC Activas ---" << std::endl;
        std::cout << "Total de sesiones: " << srv->sesionesActivas.size() << std::endl;
        std::cout << "------------------------------------------" << std::endl;
        int i = 1;
        for (const auto& par : srv->sesionesActivas) {
            const SesionUsuario& s = par.second;
            std::cout << "  Sesion " << i++ << ": " << s.usuario << "@" << s.nodoOrigen << std::endl;
            std::cout << "    Comandos OK: " << s.comandosEjecutados << " | Errores: " << s.comandosErroneos << std::endl;
        }
        std::cout << "------------------------------------------" << std::endl;
    }

    else if (cmd == "reporte_log") {
        std::string f1, f2;
        limpiarBufferCin(); // Limpiamos el 'Enter' del comando 'reporte_log'
        std::cout << "  Filtro 1 (dejar en blanco para omitir): "; 
        std::getline(std::cin, f1);
        std::cout << "  Filtro 2 (dejar en blanco para omitir): "; 
        std::getline(std::cin, f2);

        std::vector<std::string> logs = srv->gestorReportes->filtrarLog(f1, f2);
        
        std::cout << ">> --- Reporte del Log CSV (" << logs.size() << " lineas encontradas) ---" << std::endl;
        for(const std::string& linea : logs) {
            std::cout << "  " << linea << std::endl;
        }
        std::cout << "------------------------------------------------" << std::endl;
    }

    else {
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
    // Salimos de forma un poco forzada para destrabar el std::cin
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
        std::thread servidorThread(correrServidorRpc, servidor);
        servidorThread.detach(); 
        
        std::cout << "Servidor RPC iniciado en background en puerto 8080." << std::endl;
        
        // --- INICIO DEL PANEL DE CONTROL CLI LOCAL ---
        std::string usuario, clave;
        std::cout << "\n--- PANEL DE CONTROL CLI LOCAL ---" << std::endl;
        std::cout << "Requiere validacion de Administrador local:" << std::endl;
        
        // Bucle de login local
        bool adminLogueado = false;
        while (!adminLogueado) {
            std::cout << "Usuario: ";
            std::cin >> usuario;
            std::cout << "Clave: ";
            std::cin >> clave;
            
            // Validamos al usuario contra el GestorBBDD
            auto usuarioObj = servidor->gestorBBDD->obtenerUsuarioPorNombre(usuario);
            if (usuarioObj && usuarioObj->validar(clave) && usuarioObj->getTipo() == "admin") {
                adminLogueado = true;
            } else {
                std::cout << "Error: Credenciales de administrador inválidas. Intente de nuevo." << std::endl;
                if (std::cin.eof()) { // Si el usuario presiona Ctrl+D
                    throw std::runtime_error("Entrada cancelada.");
                }
            }
        }
            
        std::cout << "\nLogin de administrador exitoso. Bienvenido." << std::endl;
        mostrarMenuCli();
        
        // Bucle principal de la CLI
        std::string comando;
        while (true) {
            std::cout << "\nCLI Admin> ";
            std::cin >> comando;
            
            if (std::cin.eof()) { // Si el usuario presiona Ctrl+D
                comando = "salir";
            }

            if (comando == "salir") {
                std::cout << "Cerrando CLI y deteniendo servidor RPC..." << std::endl;
                break; // Salimos del bucle
            }
            
            ejecutarComandoCli(comando, servidor);
        }

    } catch (const std::exception& e) {
        std::cerr << "Error critico en main: " << e.what() << std::endl;
        // No salimos, dejamos que el 'finally' se ejecute
    }
    
    // Al salir del bucle (con 'salir')
    if (servidor) {
        servidor->detenerServidor(); // Le decimos al hilo RPC que pare
        delete servidor;
    }
    
    std::cout << "Servidor detenido limpiamente." << std::endl;
    return 0;
}