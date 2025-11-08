#include "GestorCodigoG.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <regex>
#include <cmath>
#include <thread>
#include <chrono>

GestorCodigoG::GestorCodigoG(const std::string& puertoSerial) 
    : serial_(std::make_unique<Serial>()),
      modoTrabajo_(ModoTrabajo::MANUAL),
      modoCoordenadas_(ModoCoordenas::ABSOLUTO),
      posicionActual_(0, 0, 0),
      posicionOrigen_(0, 0, 0),
      velocidadActual_(1000.0),
      efectorActivo_(false),
      robotConectado_(false) {
    
    // Inicializar comunicación serie
    if (!serial_->abrirPuerto()) {
        std::cerr << "Warning: No se pudo abrir puerto serie " << puertoSerial << std::endl;
    }
}

GestorCodigoG::~GestorCodigoG() {
    desconectarRobot();
}

bool GestorCodigoG::conectarRobot() {
    if (robotConectado_) {
        return true;
    }
    
    // Establecer conexión serie
    if (!serial_->abrirPuerto()) {
        std::cerr << "Error: No se pudo conectar al robot" << std::endl;
        return false;
    }
    
    std::cout << "Esperando inicialización del robot (5 segundos)..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    // Leer mensaje inicial del robot si está disponible
    std::string mensajeInicial = serial_->leerPuerto(2000);
    if (!mensajeInicial.empty()) {
        std::cout << "Mensaje inicial del robot: " << mensajeInicial << std::endl;
    }
    
    // Realizar homing primero (G28) - es crítico
    std::cout << "Realizando homing (G28)..." << std::endl;
    if (!enviarComandoConEspera("G28", 5000)) { // 4 segundos para homing
        std::cerr << "Error: No se pudo realizar homing" << std::endl;
        return false;
    }
    std::cout << "Homing completado." << std::endl;
    
    // Configurar modo absoluto
    std::cout << "Configurando modo absoluto..." << std::endl;
    if (!enviarComandoConEspera("G90", 1000)) { // 1 segundo para comandos normales
        std::cerr << "Error: No se pudo configurar modo absoluto" << std::endl;
        return false;
    }
    
    // Solicitar posición actual
    std::string respuesta = solicitarPosicionActual();
    if (!respuesta.empty()) {
        std::cout << "Robot conectado. Posición inicial: " << respuesta << std::endl;
    } else {
        std::cout << "Robot conectado (posición en origen después de homing)" << std::endl;
    }
    
    robotConectado_ = true;
    return true;
}

void GestorCodigoG::desconectarRobot() {
    if (robotConectado_) {
        // Desactivar efector y motores antes de desconectar
        desactivarEfectorFinal();
        enviarComandoASerial("M84"); // Desactivar motores
        serial_->cerrarPuerto();
        robotConectado_ = false;
        std::cout << "Robot desconectado" << std::endl;
    }
}

bool GestorCodigoG::configurarModoTrabajo(ModoTrabajo modo) {
    modoTrabajo_ = modo;
    std::cout << "Modo de trabajo cambiado a: " 
              << (modo == ModoTrabajo::MANUAL ? "MANUAL" : "AUTOMÁTICO") << std::endl;
    return true;
}

bool GestorCodigoG::configurarModoCoordenadas(ModoCoordenas modo) {
    if (!robotConectado_) {
        std::cerr << "Error: Robot no conectado" << std::endl;
        return false;
    }
    
    std::string comando = (modo == ModoCoordenas::ABSOLUTO) ? "G90" : "G91";
    if (enviarComandoConEspera(comando, 1000)) { // 1 segundo para cambio de modo
        modoCoordenadas_ = modo;
        std::cout << "Modo de coordenadas cambiado a: " 
                  << (modo == ModoCoordenas::ABSOLUTO ? "ABSOLUTO" : "RELATIVO") << std::endl;
        return true;
    }
    
    std::cerr << "Error: No se pudo cambiar modo de coordenadas" << std::endl;
    return false;
}

