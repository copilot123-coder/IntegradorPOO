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
    
    // Inicializar gestores (compatible C++11)
    gestorBBDD.reset(new GestorBBDD());
    // Crear gestor de reportes apuntando al CSV dentro de la carpeta servidor
    gestorReportes.reset(new GestorReportes("servidor_log.csv"));
    gestorRobot.reset(new GestorCodigoG());
    
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
        
        // Crear métodos RPC completos
        new MetodoLogin(servidor, this);
        new MetodoConectarRobot(servidor, this);
        new MetodoMoverRobot(servidor, this);
        new MetodoEjecutarGCode(servidor, this);
        new MetodoConfigurarAccesoRemoto(servidor, this);
        new MetodoControlMotores(servidor, this);
        new MetodoListarComandos(servidor, this);
        new MetodoReporteUsuario(servidor, this);
        new MetodoReporteAdmin(servidor, this);
        new MetodoConfigurarModo(servidor, this);
        new MetodoIrAOrigen(servidor, this);
        new MetodoControlEfector(servidor, this);
        new MetodoAprenderTrayectoria(servidor, this);
        new MetodoSubirGCode(servidor, this);
        new MetodoEjecutarArchivo(servidor, this);
        new MetodoReporteLogCsv(servidor, this);
        
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
    
    // Validar contra la base de datos
    auto usuarioObj = gestorBBDD->obtenerUsuarioPorNombre(usuario);
    if (!usuarioObj) {
        registrarEvento("Usuario no encontrado: " + usuario, usuario, nodoOrigen);
        return false;
    }
    
    if (!usuarioObj->validar(clave)) {
        registrarEvento("Credenciales inválidas para: " + usuario, usuario, nodoOrigen);
        return false;
    }
    
    return true;
}

bool ServidorRpc::esAdministrador(const std::string& sessionId) {
    auto it = sesionesActivas.find(sessionId);
    if (it == sesionesActivas.end()) {
        return false;
    }
    
    // Verificar contra la base de datos para mayor seguridad
    auto usuarioObj = gestorBBDD->obtenerUsuarioPorNombre(it->second.usuario);
    if (!usuarioObj) {
        return false;
    }
    
    return usuarioObj->getTipo() == "admin";
}

