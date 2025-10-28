#include "ServidorRpc.h"
#include "../inc/Excepciones.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

using namespace Rpc;
using namespace XmlRpc;

// Implementación de ServidorRpc
ServidorRpc::ServidorRpc() : servidor(nullptr), puerto(0), accesoRemotoHabilitado(true) {
    servidor = new XmlRpcServer();
    
    // Inicializar gestores
    gestorBBDD = std::make_unique<GestorBBDD>();
    gestorReportes = std::make_unique<GestorReportes>();
    gestorRobot = std::make_unique<GestorCodigoG>();
    
    // Inicializar base de datos
    gestorBBDD->inicializar();
    
    tiempoInicio = std::chrono::system_clock::now();
    
    registrarEvento("Servidor RPC iniciado", "SISTEMA", "localhost");
}

ServidorRpc::~ServidorRpc() {
    if (servidor) {
        delete servidor;
    }
}

void ServidorRpc::configurarPuerto(int puerto) {
    if (puerto <= 0 || puerto > 65535) {
        throw ArgumentoException("Puerto inválido");
    }
    this->puerto = puerto;
}

void ServidorRpc::iniciarServidor() {
    try {
        if (!servidor) {
            throw ConexionException("Servidor no inicializado");
        }
        
        // Crear métodos RPC básicos
        new MetodoLogin(servidor, this);
        new MetodoConectarRobot(servidor, this);
        new MetodoMoverRobot(servidor, this);
        new MetodoEjecutarGCode(servidor, this);
        
        XmlRpc::setVerbosity(1);
        
        // Iniciar servidor
        servidor->bindAndListen(puerto);
        servidor->enableIntrospection(true);
        
        std::cout << "=== SERVIDOR RPC ROBOT ===" << std::endl;
        std::cout << "Puerto: " << puerto << std::endl;
        std::cout << "Acceso remoto: " << (accesoRemotoHabilitado ? "Habilitado" : "Deshabilitado") << std::endl;
        std::cout << "Base de datos: Inicializada" << std::endl;
        std::cout << "Servidor esperando conexiones..." << std::endl;
        
        registrarEvento("Servidor RPC en línea en puerto " + std::to_string(puerto), "SISTEMA", "localhost");
        
        // Escuchar peticiones
        servidor->work(-1.0);
        
    } catch (const std::exception& e) {
        registrarEvento("Error iniciando servidor: " + std::string(e.what()), "SISTEMA", "localhost");
        throw ConexionException("No se pudo iniciar el servidor");
    }
}

void ServidorRpc::detenerServidor() {
    if (servidor) {
        registrarEvento("Servidor RPC detenido", "SISTEMA", "localhost");
        servidor->shutdown();
    }
}

bool ServidorRpc::estaActivo() const {
    return servidor != nullptr;
}

bool ServidorRpc::validarUsuario(const std::string& usuario, const std::string& clave, const std::string& nodoOrigen) {
    if (!accesoRemotoHabilitado && nodoOrigen != "localhost" && nodoOrigen != "127.0.0.1") {
        registrarEvento("Acceso remoto denegado desde " + nodoOrigen, usuario, nodoOrigen);
        return false;
    }
    
    // Validación simple - mejorará con integración completa
    if (usuario == "admin" && clave == "admin123") {
        return true;
    }
    if (usuario == "user" && clave == "user123") {
        return true;
    }
    
    registrarEvento("Credenciales inválidas para: " + usuario, usuario, nodoOrigen);
    return false;
}

bool ServidorRpc::esAdministrador(const std::string& sessionId) {
    auto it = sesionesActivas.find(sessionId);
    if (it == sesionesActivas.end()) {
        return false;
    }
    return it->second.esAdmin;
}

void ServidorRpc::registrarEvento(const std::string& evento, const std::string& usuario, const std::string& nodo) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    
    std::cout << "[" << ss.str() << "] " << evento << " (Usuario: " << usuario << ", Nodo: " << nodo << ")" << std::endl;
}

std::string ServidorRpc::generarSessionId(const std::string& usuario, const std::string& nodo) {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return usuario + "_" + std::to_string(now);
}

// Implementación de MetodoLogin
void MetodoLogin::execute(XmlRpcValue& params, XmlRpcValue& result) {
    if (params.size() < 3) {
        result["exito"] = false;
        result["mensaje"] = "Parámetros insuficientes: [usuario, clave, nodoOrigen]";
        return;
    }
    
    std::string usuario = params[0];
    std::string clave = params[1];
    std::string nodoOrigen = params[2];
    
    if (servidor->validarUsuario(usuario, clave, nodoOrigen)) {
        std::string sessionId = servidor->generarSessionId(usuario, nodoOrigen);
        
        SesionUsuario sesion;
        sesion.usuario = usuario;
        sesion.nodoOrigen = nodoOrigen;
        sesion.esAdmin = (usuario == "admin");
        sesion.tiempoConexion = std::chrono::system_clock::now();
        sesion.comandosEjecutados = 0;
        sesion.comandosErroneos = 0;
        
        servidor->sesionesActivas[sessionId] = sesion;
        
        result["exito"] = true;
        result["sessionId"] = sessionId;
        result["tipoUsuario"] = sesion.esAdmin ? "admin" : "normal";
        result["mensaje"] = "Autenticación exitosa";
        
        servidor->registrarEvento("Login exitoso", usuario, nodoOrigen);
    } else {
        result["exito"] = false;
        result["mensaje"] = "Credenciales inválidas o acceso denegado";
    }
}

