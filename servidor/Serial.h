#ifndef SERIAL_H
#define SERIAL_H

#include <string>

class Serial {
public:
    Serial();
    ~Serial();
    
    bool abrirPuerto();
    void cerrarPuerto();
    bool enviarComando(const std::string& comando);
    std::string leerPuerto(int timeoutMs = 2000);

private:
    int fd;
    bool configurar();
};

#endif 