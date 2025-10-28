#pragma once

#include <string>
#include <mutex>
#include <atomic>

#include "Articulacion.h"
#include "EfectorFinal.h"

// forward declare Serial (es una asociación, no composición)
namespace tp2 { class Serial; }

// Clase Robot: interfaz básica para controlar un robot (simulado)
class Robot {
public:
    Robot();
    ~Robot();

    // Conecta al robot (simulado). Devuelve true si la conexión se estableció.
    bool Conectar();

    // Desconecta del robot (simulado).
    void Desconectar();

    // Lleva al robot al origen (0,0,0). Devuelve true si se completó.
    bool IrAOrigen();

    // Mueve el robot a la posición (x,y,z). Devuelve true si el movimiento se realizó.
    bool MoverXYZ(double x, double y, double z);

    // Ajusta la velocidad (unidad arbitraria). No return.
    void SetVelocidad(double v);

    // Obtiene la posición como string "x,y,z".
    std::string GetPosicion() const;

    // Activa/desactiva los motores
    bool ActivarMotores();
    bool DesactivarMotores();

    // Asociación con Serial (no propietario). Puede ser nullptr.
    void SetSerial(tp2::Serial* s);
    tp2::Serial* GetSerial() const;

    // Acceso a componentes por composición
    Articulacion& GetArticulacion1();
    Articulacion& GetArticulacion2();
    Articulacion& GetArticulacion3();
    EfectorFinal& GetEfector();

private:
    mutable std::mutex mtx_;
    double x_, y_, z_;
    double velocidad_;
    std::atomic<bool> connected_;
    std::atomic<bool> motorsEnabled_;

    // componentes (composición)
    Articulacion articulacion1_;
    Articulacion articulacion2_;
    Articulacion articulacion3_;
    EfectorFinal efector_;

    // asociación con Serial (no propietario)
    tp2::Serial* serial_;

    // Helper para formato de posición
    static std::string formatPos(double x, double y, double z);
};