void ServidorRpc::registrarEvento(const std::string& evento, const std::string& usuario, const std::string& nodo) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    
    std::cout << "[" << ss.str() << "] " << evento << " (Usuario: " << usuario << ", Nodo: " << nodo << ")" << std::endl;
    // Persistir evento en el CSV de reportes
    try {
        if (gestorReportes) gestorReportes->registrarEvento(evento, usuario, nodo, "SERVIDOR");
    } catch (const std::exception &e) {
        std::cerr << "Error al persistir evento en GestorReportes: " << e.what() << std::endl;
    }
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
        
        // Obtener información del usuario desde la base de datos
        auto usuarioObj = servidor->gestorBBDD->obtenerUsuarioPorNombre(usuario);
        bool esAdmin = usuarioObj && (usuarioObj->getTipo() == "admin");
        
        SesionUsuario sesion;
        sesion.usuario = usuario;
        sesion.nodoOrigen = nodoOrigen;
        sesion.esAdmin = esAdmin;
        sesion.tiempoConexion = std::chrono::system_clock::now();
        sesion.comandosEjecutados = 0;
        sesion.comandosErroneos = 0;
        
        servidor->sesionesActivas[sessionId] = sesion;
        
        result["exito"] = true;
        result["sessionId"] = sessionId;
        result["tipoUsuario"] = esAdmin ? "admin" : "normal";
        result["mensaje"] = "Autenticación exitosa";
        
        servidor->registrarEvento("Login exitoso", usuario, nodoOrigen);
        // Notificar al GestorReportes sobre la nueva conexión del usuario
        try {
            if (servidor->gestorReportes) servidor->gestorReportes->registrarConexionUsuario(usuario);
        } catch (const std::exception &e) {
            std::cerr << "Error registrarConexionUsuario: " << e.what() << std::endl;
        }
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
    
    // Actualizar estado de conexión en el gestor de reportes
    try {
        if (servidor->gestorReportes) {
            if (accion == "conectar" && exito) {
                servidor->gestorReportes->actualizarEstadoConexion("robot_conectado");
                servidor->gestorReportes->actualizarEstadoActividad("listo");
            } else if (accion == "desconectar" && exito) {
                servidor->gestorReportes->actualizarEstadoConexion("robot_desconectado");
                servidor->gestorReportes->actualizarEstadoActividad("inactivo");
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "Error actualizando estado conexión: " << e.what() << std::endl;
    }
    
    servidor->registrarEvento("Robot " + accion, servidor->sesionesActivas[sessionId].usuario, servidor->sesionesActivas[sessionId].nodoOrigen);
    // Registrar petición en gestor de reportes
    try {
        if (servidor->gestorReportes) servidor->gestorReportes->registrarPeticion("Robot " + accion, servidor->sesionesActivas[sessionId].usuario, servidor->sesionesActivas[sessionId].nodoOrigen, exito ? "200" : "ERROR");
    } catch (const std::exception &e) {
        std::cerr << "Error registrarPeticion ConectarRobot: " << e.what() << std::endl;
    }
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
        // Actualizar posición en el gestor de reportes
        try {
            if (servidor->gestorReportes) {
                servidor->gestorReportes->actualizarPosicion("X:" + std::to_string(x) + " Y:" + std::to_string(y) + " Z:" + std::to_string(z));
                servidor->gestorReportes->actualizarEstadoActividad("moviendo");
            }
        } catch (const std::exception &e) {
            std::cerr << "Error actualizando posición: " << e.what() << std::endl;
        }
    } else {
        it->second.comandosErroneos++;
    }
    
    servidor->registrarEvento("Movimiento robot X:" + std::to_string(x) + " Y:" + std::to_string(y) + " Z:" + std::to_string(z), 
                             it->second.usuario, it->second.nodoOrigen);
    // Registrar petición en gestor de reportes
    try {
        if (servidor->gestorReportes) servidor->gestorReportes->registrarPeticion("G1 Move X:" + std::to_string(x) + " Y:" + std::to_string(y) + " Z:" + std::to_string(z), it->second.usuario, it->second.nodoOrigen, exito ? "200" : "ERROR");
    } catch (const std::exception &e) {
        std::cerr << "Error registrarPeticion MoverRobot: " << e.what() << std::endl;
    }
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
    // Registrar petición en gestor de reportes
    try {
        if (servidor->gestorReportes) servidor->gestorReportes->registrarPeticion(comandoG, it->second.usuario, it->second.nodoOrigen, exito ? "200" : "ERROR");
    } catch (const std::exception &e) {
        std::cerr << "Error registrarPeticion EjecutarGCode: " << e.what() << std::endl;
    }
}

std::string MetodoEjecutarGCode::help() {
    return "Ejecutar comando G-Code directo. Parámetros: [sessionId, comandoG]";
}

// Implementación de MetodoConfigurarAccesoRemoto
void MetodoConfigurarAccesoRemoto::execute(XmlRpcValue& params, XmlRpcValue& result) {
    if (params.size() < 2) {
        result["exito"] = false;
        result["mensaje"] = "Parámetros insuficientes: [sessionId, habilitar]";
        return;
    }
    
    std::string sessionId = params[0];
    bool habilitar = params[1];
    
    if (!servidor->esAdministrador(sessionId)) {
        result["exito"] = false;
        result["mensaje"] = "Acceso denegado: Solo administradores";
        return;
    }
    
    servidor->accesoRemotoHabilitado = habilitar;
    result["exito"] = true;
    result["mensaje"] = habilitar ? "Acceso remoto habilitado" : "Acceso remoto deshabilitado";
    
    servidor->registrarEvento("Acceso remoto " + std::string(habilitar ? "habilitado" : "deshabilitado"), 
                             servidor->sesionesActivas[sessionId].usuario, 
                             servidor->sesionesActivas[sessionId].nodoOrigen);
    try {
        if (servidor->gestorReportes) servidor->gestorReportes->registrarPeticion(std::string("Acceso remoto ") + (habilitar ? "habilitado" : "deshabilitado"), servidor->sesionesActivas[sessionId].usuario, servidor->sesionesActivas[sessionId].nodoOrigen, "200");
    } catch (const std::exception &e) {
        std::cerr << "Error registrarPeticion ConfigurarAccesoRemoto: " << e.what() << std::endl;
    }
}

