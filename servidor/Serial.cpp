#include "Serial.h"
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <iostream>
#include <cstring>
#include <sys/select.h>
#include <errno.h> // Para depurar errores

Serial::Serial() : fd(-1) {
}

Serial::~Serial() {
    cerrarPuerto();
}

bool Serial::abrirPuerto() {
    fd = open("/dev/ttyACM0", O_RDWR | O_NOCTTY);
    if (fd < 0) {
        // Intentar con /dev/ttyUSB0 si /dev/ttyACM0 falla
        fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY);
        if (fd < 0) {
            std::cerr << "Error abriendo puerto serie (probé /dev/ttyACM0 y /dev/ttyUSB0): " << strerror(errno) << std::endl;
            return false;
        }
    }
    
    // Es vital esperar a que el Arduino se reinicie después de abrir el puerto
    std::cout << "Puerto abierto, esperando reinicio de Arduino (2 segundos)..." << std::endl;
    sleep(2); // Espera 2 segundos
    
    if (!configurar()) {
        cerrarPuerto();
        return false;
    }
    
    // NO limpiamos el buffer aquí, para poder leer el mensaje inicial
    // tcflush(fd, TCIOFLUSH); // <-- Esta línea se elimina intencionalmente
    
    std::cout << "Puerto serie configurado y listo" << std::endl;
    return true;
}

void Serial::cerrarPuerto() {
    if (fd >= 0) {
        close(fd);
        fd = -1;
        std::cout << "Puerto serie cerrado" << std::endl;
    }
}

bool Serial::configurar() {
    struct termios tty;
    
    if (tcgetattr(fd, &tty) != 0) {
        std::cerr << "Error obteniendo configuración: " << strerror(errno) << std::endl;
        return false;
    }
    
    // Configurar 115200 baudios, 8N1
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);
    
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag |= (CLOCAL | CREAD); // Ignorar líneas de módem, habilitar lectura
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS; // Sin control de flujo por hardware
    
    // Modo no canónico (raw)
    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;   // No hacer eco
    tty.c_lflag &= ~ECHOE;
    tty.c_lflag &= ~ISIG;   // No interpretar señales
    
    // Sin control de flujo por software
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    
    tty.c_oflag &= ~OPOST; // Salida en modo raw
    
    // Configuración de timeouts de read()
    tty.c_cc[VMIN] = 0;  // read() no bloqueante
    tty.c_cc[VTIME] = 0; // 0 segundos. select() manejará el timeout.
    
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        std::cerr << "Error aplicando configuración: " << strerror(errno) << std::endl;
        return false;
    }
    
    return true;
}

bool Serial::enviarComando(const std::string& comando) {
    if (fd < 0) {
        std::cerr << "Puerto no abierto" << std::endl;
        return false;
    }
    
    // Limpiar buffer de ENTRADA (lo que recibimos) ANTES de enviar
    // Esto es correcto para asegurar que leemos la respuesta al comando actual
    tcflush(fd, TCIFLUSH);
    
    std::string cmd = comando + "\r\n";
    if (write(fd, cmd.c_str(), cmd.length()) != (ssize_t)cmd.length()) {
        std::cerr << "Error enviando comando: " << strerror(errno) << std::endl;
        return false;
    }
    
    // Esperar a que todos los datos se hayan transmitido
    tcdrain(fd);
    
    std::cout << "Comando enviado: " << comando << std::endl;
    return true;
}

std::string Serial::leerPuerto(int timeoutMs) {
    if (fd < 0) {
        return "";
    }
    
    std::string out;
    const int step_ms = 80; // Intervalo de sondeo
    int elapsed = 0;
    
    // Para G-code, detectar fin de respuesta cuando vemos "ok" o "error"
    auto completo = [&](const std::string& s) -> bool {
        return s.find("ok") != std::string::npos ||
               s.find("error") != std::string::npos;
    };
    
    // Este bucle se ejecutará hasta que se agote el timeout *total*
    // O hasta que la función 'completo' devuelva true.
    while (elapsed < timeoutMs) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        struct timeval tv{0, step_ms * 1000}; // timeout de select()
        
        int r = select(fd + 1, &rfds, nullptr, nullptr, &tv);
        
        if (r < 0) {
            // Error en select()
            std::cerr << "Error en select(): " << strerror(errno) << std::endl;
            break;
        }
        
        if (r > 0 && FD_ISSET(fd, &rfds)) {
            // Hay datos disponibles para leer
            char buf[256];
            ssize_t n = read(fd, buf, sizeof(buf));
            
            if (n > 0) {
                out.append(buf, n);
                // Si ya tenemos la respuesta completa, salimos
                if (completo(out)) {
                    break;
                }
            } else if (n < 0) {
                // Error en read()
                std::cerr << "Error en read(): " << strerror(errno) << std::endl;
                break;
            } else {
                // n == 0 (no debería pasar con VMIN=0, VTIME=0 y select())
            }
        } else {
            // r == 0, select() alcanzó su timeout (step_ms)
            elapsed += step_ms;
        }
    }
    
    // Limpiar caracteres de control al final
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) {
        out.pop_back();
    }
    
    return out;
}