std::string MetodoLogin::help() {
    return "Autenticación de usuario. Parámetros: [usuario, clave, nodoOrigen]";
}

// Implementación de MetodoConectarRobot
void MetodoConectarRobot::execute(XmlRpcValue& params, XmlRpcValue& result) {
    if (params.size() < 2) {
        result["exito"] = false;
        result["mensaje"] = "Parámetros insuficientes: [sessionId, accion]";
        return;
    }
    
    std::string sessionId = params[0];
    std::string accion = params[1]; // "conectar" o "desconectar"
    
    if (!servidor->esAdministrador(sessionId)) {
        result["exito"] = false;
        result["mensaje"] = "Acceso denegado: Solo administradores";
        servidor->registrarEvento("Intento de conexión robot sin permisos", "", "");
        return;
    }
    
    bool exito = false;
    if (accion == "conectar") {
        exito = servidor->gestorRobot->conectarRobot();
        result["mensaje"] = exito ? "Robot conectado" : "Error conectando robot";
    } else if (accion == "desconectar") {
        servidor->gestorRobot->desconectarRobot();
        exito = true;
        result["mensaje"] = "Robot desconectado";
    } else {
        result["mensaje"] = "Acción inválida: usar 'conectar' o 'desconectar'";
    }
    
    result["exito"] = exito;
    servidor->registrarEvento("Robot " + accion, servidor->sesionesActivas[sessionId].usuario, servidor->sesionesActivas[sessionId].nodoOrigen);
}

std::string MetodoConectarRobot::help() {
    return "Conectar/desconectar robot (solo admin). Parámetros: [sessionId, accion]";
}

// Implementación de MetodoMoverRobot
void MetodoMoverRobot::execute(XmlRpcValue& params, XmlRpcValue& result) {
    if (params.size() < 4) {
        result["exito"] = false;
        result["mensaje"] = "Parámetros insuficientes: [sessionId, x, y, z, velocidad(opcional)]";
        return;
    }
    
    std::string sessionId = params[0];
    auto it = servidor->sesionesActivas.find(sessionId);
    if (it == servidor->sesionesActivas.end()) {
        result["exito"] = false;
        result["mensaje"] = "Sesión inválida";
        return;
    }
    
    double x = params[1];
    double y = params[2];
    double z = params[3];
    double velocidad = (params.size() > 4) ? double(params[4]) : 0;
    
    bool exito;
    if (velocidad > 0) {
        exito = servidor->gestorRobot->moverEfectorConVelocidad(x, y, z, velocidad);
    } else {
        exito = servidor->gestorRobot->moverEfectorSinVelocidad(x, y, z);
    }
    
    result["exito"] = exito;
    result["mensaje"] = exito ? "Movimiento ejecutado" : "Error en movimiento";
    
    if (exito) {
        it->second.comandosEjecutados++;
    } else {
        it->second.comandosErroneos++;
    }
    
    servidor->registrarEvento("Movimiento robot X:" + std::to_string(x) + " Y:" + std::to_string(y) + " Z:" + std::to_string(z), 
                             it->second.usuario, it->second.nodoOrigen);
}

std::string MetodoMoverRobot::help() {
    return "Mover robot en modo manual. Parámetros: [sessionId, x, y, z, velocidad(opcional)]";
}

// Implementación de MetodoEjecutarGCode
void MetodoEjecutarGCode::execute(XmlRpcValue& params, XmlRpcValue& result) {
    if (params.size() < 2) {
        result["exito"] = false;
        result["mensaje"] = "Parámetros insuficientes: [sessionId, comandoG]";
        return;
    }
    
    std::string sessionId = params[0];
    std::string comandoG = params[1];
    
    auto it = servidor->sesionesActivas.find(sessionId);
    if (it == servidor->sesionesActivas.end()) {
        result["exito"] = false;
        result["mensaje"] = "Sesión inválida";
        return;
    }
    
    bool exito = servidor->gestorRobot->ejecutarComandoGDirecto(comandoG);
    
    result["exito"] = exito;
    result["mensaje"] = exito ? "Comando G-Code ejecutado" : "Error ejecutando comando";
    
    if (exito) {
        it->second.comandosEjecutados++;
    } else {
        it->second.comandosErroneos++;
    }
    
    servidor->registrarEvento("Comando G-Code: " + comandoG, it->second.usuario, it->second.nodoOrigen);
}

std::string MetodoEjecutarGCode::help() {
    return "Ejecutar comando G-Code directo. Parámetros: [sessionId, comandoG]";
}