std::string MetodoConfigurarAccesoRemoto::help() {
    return "Configurar acceso remoto (solo admin). Parámetros: [sessionId, habilitar]";
}

// Implementación de MetodoControlMotores
void MetodoControlMotores::execute(XmlRpcValue& params, XmlRpcValue& result) {
    if (params.size() < 2) {
        result["exito"] = false;
        result["mensaje"] = "Parámetros insuficientes: [sessionId, accion]";
        return;
    }
    
    std::string sessionId = params[0];
    std::string accion = params[1]; // "activar" o "desactivar"
    
    auto it = servidor->sesionesActivas.find(sessionId);
    if (it == servidor->sesionesActivas.end()) {
        result["exito"] = false;
        result["mensaje"] = "Sesión inválida";
        return;
    }
    
    bool exito = false;
    if (accion == "activar") {
        exito = servidor->gestorRobot->activarEfectorFinal();
        result["mensaje"] = exito ? "Motores activados" : "Error activando motores";
    } else if (accion == "desactivar") {
        exito = servidor->gestorRobot->desactivarEfectorFinal();
        result["mensaje"] = exito ? "Motores desactivados" : "Error desactivando motores";
    } else {
        result["mensaje"] = "Acción inválida: usar 'activar' o 'desactivar'";
    }
    
    result["exito"] = exito;
    servidor->registrarEvento("Motores " + accion, it->second.usuario, it->second.nodoOrigen);
    try {
        if (servidor->gestorReportes) servidor->gestorReportes->registrarPeticion(std::string("Motores ") + accion, it->second.usuario, it->second.nodoOrigen, exito ? "200" : "ERROR");
    } catch (const std::exception &e) {
        std::cerr << "Error registrarPeticion ControlMotores: " << e.what() << std::endl;
    }
}

std::string MetodoControlMotores::help() {
    return "Control de motores del robot. Parámetros: [sessionId, accion]";
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
    comandos["MoverRobot"] = "Mover robot: [sessionId, x, y, z, velocidad]";
    comandos["EjecutarGCode"] = "Ejecutar G-Code: [sessionId, comando]";
    comandos["ConfigurarModo"] = "Modo trabajo: [sessionId, modo, coordenadas]";
    comandos["IrAOrigen"] = "Ir a origen: [sessionId]";
    comandos["ControlEfector"] = "Control efector: [sessionId, accion]";
    comandos["ReporteUsuario"] = "Reporte personal: [sessionId]";
    comandos["AprenderTrayectoria"] = "Aprender trayectoria: [sessionId, accion, nombre]";
    comandos["SubirGCode"] = "Subir archivo: [sessionId, nombre, contenido]";
    comandos["EjecutarArchivo"] = "Ejecutar archivo: [sessionId, archivo]";
    
    if (esAdmin) {
        comandos["ConectarRobot"] = "Conectar robot: [sessionId, accion]";
        comandos["ConfigurarAccesoRemoto"] = "Acceso remoto: [sessionId, habilitar]";
        comandos["ControlMotores"] = "Control motores: [sessionId, accion]";
        comandos["ReporteAdmin"] = "Reporte admin: [sessionId, filtro1, filtro2]";
        comandos["ReporteLogCsv"] = "Log CSV filtrado: [sessionId, desde, hasta, filtroUsuario, filtroCodigo, filtroTexto1, filtroTexto2]";
    }
    
    result["exito"] = true;
    result["comandos"] = comandos;
    result["tipoUsuario"] = esAdmin ? "admin" : "normal";
}

