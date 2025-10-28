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
        
        // Crear métodos RPC
        new MetodoLogin(servidor, this);
        new MetodoConectarRobot(servidor, this);
        new MetodoConfigurearAccesoRemoto(servidor, this);
        new MetodoControlMotores(servidor, this);
        new MetodoListarComandos(servidor, this);
        new MetodoReporteUsuario(servidor, this);
        new MetodoReporteAdmin(servidor, this);
        new MetodoConfigurarModo(servidor, this);
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
    
    Usuario usr = gestorBBDD->obtenerUsuario(usuario);
    if (usr.getNombre().empty()) {
        registrarEvento("Usuario no encontrado: " + usuario, usuario, nodoOrigen);
        return false;
    }
    
    if (usr.getClave() != clave) {
        registrarEvento("Clave incorrecta para usuario: " + usuario, usuario, nodoOrigen);
        return false;
    }
    
    return true;
}

bool ServidorRpc::esAdministrador(const std::string& sessionId) {
    auto it = sesionesActivas.find(sessionId);
    if (it == sesionesActivas.end()) {
        return false;
    }
    return it->second.tipoUsuario == Usuario::TipoUsuario::ADMIN;
}

void ServidorRpc::registrarEvento(const std::string& evento, const std::string& usuario, const std::string& nodo) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    
    gestorReportes->registrarLog(ss.str(), "ServidorRPC", evento, usuario, nodo);
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
        
        Usuario usr = servidor->gestorBBDD->obtenerUsuario(usuario);
        
        SesionUsuario sesion;
        sesion.usuario = usuario;
        sesion.nodoOrigen = nodoOrigen;
        sesion.tipoUsuario = usr.getTipo();
        sesion.tiempoConexion = std::chrono::system_clock::now();
        sesion.comandosEjecutados = 0;
        sesion.comandosErroneos = 0;
        
        servidor->sesionesActivas[sessionId] = sesion;
        
        result["exito"] = true;
        result["sessionId"] = sessionId;
        result["tipoUsuario"] = (usr.getTipo() == Usuario::TipoUsuario::ADMIN) ? "admin" : "normal";
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

// Implementación de MetodoListarComandos
void MetodoListarComandos::execute(XmlRpcValue& params, XmlRpcValue& result) {
    if (params.size() < 1) {
        result["exito"] = false;
        result["mensaje"] = "Parámetros insuficientes: [sessionId]";
        return;
    }
    
    std::string sessionId = params[0];
    auto it = servidor->sesionesActivas.find(sessionId);
    if (it == servidor->sesionesActivas.end()) {
        result["exito"] = false;
        result["mensaje"] = "Sesión inválida";
        return;
    }
    
    bool esAdmin = servidor->esAdministrador(sessionId);
    
    XmlRpcValue comandos;
    comandos["Login"] = "Autenticación: [usuario, clave, nodoOrigen]";
    comandos["ListarComandos"] = "Lista comandos disponibles: [sessionId]";
    comandos["ReporteUsuario"] = "Reporte personal: [sessionId]";
    comandos["ConfigurarModo"] = "Modo trabajo: [sessionId, modo, coordenadas]";
    comandos["MoverRobot"] = "Mover robot: [sessionId, x, y, z, velocidad]";
    comandos["EjecutarGCode"] = "Ejecutar G-Code: [sessionId, comando]";
    
    if (esAdmin) {
        comandos["ConectarRobot"] = "Conectar robot: [sessionId, accion]";
        comandos["ConfigurarAccesoRemoto"] = "Acceso remoto: [sessionId, habilitar]";
        comandos["ControlMotores"] = "Control motores: [sessionId, accion]";
        comandos["ReporteAdmin"] = "Reporte admin: [sessionId, filtro1, filtro2]";
    }
    
    result["exito"] = true;
    result["comandos"] = comandos;
    result["tipoUsuario"] = esAdmin ? "admin" : "normal";
}

