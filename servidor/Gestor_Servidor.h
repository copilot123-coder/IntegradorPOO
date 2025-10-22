#pragma once

#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>

class Gestor_Servidor {
public:
    using Sender = std::function<bool(const std::string&)>;

    Gestor_Servidor();
    explicit Gestor_Servidor(Sender s);
    ~Gestor_Servidor();

    // Carga un archivo GCode (devuelve true si se cargó correctamente)
    bool CargarArchivoGCode(const std::string& path);

    // Envía una línea GCode usando el sender configurado (no altera el índice interno)
    bool EnviarLinea(const std::string& gcode);

    // Devuelve todas las líneas cargadas como un único string separado por '\n'
    std::string ListaComandos() const;

    // Habilita/deshabilita modo manual. Si está en modo manual, ModoAutomatico se detiene.
    void ModoManual(bool habilitar);

    // Ejecuta modo automático: envía las líneas restantes hasta terminar o hasta que
    // se ingrese modo manual. Devuelve true si llegó al final, false si se detuvo.
    bool ModoAutomatico();

    // Reinicia el cursor interno al inicio del archivo cargado
    void Reset();

    // Cantidad de líneas cargadas / índice actual
    std::size_t LineasTotales() const;
    std::size_t LineaActual() const;

    // Reemplaza el callback de envío (por defecto imprime en stdout y devuelve true)
    void SetSender(Sender s);

private:
    // helper para parsear y limpiar líneas GCode
    static std::string trim(const std::string& s);

    std::vector<std::string> lines_;
    std::string path_;
    std::size_t idx_;            // próxima línea a enviar en modo automático
    std::atomic<bool> manualMode_;
    Sender sender_;
    mutable std::mutex mtx_;
};