std::string MetodoListarComandos::help() {
    return "Lista comandos disponibles según privilegios. Parámetros: [sessionId]";
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
    auto tiempoConexion = std::chrono::system_clock::to_time_t(sesion.tiempoConexion);
    
    result["exito"] = true;
    result["usuario"] = sesion.usuario;
    result["comandosEjecutados"] = sesion.comandosEjecutados;
    result["comandosErroneos"] = sesion.comandosErroneos;
    result["tiempoConexion"] = std::ctime(&tiempoConexion);
    result["estadoRobot"] = servidor->gestorRobot->obtenerEstadoRobot();
    
    auto pos = servidor->gestorRobot->obtenerPosicionActual();
    result["posicionActual"] = "X:" + std::to_string(pos.x) + " Y:" + std::to_string(pos.y) + " Z:" + std::to_string(pos.z);
    // Incluir reporte general generado por GestorReportes
    try {
        if (servidor->gestorReportes) result["reporteGeneral"] = servidor->gestorReportes->reporteGeneral(sesion.usuario);
    } catch (const std::exception &e) {
        result["reporteGeneral"] = std::string("error: ") + e.what();
    }
}

std::string MetodoReporteUsuario::help() {
    return "Reporte de actividad del usuario. Parámetros: [sessionId]";
}

// Implementación de MetodoReporteAdmin
void MetodoReporteAdmin::execute(XmlRpcValue& params, XmlRpcValue& result) {
    if (params.size() < 1) {
        result["exito"] = false;
        result["mensaje"] = "Parámetros insuficientes: [sessionId, filtro1, filtro2]";
        return;
    }
    
    std::string sessionId = params[0];
    if (!servidor->esAdministrador(sessionId)) {
        result["exito"] = false;
        result["mensaje"] = "Acceso denegado: Solo administradores";
        return;
    }
    
    std::string filtro1 = (params.size() > 1) ? std::string(params[1]) : "";
    std::string filtro2 = (params.size() > 2) ? std::string(params[2]) : "";
    
    result["exito"] = true;
    result["totalSesiones"] = static_cast<int>(servidor->sesionesActivas.size());
    result["filtro1"] = filtro1;
    result["filtro2"] = filtro2;
    
    // Reporte detallado de todas las sesiones
    XmlRpcValue sesiones;
    int indice = 0;
    for (const auto& par : servidor->sesionesActivas) {
        XmlRpcValue sesion;
        sesion["sessionId"] = par.first;
        sesion["usuario"] = par.second.usuario;
        sesion["nodo"] = par.second.nodoOrigen;
        sesion["comandos"] = par.second.comandosEjecutados;
        sesion["errores"] = par.second.comandosErroneos;
        sesiones[indice++] = sesion;
    }
    result["sesiones"] = sesiones;
    // Incluir reportes del GestorReportes: log completo y filtros simples
    try {
        if (servidor->gestorReportes) {
            result["reporteAdmin"] = servidor->gestorReportes->reporteAdmin();
            if (!filtro1.empty()) result["reportePorUsuario"] = servidor->gestorReportes->reporteAdminPorUsuario(filtro1);
            if (!filtro2.empty()) result["reportePorCodigo"] = servidor->gestorReportes->reporteAdminPorCodigo(filtro2);
        }
    } catch (const std::exception &e) {
        result["reporteAdmin"] = std::string("error: ") + e.what();
    }
}

std::string MetodoReporteAdmin::help() {
    return "Reporte administrativo completo (solo admin). Parámetros: [sessionId, filtro1, filtro2]";
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
    result["mensaje"] = exito ? "Modo configurado correctamente" : "Error configurando modo";
    
    servidor->registrarEvento("Configuración modo: " + modoTrabajo + "/" + modoCoordenadas, 
                             it->second.usuario, it->second.nodoOrigen);
}