std::string MetodoListarComandos::help() {
    return "Lista comandos disponibles según privilegios. Parámetros: [sessionId]";
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

// Implementación de MetodoReporteUsuario
void MetodoReporteUsuario::execute(XmlRpcValue& params, XmlRpcValue& result) {
    if (params.size() < 1) {
        result["exito"] = false;
        result["mensaje"] = "Parámetros insuficientes: [sessionId]";
        return;
    }
    
    std::string sessionId = params[0];
    auto it = servidor->sesionesActivas.find(sessionId);
    if (it == servidor->sesionesActivas.end()) {
        result["exito"] = false;
        result["mensaje"] = "Sesión inválida";
        return;
    }
    
    const SesionUsuario& sesion = it->second;
    
    result["exito"] = true;
    result["usuario"] = sesion.usuario;
    result["comandosEjecutados"] = sesion.comandosEjecutados;
    result["comandosErroneos"] = sesion.comandosErroneos;
    result["estadoRobot"] = servidor->gestorRobot->obtenerEstadoRobot();
    result["posicionActual"] = "X:" + std::to_string(servidor->gestorRobot->obtenerPosicionActual().x) + 
                              " Y:" + std::to_string(servidor->gestorRobot->obtenerPosicionActual().y) + 
                              " Z:" + std::to_string(servidor->gestorRobot->obtenerPosicionActual().z);
}

std::string MetodoReporteUsuario::help() {
    return "Reporte de actividad del usuario. Parámetros: [sessionId]";
}

// Implementación de MetodoConfigurarModo
void MetodoConfigurarModo::execute(XmlRpcValue& params, XmlRpcValue& result) {
    if (params.size() < 3) {
        result["exito"] = false;
        result["mensaje"] = "Parámetros insuficientes: [sessionId, modoTrabajo, modoCoordenadas]";
        return;
    }
    
    std::string sessionId = params[0];
    std::string modoTrabajo = params[1]; // "manual" o "automatico"
    std::string modoCoordenadas = params[2]; // "absoluto" o "relativo"
    
    auto it = servidor->sesionesActivas.find(sessionId);
    if (it == servidor->sesionesActivas.end()) {
        result["exito"] = false;
        result["mensaje"] = "Sesión inválida";
        return;
    }
    
    bool exito = true;
    
    if (modoTrabajo == "manual") {
        exito &= servidor->gestorRobot->configurarModoTrabajo(ModoTrabajo::MANUAL);
    } else if (modoTrabajo == "automatico") {
        exito &= servidor->gestorRobot->configurarModoTrabajo(ModoTrabajo::AUTOMATICO);
    } else {
        exito = false;
    }
    
    if (modoCoordenadas == "absoluto") {
        exito &= servidor->gestorRobot->configurarModoCoordenadas(ModoCoordenas::ABSOLUTO);
    } else if (modoCoordenadas == "relativo") {
        exito &= servidor->gestorRobot->configurarModoCoordenadas(ModoCoordenas::RELATIVO);
    } else {
        exito = false;
    }
    
    result["exito"] = exito;
    result["mensaje"] = exito ? "Modo configurado" : "Error configurando modo";
    
    servidor->registrarEvento("Configuración modo: " + modoTrabajo + "/" + modoCoordenadas, 
                             it->second.usuario, it->second.nodoOrigen);
}

std::string MetodoConfigurarModo::help() {
    return "Configurar modo de trabajo. Parámetros: [sessionId, modoTrabajo, modoCoordenadas]";
}

// Implementaciones simples para métodos faltantes
void MetodoConfigurearAccesoRemoto::execute(XmlRpcValue& params, XmlRpcValue& result) {
    result["exito"] = true;
    result["mensaje"] = "Método no implementado completamente";
}

std::string MetodoConfigurearAccesoRemoto::help() {
    return "Configurar acceso remoto (solo admin). Parámetros: [sessionId, habilitar]";
}

void MetodoControlMotores::execute(XmlRpcValue& params, XmlRpcValue& result) {
    result["exito"] = true;
    result["mensaje"] = "Método no implementado completamente";
}

std::string MetodoControlMotores::help() {
    return "Control de motores del robot. Parámetros: [sessionId, accion]";
}

void MetodoReporteAdmin::execute(XmlRpcValue& params, XmlRpcValue& result) {
    if (params.size() < 1) {
        result["exito"] = false;
        result["mensaje"] = "Parámetros insuficientes: [sessionId]";
        return;
    }
    
    std::string sessionId = params[0];
    if (!servidor->esAdministrador(sessionId)) {
        result["exito"] = false;
        result["mensaje"] = "Acceso denegado: Solo administradores";
        return;
    }
    
    result["exito"] = true;
    result["totalUsuarios"] = static_cast<int>(servidor->sesionesActivas.size());
    result["reporteGeneral"] = servidor->gestorReportes->reporteGeneral();
}

std::string MetodoReporteAdmin::help() {
    return "Reporte administrativo completo (solo admin). Parámetros: [sessionId, filtro1, filtro2]";
}