bool GestorCodigoG::validarPosicion(const Posicion& pos) const {
    // Calcular módulo cuadrado de la posición (más eficiente que sqrt)
    double squaredPositionModule = sq(pos.x) + sq(pos.y);
    
    // Validar límites usando módulo cuadrado y coordenada Z - similar al firmware
    bool posicionValida = squaredPositionModule <= sq(R_MAX) 
                      && squaredPositionModule >= sq(R_MIN) 
                      && pos.z >= Z_MIN  
                      && pos.z <= Z_MAX;
    
    if (!posicionValida) {
        double r = std::sqrt(squaredPositionModule);
        if (squaredPositionModule < sq(R_MIN) || squaredPositionModule > sq(R_MAX)) {
            std::cerr << "Error: Posición radial fuera de límites [" << R_MIN << ", " << R_MAX << "] - actual: " << r << std::endl;
        }
        if (pos.z < Z_MIN || pos.z > Z_MAX) {
            std::cerr << "Error: Posición Z fuera de límites [" << Z_MIN << ", " << Z_MAX << "] - actual: " << pos.z << std::endl;
        }
        return false;
    }
    
    return true;
}

double GestorCodigoG::calcularDistanciaRadial(double x, double y) const {
    return std::sqrt(x * x + y * y);
}

bool GestorCodigoG::validarComandoG(const std::string& comando) const {
    // Validación básica de formato G-Code
    std::regex gcode_pattern(R"(^[GM]\d+(\s+[XYZFES][-+]?\d*\.?\d*)*\s*$)");
    return std::regex_match(comando, gcode_pattern);
}

ComandoG GestorCodigoG::parsearComandoG(const std::string& comando) const {
    ComandoG cmd;
    cmd.comando = comando;
    cmd.valido = validarComandoG(comando);
    
    if (!cmd.valido) {
        return cmd;
    }
    
    // Extraer coordenadas usando regex
    std::regex x_pattern(R"(X\s*([-+]?\d*\.?\d+))");
    std::regex y_pattern(R"(Y\s*([-+]?\d*\.?\d+))");
    std::regex z_pattern(R"(Z\s*([-+]?\d*\.?\d+))");
    std::regex f_pattern(R"(F\s*([-+]?\d*\.?\d+))");
    
    std::smatch match;
    
    if (std::regex_search(comando, match, x_pattern)) {
        cmd.posicion.x = std::stod(match[1]);
    }
    if (std::regex_search(comando, match, y_pattern)) {
        cmd.posicion.y = std::stod(match[1]);
    }
    if (std::regex_search(comando, match, z_pattern)) {
        cmd.posicion.z = std::stod(match[1]);
    }
    if (std::regex_search(comando, match, f_pattern)) {
        cmd.velocidad = std::stod(match[1]);
    }
    
    // Determinar descripción del comando
    if (comando.find("G28") != std::string::npos) {
        cmd.descripcion = "Home - Ir a origen";
    } else if (comando.find("G1") != std::string::npos || comando.find("G0") != std::string::npos) {
        cmd.descripcion = "Movimiento lineal";
    } else if (comando.find("M3") != std::string::npos) {
        cmd.descripcion = "Activar efector";
    } else if (comando.find("M5") != std::string::npos) {
        cmd.descripcion = "Desactivar efector";
    } else {
        cmd.descripcion = "Comando G-Code";
    }
    
    return cmd;
}

std::string GestorCodigoG::posicionAComandoG(const Posicion& pos, double velocidad) const {
    std::ostringstream cmd;
    cmd << "G1 X" << pos.x << " Y" << pos.y << " Z" << pos.z;
    if (velocidad > 0) {
        cmd << " F" << velocidad;
    }
    return cmd.str();
}

Posicion GestorCodigoG::coordenadasXYZAComandoG(double x, double y, double z) const {
    Posicion pos(x, y, z);
    
    // Si estamos en modo relativo, ajustar respecto a posición actual
    if (modoCoordenadas_ == ModoCoordenas::RELATIVO) {
        pos.x += posicionActual_.x;
        pos.y += posicionActual_.y;
        pos.z += posicionActual_.z;
    }
    
    return pos;
}

bool GestorCodigoG::enviarComandoASerial(const std::string& comando) {
    if (!robotConectado_) {
        std::cerr << "Error: Robot no conectado" << std::endl;
        return false;
    }
    
    if (!serial_->enviarComando(comando)) {
        std::cerr << "Error enviando comando: " << comando << std::endl;
        return false;
    }
    
    // Esperar respuesta del robot
    std::string respuesta = serial_->leerPuerto(2000);
    if (respuesta.empty()) {
        std::cout << "Comando enviado sin respuesta: " << comando << std::endl;
        return true; // Asumir éxito si no hay respuesta
    }
    
    std::cout << "Respuesta: " << respuesta << std::endl;
    return true;
}

