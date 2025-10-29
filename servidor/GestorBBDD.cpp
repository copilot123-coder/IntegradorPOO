#include "GestorBBDD.h"
#include "Usuario.h"
#include <sqlite3.h>
#include <iostream>

GestorBBDD::GestorBBDD(const std::string& dbPath) : db_(nullptr), dbPath_(dbPath), conectado_(false) {
}

GestorBBDD::~GestorBBDD() {
    cerrar();
}

bool GestorBBDD::inicializar() {
    int rc = sqlite3_open(dbPath_.c_str(), &db_);
    if (rc) {
        std::cerr << "Error al abrir BD: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    conectado_ = true;
    
    if (!crearTablas()) {
        return false;
    }
    
    // Crear usuarios por defecto si no existen
    return crearUsuariosPorDefecto();
}

bool GestorBBDD::crearTablas() {
    std::string sql = 
        "CREATE TABLE IF NOT EXISTS usuarios ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "nombre TEXT NOT NULL UNIQUE,"
        "clave TEXT NOT NULL,"
        "tipo TEXT NOT NULL);";
    
    return ejecutarSQL(sql);
}

bool GestorBBDD::ejecutarSQL(const std::string& sql) {
    char* errMsg = 0;
    int rc = sqlite3_exec(db_, sql.c_str(), 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Error SQL: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool GestorBBDD::agregarUsuario(const Usuario& usuario) {
    sqlite3_stmt* stmt;
    std::string sql = "INSERT INTO usuarios (nombre, clave, tipo) VALUES (?, ?, ?);";
    
    std::cout << "Insertando usuario - Nombre: '" << usuario.getNombre() 
              << "', Clave: '" << usuario.getClave() << "', Tipo: '" << usuario.getTipo() << "'" << std::endl;
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, 0) != SQLITE_OK) {
        std::cerr << "Error preparando SQL: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    // Usar SQLITE_TRANSIENT para copiar las strings en lugar de SQLITE_STATIC
    sqlite3_bind_text(stmt, 1, usuario.getNombre().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, usuario.getClave().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, usuario.getTipo().c_str(), -1, SQLITE_TRANSIENT);
    
    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (result == SQLITE_DONE) {
        std::cout << "Usuario insertado exitosamente." << std::endl;
        return true;
    } else {
        std::cerr << "Error insertando usuario: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
}

bool GestorBBDD::eliminarUsuario(int id) {
    std::string sql = "DELETE FROM usuarios WHERE id = " + std::to_string(id) + ";";
    return ejecutarSQL(sql);
}

std::unique_ptr<Usuario> GestorBBDD::obtenerUsuario(int id) {
    sqlite3_stmt* stmt;
    std::string sql = "SELECT id, nombre, clave, tipo FROM usuarios WHERE id = ?;";
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, 0) != SQLITE_OK) {
        return nullptr;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        auto usuario = std::make_unique<Usuario>(
            sqlite3_column_int(stmt, 0),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3))
        );
        sqlite3_finalize(stmt);
        return usuario;
    }
    
    sqlite3_finalize(stmt);
    return nullptr;
}

std::unique_ptr<Usuario> GestorBBDD::obtenerUsuarioPorNombre(const std::string& nombre) {
    sqlite3_stmt* stmt;
    std::string sql = "SELECT id, nombre, clave, tipo FROM usuarios WHERE nombre = ?;";
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, 0) != SQLITE_OK) {
        return nullptr;
    }
    
    sqlite3_bind_text(stmt, 1, nombre.c_str(), -1, SQLITE_STATIC);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        auto usuario = std::make_unique<Usuario>(
            sqlite3_column_int(stmt, 0),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3))
        );
        sqlite3_finalize(stmt);
        return usuario;
    }
    
    sqlite3_finalize(stmt);
    return nullptr;
}

std::vector<Usuario> GestorBBDD::obtenerTodosUsuarios() {
    std::vector<Usuario> usuarios;
    sqlite3_stmt* stmt;
    std::string sql = "SELECT id, nombre, clave, tipo FROM usuarios;";
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            usuarios.emplace_back(
                sqlite3_column_int(stmt, 0),
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)),
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3))
            );
        }
        sqlite3_finalize(stmt);
    }
    
    return usuarios;
}

bool GestorBBDD::existeAdmin() {
    sqlite3_stmt* stmt;
    std::string sql = "SELECT COUNT(*) FROM usuarios WHERE tipo = 'admin';";
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int count = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);
            return count > 0;
        }
        sqlite3_finalize(stmt);
    }
    
    return false;
}

bool GestorBBDD::cambiarTipoUsuario(int id, const std::string& nuevoTipo) {
    // Solo permitir un admin
    if (nuevoTipo == "admin" && existeAdmin()) {
        return false;
    }
    
    std::string sql = "UPDATE usuarios SET tipo = '" + escaparComillas(nuevoTipo) + 
                     "' WHERE id = " + std::to_string(id) + ";";
    return ejecutarSQL(sql);
}

void GestorBBDD::cerrar() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
        conectado_ = false;
    }
}

std::string GestorBBDD::escaparComillas(const std::string& str) {
    std::string resultado = str;
    size_t pos = 0;
    while ((pos = resultado.find("'", pos)) != std::string::npos) {
        resultado.replace(pos, 1, "''");
        pos += 2;
    }
    return resultado;
}

bool GestorBBDD::crearUsuariosPorDefecto() {
    // Verificar si ya existen usuarios
    std::vector<Usuario> usuarios = obtenerTodosUsuarios();
    if (!usuarios.empty()) {
        std::cout << "Usuarios ya existen en la base de datos:" << std::endl;
        for (const auto& u : usuarios) {
            std::cout << "  ID: " << u.getId() << ", Nombre: '" << u.getNombre() 
                      << "', Clave: '" << u.getClave() << "', Tipo: '" << u.getTipo() << "'" << std::endl;
        }
        return true; // Ya hay usuarios, no crear por defecto
    }
    
    std::cout << "Creando usuarios por defecto..." << std::endl;
    
    // Crear usuario administrador por defecto con contraseñas simples
    Usuario admin(0, "admin", "admin", "admin");
    std::cout << "Creando admin - Nombre: '" << admin.getNombre() 
              << "', Clave: '" << admin.getClave() << "', Tipo: '" << admin.getTipo() << "'" << std::endl;
    if (!agregarUsuario(admin)) {
        std::cerr << "Error creando usuario admin por defecto" << std::endl;
        return false;
    }
    
    // Crear usuario normal por defecto con contraseñas simples
    Usuario user(0, "user", "user", "normal");
    std::cout << "Creando user - Nombre: '" << user.getNombre() 
              << "', Clave: '" << user.getClave() << "', Tipo: '" << user.getTipo() << "'" << std::endl;
    if (!agregarUsuario(user)) {
        std::cerr << "Error creando usuario normal por defecto" << std::endl;
        return false;
    }
    
    std::cout << "Usuarios por defecto creados: admin/admin, user/user" << std::endl;
    return true;
}
