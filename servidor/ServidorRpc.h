#ifndef SERVIDOR_RPC_H
#define SERVIDOR_RPC_H

#include "../lib/XmlRpc.h"
#include "../inc/Excepciones.h"
#include "GestorBBDD.h"
#include "GestorReportes.h"
#include "GestorCodigoG.h"
#include "Usuario.h"
#include "Usuario.h"
#include <string>
#include <memory>
#include <map>
#include <vector>
#include <chrono>

namespace Rpc {

    // Estructura para manejar sesiones de usuario
    struct SesionUsuario {
        std::string usuario;
        std::string nodoOrigen;
        bool esAdmin;
        std::chrono::system_clock::time_point tiempoConexion;
        int comandosEjecutados;
        int comandosErroneos;
    };

    // Clase del modelo para el servidor
    class ServidorRpc {
    public:  // Hacer todo público por simplicidad
        XmlRpc::XmlRpcServer* servidor;
        int puerto;
        
        // Gestores integrados
        std::unique_ptr<GestorBBDD> gestorBBDD;
        std::unique_ptr<GestorReportes> gestorReportes;
        std::unique_ptr<GestorCodigoG> gestorRobot;
        
        // Control de acceso y sesiones
        std::map<std::string, SesionUsuario> sesionesActivas;
        bool accesoRemotoHabilitado;
        
        // Estado del servidor
        std::chrono::system_clock::time_point tiempoInicio;
        
    public:
        ServidorRpc();
        ~ServidorRpc();
        
        void configurarPuerto(int puerto);
        void iniciarServidor();
        void detenerServidor();
        bool estaActivo() const;
        
        // Métodos de utilidad para validación
        bool validarUsuario(const std::string& usuario, const std::string& clave, const std::string& nodoOrigen);
        bool esAdministrador(const std::string& sessionId);
        void registrarEvento(const std::string& evento, const std::string& usuario = "", const std::string& nodo = "");
        std::string generarSessionId(const std::string& usuario, const std::string& nodo);
    };