bool GestorCodigoG::enviarComandoConEspera(const std::string& comando, int tiempoEsperaMs) {
    if (!serial_->enviarComando(comando)) {
        std::cerr << "Error enviando comando: " << comando << std::endl;
        return false;
    }
    
    std::cout << "Enviado: " << comando << " (esperando " << tiempoEsperaMs << "ms)" << std::endl;
    
    // Esperar respuesta del robot con tiempo personalizado
    std::string respuesta = serial_->leerPuerto(tiempoEsperaMs);
    if (respuesta.empty()) {
        std::cout << "Comando enviado sin respuesta: " << comando << std::endl;
        return true; // Asumir éxito para comandos que no requieren respuesta
    }
    
    std::cout << "Respuesta: " << respuesta << std::endl;
    
    // Verificar si hay errores en la respuesta
    if (respuesta.find("error") != std::string::npos || 
        respuesta.find("Error") != std::string::npos ||
        respuesta.find("ERROR") != std::string::npos) {
        std::cerr << "Error en respuesta del robot: " << respuesta << std::endl;
        return false;
    }
    
    return true;
}

std::string GestorCodigoG::solicitarPosicionActual() {
    if (!robotConectado_) {
        return "";
    }
    
    if (!serial_->enviarComando("M114")) {
        return "";
    }
    
    return serial_->leerPuerto(2000);
}

std::string GestorCodigoG::solicitarEstadoRobot() {
    if (!robotConectado_) {
        return "Robot desconectado";
    }
    
    if (!serial_->enviarComando("M115")) {
        return "Error consultando estado";
    }
    
    std::string respuesta = serial_->leerPuerto(2000);
    return respuesta.empty() ? "Sin respuesta del robot" : respuesta;
}

bool GestorCodigoG::irAPosicionOrigen() {
    if (modoTrabajo_ != ModoTrabajo::MANUAL) {
        std::cerr << "Error: Función solo disponible en modo manual" << std::endl;
        return false;
    }
    
    if (!robotConectado_) {
        std::cerr << "Error: Robot no conectado" << std::endl;
        return false;
    }
    
    std::cout << "Enviando robot a posición de origen..." << std::endl;
    if (enviarComandoConEspera("G28", 4000)) { // 4 segundos para homing
        posicionActual_ = posicionOrigen_;
        return true;
    }
    
    return false;
}

bool GestorCodigoG::moverEfectorConVelocidad(double x, double y, double z, double velocidad) {
    if (modoTrabajo_ != ModoTrabajo::MANUAL) {
        std::cerr << "Error: Función solo disponible en modo manual" << std::endl;
        return false;
    }
    
    Posicion nuevaPos = coordenadasXYZAComandoG(x, y, z);
    
    if (!validarPosicion(nuevaPos)) {
        return false;
    }
    
    // Asegurar modo absoluto para movimientos XYZ
    if (modoCoordenadas_ != ModoCoordenas::ABSOLUTO) {
        configurarModoCoordenadas(ModoCoordenas::ABSOLUTO);
    }
    
    std::string comando = posicionAComandoG(nuevaPos, velocidad);
    if (enviarComandoConEspera(comando, 1000)) { // 1 segundo para movimientos
        posicionActual_ = nuevaPos;
        velocidadActual_ = velocidad;
        return true;
    }
    
    return false;
}

bool GestorCodigoG::moverEfectorSinVelocidad(double x, double y, double z) {
    return moverEfectorConVelocidad(x, y, z, velocidadActual_);
}

bool GestorCodigoG::activarEfectorFinal() {
    if (!robotConectado_) {
        std::cerr << "Error: Robot no conectado" << std::endl;
        return false;
    }
    
    if (enviarComandoConEspera("M3", 1000)) { // 1 segundo para activar efector
        efectorActivo_ = true;
        std::cout << "Efector final activado" << std::endl;
        return true;
    }
    
    return false;
}

bool GestorCodigoG::desactivarEfectorFinal() {
    if (!robotConectado_) {
        return true; // Ya está desactivado
    }
    
    if (enviarComandoConEspera("M5", 1000)) { // 1 segundo para desactivar efector
        efectorActivo_ = false;
        std::cout << "Efector final desactivado" << std::endl;
        return true;
    }
    
    return false;
}