std::string MetodoConfigurarModo::help() {
    return "Configurar modo de trabajo. Parámetros: [sessionId, modoTrabajo, modoCoordenadas]";
}

// Implementación de MetodoIrAOrigen
void MetodoIrAOrigen::execute(XmlRpcValue& params, XmlRpcValue& result) {
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
    
    bool exito = servidor->gestorRobot->irAPosicionOrigen();
    
    result["exito"] = exito;
    result["mensaje"] = exito ? "Robot en posición origen" : "Error moviendo a origen";
    
    if (exito) {
        it->second.comandosEjecutados++;
        // Actualizar posición en el gestor de reportes
        try {
            if (servidor->gestorReportes) {
                servidor->gestorReportes->actualizarPosicion("X:0.0 Y:0.0 Z:0.0 (origen)");
                servidor->gestorReportes->actualizarEstadoActividad("en_origen");
            }
        } catch (const std::exception &e) {
            std::cerr << "Error actualizando posición origen: " << e.what() << std::endl;
        }
    } else {
        it->second.comandosErroneos++;
    }
    
    servidor->registrarEvento("Ir a origen", it->second.usuario, it->second.nodoOrigen);
    try {
        if (servidor->gestorReportes) servidor->gestorReportes->registrarPeticion("G0 Ir a origen", it->second.usuario, it->second.nodoOrigen, exito ? "200" : "ERROR");
    } catch (const std::exception &e) {
        std::cerr << "Error registrarPeticion IrAOrigen: " << e.what() << std::endl;
    }
}

std::string MetodoIrAOrigen::help() {
    return "Mover robot a posición de origen. Parámetros: [sessionId]";
}

// Implementación de MetodoControlEfector
void MetodoControlEfector::execute(XmlRpcValue& params, XmlRpcValue& result) {
    if (params.size() < 2) {
        result["exito"] = false;
        result["mensaje"] = "Parámetros insuficientes: [sessionId, accion]";
        return;
    }
    
    std::string sessionId = params[0];
    std::string accion = params[1]; // "activar" o "desactivar"
    
    auto it = servidor->sesionesActivas.find(sessionId);
    if (it == servidor->sesionesActivas.end()) {
        result["exito"] = false;
        result["mensaje"] = "Sesión inválida";
        return;
    }
    
    bool exito = false;
    if (accion == "activar") {
        exito = servidor->gestorRobot->activarEfectorFinal();
        result["mensaje"] = exito ? "Efector activado" : "Error activando efector";
    } else if (accion == "desactivar") {
        exito = servidor->gestorRobot->desactivarEfectorFinal();
        result["mensaje"] = exito ? "Efector desactivado" : "Error desactivando efector";
    } else {
        result["mensaje"] = "Acción inválida: usar 'activar' o 'desactivar'";
    }
    
    result["exito"] = exito;
    
    if (exito) {
        it->second.comandosEjecutados++;
    } else {
        it->second.comandosErroneos++;
    }
    
    servidor->registrarEvento("Efector " + accion, it->second.usuario, it->second.nodoOrigen);
}

std::string MetodoControlEfector::help() {
    return "Control del efector final. Parámetros: [sessionId, accion]";
}

