#include "Serial.h"
#include <iostream>
#include <string>

/**
 * @brief Programa principal para probar la clase Serial.
 *
 * 1. Abre el puerto serie.
 * 2. Lee el mensaje de bienvenida inicial del Arduino.
 * 3. Envía un comando "G28" y lee su respuesta.
 * 4. Entra en un bucle interactivo para enviar comandos desde la consola.
 * 5. Cierra el puerto al salir (con "exit").
 */
int main() {
    Serial miPuertoSerial;

    // 1. Abrir puerto
    if (!miPuertoSerial.abrirPuerto()) {
        std::cerr << "No se pudo abrir el puerto serie." << std::endl;
        return 1;
    }

    // 2. Leer mensaje inicial
    // El 'sleep(2)' en abrirPuerto() ya esperó el reinicio.
    // Leemos durante 1.5s para capturar el mensaje de bienvenida.
    std::cout << "\n--- Leyendo mensaje inicial (espera max 1.5s) ---" << std::endl;
    std::string inicial = miPuertoSerial.leerPuerto(1500);
    if (!inicial.empty()) {
        std::cout << "Inicial: '" << inicial << "'" << std::endl;
    } else {
        std::cout << "No se recibió mensaje inicial (timeout)." << std::endl;
    }
    std::cout << "----------------------------------------------\n" << std::endl;


    // 3. Enviar G28
    std::cout << "--- Enviando comando G28 ---" << std::endl;
    if (miPuertoSerial.enviarComando("G28")) {
        // Esperamos la respuesta. Los comandos G-code pueden tardar.
        // Damos un timeout generoso (ej. 5 segundos) para la calibración.
        std::string respuestaG28 = miPuertoSerial.leerPuerto(3500);
        std::cout << "Respuesta G28: '" << respuestaG28 << "'" << std::endl;
    } else {
        std::cerr << "Error al enviar G28" << std::endl;
    }
    std::cout << "--------------------------------\n" << std::endl;

    // 4. Bucle de comandos por consola
    std::cout << "--- Ingrese comandos G-code (escriba 'exit' para salir) ---" << std::endl;
    std::cout << "> ";
    std::string linea;
    while (std::getline(std::cin, linea)) {
        if (linea == "exit") {
            break;
        }

        if (linea.empty()) {
            std::cout << "> ";
            continue;
        }

        if (miPuertoSerial.enviarComando(linea)) {
            // Esperar respuesta (ej. 5 segundos)
            std::string respuesta = miPuertoSerial.leerPuerto(1000);
            std::cout << "Respuesta: '" << respuesta << "'" << std::endl;
        } else {
            std::cerr << "Error al enviar comando: " << linea << std::endl;
        }
        std::cout << "> "; // Prompt para el siguiente comando
    }

    // 5. Salir
    std::cout << "Saliendo..." << std::endl;
    miPuertoSerial.cerrarPuerto();
    return 0;
}