bool GestorCodigoG::iniciarAprendizajeTrayectoria(const std::string& nombreTrayectoria) {
    if (modoTrabajo_ != ModoTrabajo::MANUAL) {
        std::cerr << "Error: Aprendizaje solo disponible en modo manual" << std::endl;
        return false;
    }
    
    trayectoriaAprendida_.clear();
    nombreTrayectoriaActual_ = nombreTrayectoria;
    std::cout << "Iniciando aprendizaje de trayectoria: " << nombreTrayectoria << std::endl;
    return true;
}

bool GestorCodigoG::agregarPasoTrayectoria(double x, double y, double z, double velocidad) {
    if (nombreTrayectoriaActual_.empty()) {
        std::cerr << "Error: No se ha iniciado el aprendizaje de trayectoria" << std::endl;
        return false;
    }
    
    Posicion nuevaPos = coordenadasXYZAComandoG(x, y, z);
    
    if (!validarPosicion(nuevaPos)) {
        return false;
    }
    
    ComandoG cmd;
    cmd.comando = posicionAComandoG(nuevaPos, velocidad > 0 ? velocidad : velocidadActual_);
    cmd.descripcion = "Movimiento aprendido";
    cmd.posicion = nuevaPos;
    cmd.velocidad = velocidad > 0 ? velocidad : velocidadActual_;
    cmd.valido = true;
    
    trayectoriaAprendida_.push_back(cmd);
    std::cout << "Paso agregado a trayectoria: " << cmd.comando << std::endl;
    
    // Ejecutar el movimiento inmediatamente en modo aprendizaje
    return moverEfectorConVelocidad(x, y, z, cmd.velocidad);
}

bool GestorCodigoG::agregarComandoGTrayectoria(const std::string& comandoG) {
    if (nombreTrayectoriaActual_.empty()) {
        std::cerr << "Error: No se ha iniciado el aprendizaje de trayectoria" << std::endl;
        return false;
    }
    
    ComandoG cmd = parsearComandoG(comandoG);
    if (!cmd.valido) {
        std::cerr << "Error: Comando G-Code inválido: " << comandoG << std::endl;
        return false;
    }
    
    trayectoriaAprendida_.push_back(cmd);
    std::cout << "Comando G agregado a trayectoria: " << comandoG << std::endl;
    
    // Ejecutar el comando inmediatamente en modo aprendizaje
    return ejecutarComandoGDirecto(comandoG);
}

bool GestorCodigoG::finalizarAprendizajeTrayectoria() {
    if (nombreTrayectoriaActual_.empty()) {
        std::cerr << "Error: No hay trayectoria en progreso" << std::endl;
        return false;
    }
    
    if (trayectoriaAprendida_.empty()) {
        std::cerr << "Error: La trayectoria está vacía" << std::endl;
        return false;
    }
    
    if (guardarTrayectoria(nombreTrayectoriaActual_)) {
        std::cout << "Trayectoria '" << nombreTrayectoriaActual_ 
                  << "' guardada con " << trayectoriaAprendida_.size() << " pasos" << std::endl;
        nombreTrayectoriaActual_.clear();
        return true;
    }
    
    return false;
}

bool GestorCodigoG::cancelarAprendizajeTrayectoria() {
    trayectoriaAprendida_.clear();
    nombreTrayectoriaActual_.clear();
    std::cout << "Aprendizaje de trayectoria cancelado" << std::endl;
    return true;
}

bool GestorCodigoG::guardarTrayectoria(const std::string& nombreArchivo) {
    try {
        GestorArchivos gestor(nombreArchivo + ".gcode");
        gestor.open("w");
        
        // Escribir header
        gestor.write("; Trayectoria generada por GestorCodigoG\n");
        gestor.write("; Nombre: " + nombreArchivo + "\n");
        gestor.write("G90 ; Modo absoluto\n");
        gestor.write("G28 ; Home\n");
        
        // Escribir comandos de la trayectoria
        for (const auto& cmd : trayectoriaAprendida_) {
            gestor.write(cmd.comando + " ; " + cmd.descripcion + "\n");
        }
        
        gestor.write("M5 ; Desactivar efector\n");
        gestor.write("G28 ; Retornar a home\n");
        gestor.close();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error guardando trayectoria: " << e.what() << std::endl;
        return false;
    }
}

