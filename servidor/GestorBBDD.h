#pragma once

#include <string>
#include <vector>
#include <memory>
#include "Usuario.h"

// Forward declaration para SQLite
struct sqlite3;

class GestorBBDD {
public:
    GestorBBDD(const std::string& dbPath = "usuarios.db");
    ~GestorBBDD();

    // Inicializa la base de datos y crea las tablas si no existen
    bool inicializar();

    // Gestión de usuarios
    bool agregarUsuario(const Usuario& usuario);
    bool eliminarUsuario(int id);
    std::unique_ptr<Usuario> obtenerUsuario(int id);
    std::unique_ptr<Usuario> obtenerUsuarioPorNombre(const std::string& nombre);
    std::vector<Usuario> obtenerTodosUsuarios();

    // Validación especial para admin
    bool existeAdmin();
    bool cambiarTipoUsuario(int id, const std::string& nuevoTipo);

    // Cerrar conexión
    void cerrar();

private:
    sqlite3* db_;
    std::string dbPath_;
    bool conectado_;

    // Métodos auxiliares
    bool crearTablas();
    bool ejecutarSQL(const std::string& sql);
    std::string escaparComillas(const std::string& str);
};