// Implementación de MetodoAprenderTrayectoria
void MetodoAprenderTrayectoria::execute(XmlRpcValue& params, XmlRpcValue& result) {
    if (params.size() < 2) {
        result["exito"] = false;
        result["mensaje"] = "Parámetros insuficientes: [sessionId, accion, nombre]";
        return;
    }
    
    std::string sessionId = params[0];
    std::string accion = params[1]; // "iniciar", "agregar", "finalizar"
    
    auto it = servidor->sesionesActivas.find(sessionId);
    if (it == servidor->sesionesActivas.end()) {
        result["exito"] = false;
        result["mensaje"] = "Sesión inválida";
        return;
    }
    
    bool exito = false;
    
    if (accion == "iniciar") {
        if (params.size() < 3) {
            result["mensaje"] = "Falta nombre de trayectoria";
            return;
        }
        std::string nombre = params[2];
        exito = servidor->gestorRobot->iniciarAprendizajeTrayectoria(nombre);
        result["mensaje"] = exito ? "Aprendizaje iniciado" : "Error iniciando aprendizaje";
    } else if (accion == "agregar") {
        if (params.size() < 6) {
            result["mensaje"] = "Faltan coordenadas: [sessionId, agregar, x, y, z, velocidad]";
            return;
        }
        double x = params[2];
        double y = params[3];
        double z = params[4];
        double vel = params[5];
        exito = servidor->gestorRobot->agregarPasoTrayectoria(x, y, z, vel);
        result["mensaje"] = exito ? "Paso agregado" : "Error agregando paso";
    } else if (accion == "finalizar") {
        exito = servidor->gestorRobot->finalizarAprendizajeTrayectoria();
        result["mensaje"] = exito ? "Trayectoria guardada" : "Error guardando trayectoria";
    } else {
        result["mensaje"] = "Acción inválida: usar 'iniciar', 'agregar' o 'finalizar'";
    }
    
    result["exito"] = exito;
    servidor->registrarEvento("Aprendizaje " + accion, it->second.usuario, it->second.nodoOrigen);
}

std::string MetodoAprenderTrayectoria::help() {
    return "Aprender trayectoria paso a paso. Parámetros: [sessionId, accion, nombre/coordenadas]";
}

// Implementación de MetodoSubirGCode
void MetodoSubirGCode::execute(XmlRpcValue& params, XmlRpcValue& result) {
    if (params.size() < 3) {
        result["exito"] = false;
        result["mensaje"] = "Parámetros insuficientes: [sessionId, nombreArchivo, contenido]";
        return;
    }
    
    std::string sessionId = params[0];
    std::string nombreArchivo = params[1];
    std::string contenido = params[2];
    
    auto it = servidor->sesionesActivas.find(sessionId);
    if (it == servidor->sesionesActivas.end()) {
        result["exito"] = false;
        result["mensaje"] = "Sesión inválida";
        return;
    }
    
    // Crear archivo con el contenido
    std::string rutaCompleta = nombreArchivo + "_" + it->second.usuario + ".gcode";
    
    try {
        std::ofstream archivo(rutaCompleta);
        archivo << contenido;
        archivo.close();
        
        result["exito"] = true;
        result["mensaje"] = "Archivo subido correctamente";
        result["archivo"] = rutaCompleta;
        
        servidor->registrarEvento("Archivo subido: " + rutaCompleta, it->second.usuario, it->second.nodoOrigen);
        try {
            if (servidor->gestorReportes) servidor->gestorReportes->registrarPeticion(std::string("Archivo subido: ") + rutaCompleta, it->second.usuario, it->second.nodoOrigen, "200");
        } catch (const std::exception &e) {
            std::cerr << "Error registrarPeticion SubirGCode: " << e.what() << std::endl;
        }
    } catch (const std::exception& e) {
        result["exito"] = false;
        result["mensaje"] = "Error guardando archivo: " + std::string(e.what());
    }
}

std::string MetodoSubirGCode::help() {
    return "Subir archivo G-Code. Parámetros: [sessionId, nombreArchivo, contenido]";
}

