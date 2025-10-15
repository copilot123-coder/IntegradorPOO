#include "GestorBBDD.h"
#include <iostream>

GestorBBDD::GestorBBDD() : conectado(false) {}
GestorBBDD::~GestorBBDD(){ if(conectado) Desconectar(); }

bool GestorBBDD::Conectar(const std::string &connStr) {
    // Simulamos conexión validando que la cadena no esté vacía
    if (connStr.empty()) return false;
    conexion = connStr;
    conectado = true;
    return true;
}

void GestorBBDD::Desconectar(){ conectado = false; conexion.clear(); }

std::string GestorBBDD::EjecutarConsulta(const std::string &sql){
    if(!conectado) return std::string();
    // Simulación: retornamos cabecera y la misma consulta como fila
    return std::string("result\n") + sql + "\n";
}

int GestorBBDD::EjecutarComando(const std::string &sql){
    if(!conectado) return -1;
    // Simulamos que afecta 1 fila
    return 1;
}