    // Método de autenticación
    class MetodoLogin : public XmlRpc::XmlRpcServerMethod {
    private:
        ServidorRpc* servidor;
    public:
        MetodoLogin(XmlRpc::XmlRpcServer* S, ServidorRpc* srv) 
            : XmlRpc::XmlRpcServerMethod("Login", S), servidor(srv) {}
        void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result);
        std::string help();
    };

    // Método conectar robot (solo admin)
    class MetodoConectarRobot : public XmlRpc::XmlRpcServerMethod {
    private:
        ServidorRpc* servidor;
    public:
        MetodoConectarRobot(XmlRpc::XmlRpcServer* S, ServidorRpc* srv) 
            : XmlRpc::XmlRpcServerMethod("ConectarRobot", S), servidor(srv) {}
        void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result);
        std::string help();
    };

    // Método mover robot (modo manual)
    class MetodoMoverRobot : public XmlRpc::XmlRpcServerMethod {
    private:
        ServidorRpc* servidor;
    public:
        MetodoMoverRobot(XmlRpc::XmlRpcServer* S, ServidorRpc* srv) 
            : XmlRpc::XmlRpcServerMethod("MoverRobot", S), servidor(srv) {}
        void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result);
        std::string help();
    };

    // Método ejecutar G-Code directo
    class MetodoEjecutarGCode : public XmlRpc::XmlRpcServerMethod {
    private:
        ServidorRpc* servidor;
    public:
        MetodoEjecutarGCode(XmlRpc::XmlRpcServer* S, ServidorRpc* srv) 
            : XmlRpc::XmlRpcServerMethod("EjecutarGCode", S), servidor(srv) {}
        void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result);
        std::string help();
    };

    // Método configurar acceso remoto (solo admin)
    class MetodoConfigurarAccesoRemoto : public XmlRpc::XmlRpcServerMethod {
    private:
        ServidorRpc* servidor;
    public:
        MetodoConfigurarAccesoRemoto(XmlRpc::XmlRpcServer* S, ServidorRpc* srv) 
            : XmlRpc::XmlRpcServerMethod("ConfigurarAccesoRemoto", S), servidor(srv) {}
        void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result);
        std::string help();
    };

    // Método control motores
    class MetodoControlMotores : public XmlRpc::XmlRpcServerMethod {
    private:
        ServidorRpc* servidor;
    public:
        MetodoControlMotores(XmlRpc::XmlRpcServer* S, ServidorRpc* srv) 
            : XmlRpc::XmlRpcServerMethod("ControlMotores", S), servidor(srv) {}
        void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result);
        std::string help();
    };

    // Método listar comandos
    class MetodoListarComandos : public XmlRpc::XmlRpcServerMethod {
    private:
        ServidorRpc* servidor;
    public:
        MetodoListarComandos(XmlRpc::XmlRpcServer* S, ServidorRpc* srv) 
            : XmlRpc::XmlRpcServerMethod("ListarComandos", S), servidor(srv) {}
        void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result);
        std::string help();
    };

    // Método reporte usuario
    class MetodoReporteUsuario : public XmlRpc::XmlRpcServerMethod {
    private:
        ServidorRpc* servidor;
    public:
        MetodoReporteUsuario(XmlRpc::XmlRpcServer* S, ServidorRpc* srv) 
            : XmlRpc::XmlRpcServerMethod("ReporteUsuario", S), servidor(srv) {}
        void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result);
        std::string help();
    };

    // Método reporte admin
    class MetodoReporteAdmin : public XmlRpc::XmlRpcServerMethod {
    private:
        ServidorRpc* servidor;
    public:
        MetodoReporteAdmin(XmlRpc::XmlRpcServer* S, ServidorRpc* srv) 
            : XmlRpc::XmlRpcServerMethod("ReporteAdmin", S), servidor(srv) {}
        void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result);
        std::string help();
    };

    // Método configurar modo trabajo
    class MetodoConfigurarModo : public XmlRpc::XmlRpcServerMethod {
    private:
        ServidorRpc* servidor;
    public:
        MetodoConfigurarModo(XmlRpc::XmlRpcServer* S, ServidorRpc* srv) 
            : XmlRpc::XmlRpcServerMethod("ConfigurarModo", S), servidor(srv) {}
        void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result);
        std::string help();
    };

    // Método ir a origen
    class MetodoIrAOrigen : public XmlRpc::XmlRpcServerMethod {
    private:
        ServidorRpc* servidor;
    public:
        MetodoIrAOrigen(XmlRpc::XmlRpcServer* S, ServidorRpc* srv) 
            : XmlRpc::XmlRpcServerMethod("IrAOrigen", S), servidor(srv) {}
        void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result);
        std::string help();
    };

    // Método control efector
    class MetodoControlEfector : public XmlRpc::XmlRpcServerMethod {
    private:
        ServidorRpc* servidor;
    public:
        MetodoControlEfector(XmlRpc::XmlRpcServer* S, ServidorRpc* srv) 
            : XmlRpc::XmlRpcServerMethod("ControlEfector", S), servidor(srv) {}
        void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result);
        std::string help();
    };

    // Método aprender trayectoria
    class MetodoAprenderTrayectoria : public XmlRpc::XmlRpcServerMethod {
    private:
        ServidorRpc* servidor;
    public:
        MetodoAprenderTrayectoria(XmlRpc::XmlRpcServer* S, ServidorRpc* srv) 
            : XmlRpc::XmlRpcServerMethod("AprenderTrayectoria", S), servidor(srv) {}
        void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result);
        std::string help();
    };

    // Método subir archivo G-code
    class MetodoSubirGCode : public XmlRpc::XmlRpcServerMethod {
    private:
        ServidorRpc* servidor;
    public:
        MetodoSubirGCode(XmlRpc::XmlRpcServer* S, ServidorRpc* srv) 
            : XmlRpc::XmlRpcServerMethod("SubirGCode", S), servidor(srv) {}
        void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result);
        std::string help();
    };

    // Método ejecutar archivo G-code (modo automático)
    class MetodoEjecutarArchivo : public XmlRpc::XmlRpcServerMethod {
    private:
        ServidorRpc* servidor;
    public:
        MetodoEjecutarArchivo(XmlRpc::XmlRpcServer* S, ServidorRpc* srv) 
            : XmlRpc::XmlRpcServerMethod("EjecutarArchivo", S), servidor(srv) {}
        void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result);
        std::string help();
    };

} // namespace Rpc

#endif