bool GestorCodigoG::cargarArchivoGCode(const std::string& nombreArchivo) {
    try {
        GestorArchivos gestor(nombreArchivo);
        if (!gestor.exist()) {
            std::cerr << "Error: Archivo no encontrado: " << nombreArchivo << std::endl;
            return false;
        }
        
        gestor.open("r");
        trayectoriaAprendida_.clear();
        
        std::string linea;
        while (!(linea = gestor.getLine()).empty()) {
            // Saltar comentarios y líneas vacías
            if (linea.empty() || linea[0] == ';') {
                continue;
            }
            
            ComandoG cmd = parsearComandoG(linea);
            if (cmd.valido) {
                trayectoriaAprendida_.push_back(cmd);
            }
        }
        
        gestor.close();
        std::cout << "Archivo G-Code cargado: " << nombreArchivo 
                  << " (" << trayectoriaAprendida_.size() << " comandos)" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error cargando archivo: " << e.what() << std::endl;
        return false;
    }
}

bool GestorCodigoG::ejecutarComandoGDirecto(const std::string& comandoG) {
    if (!robotConectado_) {
        std::cerr << "Error: Robot no conectado" << std::endl;
        return false;
    }
    
    ComandoG cmd = parsearComandoG(comandoG);
    if (!cmd.valido) {
        std::cerr << "Error: Comando G-Code inválido: " << comandoG << std::endl;
        return false;
    }
    
    // Validar posición si es comando de movimiento
    if (comandoG.find("G1") != std::string::npos || comandoG.find("G0") != std::string::npos) {
        Posicion nuevaPos = cmd.posicion;
        if (modoCoordenadas_ == ModoCoordenas::RELATIVO) {
            nuevaPos.x += posicionActual_.x;
            nuevaPos.y += posicionActual_.y;
            nuevaPos.z += posicionActual_.z;
        }
        
        if (!validarPosicion(nuevaPos)) {
            return false;
        }
        
        if (enviarComandoASerial(comandoG)) {
            posicionActual_ = nuevaPos;
            return true;
        }
    } else {
        // Otros comandos (M, G28, etc.)
        return enviarComandoASerial(comandoG);
    }
    
    return false;
}

Posicion GestorCodigoG::obtenerPosicionActual() {
    std::string respuesta = solicitarPosicionActual();
    // Aquí se podría parsear la respuesta para actualizar posicionActual_
    // Por ahora devolvemos la posición almacenada localmente
    return posicionActual_;
}

std::string GestorCodigoG::obtenerEstadoRobot() const {
    std::ostringstream estado;
    estado << "Estado del Robot:\n";
    estado << "- Conectado: " << (robotConectado_ ? "Sí" : "No") << "\n";
    estado << "- Modo trabajo: " << (modoTrabajo_ == ModoTrabajo::MANUAL ? "Manual" : "Automático") << "\n";
    estado << "- Modo coordenadas: " << (modoCoordenadas_ == ModoCoordenas::ABSOLUTO ? "Absoluto" : "Relativo") << "\n";
    estado << "- Posición actual: X" << posicionActual_.x << " Y" << posicionActual_.y << " Z" << posicionActual_.z << "\n";
    estado << "- Velocidad actual: " << velocidadActual_ << "\n";
    estado << "- Efector activo: " << (efectorActivo_ ? "Sí" : "No") << "\n";
    
    if (!nombreTrayectoriaActual_.empty()) {
        estado << "- Aprendiendo trayectoria: " << nombreTrayectoriaActual_ << " (" << trayectoriaAprendida_.size() << " pasos)\n";
    }
    
    return estado.str();
}

std::string GestorCodigoG::obtenerEspacioTrabajoInfo() const {
    std::ostringstream info;
    info << "Espacio de Trabajo del Robot:\n";
    info << "- Longitud brazo inferior: " << LOW_SHANK_LENGTH << " mm\n";
    info << "- Longitud brazo superior: " << HIGH_SHANK_LENGTH << " mm\n";
    info << "- Límites Z: [" << Z_MIN << ", " << Z_MAX << "] mm\n";
    info << "- Límites radiales: [" << R_MIN << ", " << R_MAX << "] mm\n";
    return info.str();
}

void GestorCodigoG::limpiarTrayectoriaActual() {
    trayectoriaAprendida_.clear();
    nombreTrayectoriaActual_.clear();
    std::cout << "Trayectoria actual limpiada" << std::endl;
}
