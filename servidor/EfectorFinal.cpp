#include "EfectorFinal.h"
#include <thread>
using namespace std::chrono_literals;

EfectorFinal::EfectorFinal()
: tipo_(), activo_(false) {}

EfectorFinal::~EfectorFinal() {
    Desactivar();
}

bool EfectorFinal::Activar(const std::string& tipo) {
    if (tipo.empty()) return false;
    {
        std::lock_guard<std::mutex> lg(mtx_);
        tipo_ = tipo;
    }
    // simulación de tiempo de activación
    std::this_thread::sleep_for(30ms);
    activo_.store(true);
    return true;
}

bool EfectorFinal::Desactivar() {
    if (!activo_.load()) return false;
    activo_.store(false);
    std::this_thread::sleep_for(10ms);
    {
        std::lock_guard<std::mutex> lg(mtx_);
        tipo_.clear();
    }
    return true;
}

std::string EfectorFinal::GetTipo() const {
    std::lock_guard<std::mutex> lg(mtx_);
    return tipo_;
}