// Implementación de MetodoEjecutarArchivo
void MetodoEjecutarArchivo::execute(XmlRpcValue& params, XmlRpcValue& result) {
    if (params.size() < 2) {
        result["exito"] = false;
        result["mensaje"] = "Parámetros insuficientes: [sessionId, nombreArchivo]";
        return;
    }
    
    std::string sessionId = params[0];
    std::string nombreArchivo = params[1];
    
    auto it = servidor->sesionesActivas.find(sessionId);
    if (it == servidor->sesionesActivas.end()) {
        result["exito"] = false;
        result["mensaje"] = "Sesión inválida";
        return;
    }
    
    // Verificar permisos: usuarios normales solo pueden ejecutar sus propios archivos
    if (!servidor->esAdministrador(sessionId)) {
        if (nombreArchivo.find("_" + it->second.usuario + ".gcode") == std::string::npos) {
            result["exito"] = false;
            result["mensaje"] = "Acceso denegado: Solo puede ejecutar sus propios archivos";
            return;
        }
    }
    
    bool exito = servidor->gestorRobot->cargarArchivoGCode(nombreArchivo);
    
    result["exito"] = exito;
    result["mensaje"] = exito ? "Archivo ejecutado correctamente" : "Error ejecutando archivo";
    
    if (exito) {
        it->second.comandosEjecutados++;
    } else {
        it->second.comandosErroneos++;
    }
    
    servidor->registrarEvento("Ejecutar archivo: " + nombreArchivo, it->second.usuario, it->second.nodoOrigen);
    try {
        if (servidor->gestorReportes) servidor->gestorReportes->registrarPeticion(std::string("Ejecutar archivo: ") + nombreArchivo, it->second.usuario, it->second.nodoOrigen, exito ? "200" : "ERROR");
    } catch (const std::exception &e) {
        std::cerr << "Error registrarPeticion EjecutarArchivo: " << e.what() << std::endl;
    }
}

std::string MetodoEjecutarArchivo::help() {
    return "Ejecutar archivo G-Code en modo automático. Parámetros: [sessionId, nombreArchivo]";
}

// Implementación de MetodoReporteLogCsv
void MetodoReporteLogCsv::execute(XmlRpcValue& params, XmlRpcValue& result) {
    if (params.size() < 1) {
        result["exito"] = false;
        result["mensaje"] = "Parámetros insuficientes: [sessionId, desde, hasta, filtroUsuario, filtroCodigo]";
        return;
    }
    
    std::string sessionId = params[0];
    if (!servidor->esAdministrador(sessionId)) {
        result["exito"] = false;
        result["mensaje"] = "Acceso denegado: Solo administradores";
        return;
    }
    
    // Parámetros opcionales
    std::string desde = (params.size() > 1) ? std::string(params[1]) : "1900-01-01 00:00:00";
    std::string hasta = (params.size() > 2) ? std::string(params[2]) : "2099-12-31 23:59:59";
    std::string filtroUsuario = (params.size() > 3) ? std::string(params[3]) : "";
    std::string filtroCodigo = (params.size() > 4) ? std::string(params[4]) : "";
    
    try {
        if (servidor->gestorReportes) {
            std::string logFiltrado = servidor->gestorReportes->reporteLog(desde, hasta, filtroUsuario, filtroCodigo);
            
            result["exito"] = true;
            result["logCsv"] = logFiltrado;
            result["filtros"]["desde"] = desde;
            result["filtros"]["hasta"] = hasta;
            result["filtros"]["usuario"] = filtroUsuario;
            result["filtros"]["codigo"] = filtroCodigo;
            
            // También proporcionar filtrado por texto libre
            if (params.size() > 5) {
                std::string filtro1 = std::string(params[5]);
                std::string filtro2 = (params.size() > 6) ? std::string(params[6]) : "";
                
                std::vector<std::string> lineasFiltradas = servidor->gestorReportes->filtrarLog(filtro1, filtro2);
                
                XmlRpcValue lineas;
                for (size_t i = 0; i < lineasFiltradas.size(); ++i) {
                    lineas[static_cast<int>(i)] = lineasFiltradas[i];
                }
                result["lineasFiltradas"] = lineas;
                result["filtros"]["texto1"] = filtro1;
                result["filtros"]["texto2"] = filtro2;
            }
        } else {
            result["exito"] = false;
            result["mensaje"] = "Gestor de reportes no disponible";
        }
    } catch (const std::exception& e) {
        result["exito"] = false;
        result["mensaje"] = std::string("Error obteniendo log: ") + e.what();
    }
}

std::string MetodoReporteLogCsv::help() {
    return "Obtener log CSV filtrado (solo admin). Parámetros: [sessionId, desde, hasta, filtroUsuario, filtroCodigo, filtroTexto1, filtroTexto2]";
}
