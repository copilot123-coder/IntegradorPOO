#ifndef EXCEPCIONES_H
#define EXCEPCIONES_H

#include <exception>
#include <string>

namespace Rpc {

    // Excepcion base para errores del sistema RPC
    class RpcException : public std::exception {
    private:
        std::string mensaje;
    
    public:
        RpcException(const std::string& msg) : mensaje(msg) {}
        virtual const char* what() const throw() {
            return mensaje.c_str();
        }
    };

    // Excepcion para errores de conexion
    class ConexionException : public RpcException {
    public:
        ConexionException(const std::string& msg) 
            : RpcException("Error de conexion: " + msg) {}
    };

    // Excepcion para argumentos invalidos
    class ArgumentoException : public RpcException {
    public:
        ArgumentoException(const std::string& msg) 
            : RpcException("Argumento invalido: " + msg) {}
    };

    // Excepcion para metodos no encontrados
    class MetodoException : public RpcException {
    public:
        MetodoException(const std::string& msg) 
            : RpcException("Metodo no encontrado: " + msg) {}
    };

} // namespace RpcSquartini

#endif
