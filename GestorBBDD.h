#ifndef GESTOR_BBDD_H
#define GESTOR_BBDD_H

#include <string>

class GestorBBDD {
public:
    GestorBBDD();
    ~GestorBBDD();

    bool Conectar(const std::string &connStr);
    void Desconectar();
    // EjecutarConsulta devuelve resultados en CSV (rows\n) por simplicidad
    std::string EjecutarConsulta(const std::string &sql);
    // EjecutarComando devuelve numero de filas afectadas (simulado)
    int EjecutarComando(const std::string &sql);

private:
    bool conectado;
    std::string conexion;
};

#endif // GESTOR_BBDD_H
