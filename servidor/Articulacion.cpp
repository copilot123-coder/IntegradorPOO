#include "Articulacion.h"
#include <cmath>
#include <thread>

using namespace std::chrono_literals;

Articulacion::Articulacion()
: angulo_(0.0), velMax_(90.0) {}

Articulacion::~Articulacion() = default;

bool Articulacion::MoverA(double angulo, double velocidad) {
    if (velocidad <= 0) return false;
    double useVel = std::min(velocidad, velMax_);
    double start;
    {
        std::lock_guard<std::mutex> lg(mtx_);
        start = angulo_;
    }
    double diff = std::fabs(angulo - start);
    if (diff < 1e-6) return true;
    double t_ms = (diff / useVel) * 1000.0; // ms
    if (t_ms > 0) std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(t_ms)));
    {
        std::lock_guard<std::mutex> lg(mtx_);
        angulo_ = angulo;
    }
    return true;
}

void Articulacion::SetVelocidadMax(double v) {
    if (v <= 0) return;
    std::lock_guard<std::mutex> lg(mtx_);
    velMax_ = v;
}

double Articulacion::GetAngulo() const {
    std::lock_guard<std::mutex> lg(mtx_);
    return angulo_;
}
