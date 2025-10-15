#include "../inc/ServidorRpc.h"
#include <iostream>

using namespace Rpc;
using namespace XmlRpc;

// Implementacion de ServidorRpc
ServidorRpc::ServidorRpc() : servidor(nullptr), puerto(0) {
    servidor = new XmlRpcServer();
}

ServidorRpc::~ServidorRpc() {
    if (servidor) {
        delete servidor;
    }
}

void ServidorRpc::configurarPuerto(int puerto) {
    if (puerto <= 0 || puerto > 65535) {
        throw ArgumentoException("Puerto invalido");
    }
    this->puerto = puerto;
}

void ServidorRpc::iniciarServidor() {
    try {
        if (!servidor) {
            throw ConexionException("Servidor no inicializado");
        }
        
        // Crear metodos
        MetodoServerTest* serverTest = new MetodoServerTest(servidor);
        MetodoEco* eco = new MetodoEco(servidor);
        MetodoSumar* sumar = new MetodoSumar(servidor);
        
        XmlRpc::setVerbosity(5);
        
        // Crear socket del servidor
        servidor->bindAndListen(puerto);
        servidor->enableIntrospection(true);
        
        std::cout << "Servidor iniciado en puerto " << puerto << std::endl;
        
        // Escuchar peticiones
        servidor->work(-1.0);
        
    } catch (const std::exception& e) {
        throw ConexionException("No se pudo iniciar el servidor");
    }
}

void ServidorRpc::detenerServidor() {
    if (servidor) {
        servidor->shutdown();
    }
}

bool ServidorRpc::estaActivo() const {
    return servidor != nullptr;
}

// Implementacion de MetodoServerTest
void MetodoServerTest::execute(XmlRpcValue& params, XmlRpcValue& result) {
    result = "Hi, soy el servidor RPC !!";
}

std::string MetodoServerTest::help() {
    return std::string("Respondo quien soy cuando no hay argumentos");
}

// Implementacion de MetodoEco
void MetodoEco::execute(XmlRpcValue& params, XmlRpcValue& result) {
    if (params.size() == 0) {
        throw ArgumentoException("Se necesita al menos un argumento");
    }
    
    std::string resultString = "Hola, ";
    resultString += std::string(params[0]);
    resultString += std::string(" ");
    resultString += std::string(params[0]);
    result = resultString;
}

std::string MetodoEco::help() {
    return std::string("Diga algo y recibira un saludo");
}

// Implementacion de MetodoSumar
void MetodoSumar::execute(XmlRpcValue& params, XmlRpcValue& result) {
    int nArgs = params.size();
    if (nArgs == 0) {
        throw ArgumentoException("Se necesita al menos un numero");
    }
    
    double sum = 0.0;
    for (int i = 0; i < nArgs; ++i) {
        sum += double(params[i]);
    }
    result = sum;
}

std::string MetodoSumar::help() {
    return std::string("Indique varios numeros reales separados por espacio");
}
