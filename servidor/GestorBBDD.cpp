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
    return crearTablas();
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
    std::string sql = "INSERT INTO usuarios (nombre, clave, tipo) VALUES ('" +
                     escaparComillas(usuario.getNombre()) + "', '" +
                     escaparComillas("clave_temp") + "', '" +
                     escaparComillas(usuario.getTipo()) + "');";
    return ejecutarSQL(sql);
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
