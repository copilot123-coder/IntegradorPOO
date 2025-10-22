#pragma once

#include <string>
#include <mutex>
#include <atomic>

// Clase EfectorFinal: controla el actuador final (ej: ventosa, pinza)
class EfectorFinal {
public:
    EfectorFinal();
    ~EfectorFinal();

    // Activa el efector para el tipo indicado (por ejemplo "ventosa", "pinza")
    // Devuelve true si se activó correctamente.
    bool Activar(const std::string& tipo);

    // Desactiva el efector. Devuelve true si se desactivó correctamente.
    bool Desactivar();

    // Devuelve el tipo actualmente activo o vacío si ninguno.
    std::string GetTipo() const;

private:
    mutable std::mutex mtx_;
    std::string tipo_;
    std::atomic<bool> activo_;
};
