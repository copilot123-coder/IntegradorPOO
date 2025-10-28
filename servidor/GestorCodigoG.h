#ifndef GESTORCODIGOG_H
#define GESTORCODIGOG_H

#include <string>
#include <vector>
#include <memory>
#include <map>
#include "Serial.h"
#include "GestorArchivos.h"

// Definiciones del espacio de trabajo del robot
#define LOW_SHANK_LENGTH 120.0
#define HIGH_SHANK_LENGTH 120.0
#define Z_MIN -115 // -140.0 //MINIMUM Z HEIGHT OF TOOLHEAD TOUCHING GROUND
#define Z_MAX (LOW_SHANK_LENGTH+30.0) //SHANK_LENGTH ADDING ARBITUARY NUMBER FOR Z_MAX
#define SHANKS_MIN_ANGLE_COS 0.791436948 
#define SHANKS_MAX_ANGLE_COS -0.774944489 
#define R_MIN (sqrt((sq(LOW_SHANK_LENGTH) + sq(HIGH_SHANK_LENGTH)) - (2*LOW_SHANK_LENGTH*HIGH_SHANK_LENGTH*SHANKS_MIN_ANGLE_COS) ))
#define R_MAX (sqrt((sq(LOW_SHANK_LENGTH) + sq(HIGH_SHANK_LENGTH)) - (2*LOW_SHANK_LENGTH*HIGH_SHANK_LENGTH*SHANKS_MAX_ANGLE_COS) ))
#define sq(x) ((x)*(x))

enum class ModoTrabajo {
    MANUAL,
    AUTOMATICO
};

enum class ModoCoordenas {
    ABSOLUTO,
    RELATIVO
};

struct Posicion {
    double x;
    double y;
    double z;
    
    Posicion(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}
};

struct ComandoG {
    std::string comando;
    std::string descripcion;
    Posicion posicion;
    double velocidad;
    bool valido;
    
    ComandoG() : velocidad(0), valido(false) {}
};

class GestorCodigoG {
private:
    std::unique_ptr<Serial> serial_;
    
    ModoTrabajo modoTrabajo_;
    ModoCoordenas modoCoordenadas_;
    
    Posicion posicionActual_;
    Posicion posicionOrigen_;
    double velocidadActual_;
    bool efectorActivo_;
    bool robotConectado_;
    
    std::vector<ComandoG> trayectoriaAprendida_;
    std::string nombreTrayectoriaActual_;
    
    // Métodos de validación
    bool validarPosicion(const Posicion& pos) const;
    bool validarComandoG(const std::string& comando) const;
    double calcularDistanciaRadial(double x, double y) const;
    
    // Métodos de conversión
    ComandoG parsearComandoG(const std::string& comando) const;
    std::string posicionAComandoG(const Posicion& pos, double velocidad = 0) const;
    Posicion coordenadasXYZAComandoG(double x, double y, double z) const;
    
    // Métodos de comunicación con robot
    bool enviarComandoASerial(const std::string& comando);
    bool enviarComandoConEspera(const std::string& comando, int tiempoEsperaMs);
    std::string solicitarPosicionActual();
    std::string solicitarEstadoRobot();

public:
    explicit GestorCodigoG(const std::string& puertoSerial = "/dev/ttyUSB0");
    ~GestorCodigoG();
    
    // Conexión y configuración inicial
    bool conectarRobot();
    void desconectarRobot();
    bool configurarModoTrabajo(ModoTrabajo modo);
    bool configurarModoCoordenadas(ModoCoordenas modo);
    
    // Gestión de archivos G-Code
    bool subirArchivoGCode(const std::string& rutaArchivo, const std::string& nombreDestino);
    std::vector<std::string> obtenerListaArchivos(const std::string& usuario = "") const;
    bool eliminarArchivoGCode(const std::string& nombreArchivo);
    bool cargarArchivoGCode(const std::string& nombreArchivo); // Método público para cargar archivos
    bool guardarTrayectoria(const std::string& nombreArchivo);
    std::vector<std::string> listarArchivosGCode(const std::string& usuario = "") const;
    
    // Modo Manual - Movimientos básicos
    bool irAPosicionOrigen();
    bool moverEfectorConVelocidad(double x, double y, double z, double velocidad);
    bool moverEfectorSinVelocidad(double x, double y, double z);
    bool activarEfectorFinal();
    bool desactivarEfectorFinal();
    
    // Modo Manual - Aprendizaje de trayectoria
    bool iniciarAprendizajeTrayectoria(const std::string& nombreTrayectoria);
    bool agregarPasoTrayectoria(double x, double y, double z, double velocidad = 0);
    bool agregarComandoGTrayectoria(const std::string& comandoG);
    bool finalizarAprendizajeTrayectoria();
    bool cancelarAprendizajeTrayectoria();
    
    // Modo Automático - Ejecución de secuencias
    bool cargarSecuenciaTrabajo(const std::string& nombreArchivo, const std::string& usuario = "");
    bool ejecutarSecuenciaCompleta();
    bool ejecutarPasoAPaso();
    bool pausarEjecucion();
    bool reanudarEjecucion();
    bool detenerEjecucion();
    
    // Consultas de estado
    Posicion obtenerPosicionActual();
    std::string obtenerEstadoRobot() const;
    ModoTrabajo obtenerModoTrabajo() const { return modoTrabajo_; }
    ModoCoordenas obtenerModoCoordenadas() const { return modoCoordenadas_; }
    bool estaConectado() const { return robotConectado_; }
    bool estaEfectorActivo() const { return efectorActivo_; }
    
    // Comandos directos G-Code (opcional)
    bool ejecutarComandoGDirecto(const std::string& comandoG);
    bool validarComandoGAntesDEjecutar(const std::string& comandoG) const;
    
    // Utilidades
    std::string obtenerEspacioTrabajoInfo() const;
    std::vector<ComandoG> obtenerTrayectoriaActual() const { return trayectoriaAprendida_; }
    void limpiarTrayectoriaActual();
};

#endif
