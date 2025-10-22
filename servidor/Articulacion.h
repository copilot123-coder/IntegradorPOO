#pragma once

#include <mutex>

// Clase Articulacion: controla una articulación simple (ángulo en grados)
class Articulacion {
public:
    Articulacion();
    ~Articulacion();

    // Mueve la articulación al ángulo dado (grados) con la velocidad dada (grados/s).
    // Devuelve true si el movimiento se completó (simulado).
    bool MoverA(double angulo, double velocidad);

    // Ajusta la velocidad máxima permitida.
    void SetVelocidadMax(double v);

    // Devuelve el ángulo actual en grados.
    double GetAngulo() const;

private:
    mutable std::mutex mtx_;
    double angulo_;       // grados
    double velMax_;       // grados por segundo
};
