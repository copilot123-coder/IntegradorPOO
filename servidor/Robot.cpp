#include "Robot.h"

#include <sstream>
#include <thread>
#include <chrono>
#include <cmath>

using namespace std::chrono_literals;

Robot::Robot()
: x_(0.0), y_(0.0), z_(0.0), velocidad_(1.0), connected_(false), motorsEnabled_(false), serial_(nullptr)
{}

Robot::~Robot() {
    // asegurar desconexión
    Desconectar();
}

bool Robot::Conectar() {
    bool expected = false;
    if (!connected_.compare_exchange_strong(expected, true)) {
        // ya conectado
        return true;
    }
    // simulamos pequeña latencia
    std::this_thread::sleep_for(50ms);
    return true;
}

void Robot::Desconectar() {
    bool was = connected_.exchange(false);
    if (was) {
        // simulamos apagado de motores
        motorsEnabled_.store(false);
        std::this_thread::sleep_for(20ms);
    }
}

bool Robot::IrAOrigen() {
    if (!connected_.load()) return false;
    if (!motorsEnabled_.load()) return false;

    std::lock_guard<std::mutex> lg(mtx_);
    // simulamos movimiento sencillo: tiempo proporcional a distancia
    double dist = std::sqrt(x_*x_ + y_*y_ + z_*z_);
    if (dist < 1e-6) return true; // ya en origen
    double t_ms = (dist / std::max(1e-6, velocidad_)) * 500.0; // factor de simulacion
    if (t_ms > 0) std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(t_ms)));
    x_ = y_ = z_ = 0.0;
    return true;
}

bool Robot::MoverXYZ(double x, double y, double z) {
    if (!connected_.load()) return false;
    if (!motorsEnabled_.load()) return false;

    std::lock_guard<std::mutex> lg(mtx_);
    double dx = x - x_;
    double dy = y - y_;
    double dz = z - z_;
    double dist = std::sqrt(dx*dx + dy*dy + dz*dz);
    if (dist < 1e-9) return true; // ya en la posicion
    double t_ms = (dist / std::max(1e-6, velocidad_)) * 500.0;
    if (t_ms > 0) std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(t_ms)));
    x_ = x; y_ = y; z_ = z;
    return true;
}

void Robot::SetVelocidad(double v) {
    if (v <= 0) return;
    std::lock_guard<std::mutex> lg(mtx_);
    velocidad_ = v;
}

std::string Robot::GetPosicion() const {
    std::lock_guard<std::mutex> lg(mtx_);
    return formatPos(x_, y_, z_);
}

bool Robot::ActivarMotores() {
    if (!connected_.load()) return false;
    motorsEnabled_.store(true);
    // pequeña latencia
    std::this_thread::sleep_for(10ms);
    return true;
}

bool Robot::DesactivarMotores() {
    if (!connected_.load()) return false;
    motorsEnabled_.store(false);
    std::this_thread::sleep_for(10ms);
    return true;
}

std::string Robot::formatPos(double x, double y, double z) {
    std::ostringstream oss;
    oss.setf(std::ios::fixed); oss.precision(3);
    oss << x << "," << y << "," << z;
    return oss.str();
}

void Robot::SetSerial(tp2::Serial* s) {
    serial_ = s;
}

tp2::Serial* Robot::GetSerial() const {
    return serial_;
}

Articulacion& Robot::GetArticulacion1() {
    return articulacion1_;
}

Articulacion& Robot::GetArticulacion2() {
    return articulacion2_;
}

Articulacion& Robot::GetArticulacion3() {
    return articulacion3_;
}

EfectorFinal& Robot::GetEfector() {
    return efector_;
}
