#ifndef SERVIDOR_RPC_H
#define SERVIDOR_RPC_H

#include "../lib/XmlRpc.h"
#include "../inc/Excepciones.h"
#include <string>

namespace Rpc {

    // Clase del modelo para el servidor
    class ServidorRpc {
    private:
        XmlRpc::XmlRpcServer* servidor;
        int puerto;
        
    public:
        ServidorRpc();
        ~ServidorRpc();
        
        void configurarPuerto(int puerto);
        void iniciarServidor();
        void detenerServidor();
        bool estaActivo() const;
    };

    // Metodo ServerTest
    class MetodoServerTest : public XmlRpc::XmlRpcServerMethod {
    public:
        MetodoServerTest(XmlRpc::XmlRpcServer* S) 
            : XmlRpc::XmlRpcServerMethod("ServerTest", S) {}
            
        void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result);
        std::string help();
    };

    // Metodo Eco
    class MetodoEco : public XmlRpc::XmlRpcServerMethod {
    public:
        MetodoEco(XmlRpc::XmlRpcServer* S) 
            : XmlRpc::XmlRpcServerMethod("Eco", S) {}
            
        void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result);
        std::string help();
    };

    // Metodo Sumar
    class MetodoSumar : public XmlRpc::XmlRpcServerMethod {
    public:
        MetodoSumar(XmlRpc::XmlRpcServer* S) 
            : XmlRpc::XmlRpcServerMethod("Sumar", S) {}
            
        void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result);
        std::string help();
    };

} // namespace RpcSquartini

#endif
