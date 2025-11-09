#include "GestorBBDD.h"
#include <iostream>

GestorBBDD::GestorBBDD(const std::string& dbPath) : dbPath(dbPath), db(nullptr) {}

GestorBBDD::~GestorBBDD() {
    if (db) sqlite3_close(db);
}

void GestorBBDD::inicializar() {
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr << "GestorBBDD: no se pudo abrir DB: " << (db?sqlite3_errmsg(db):"(null)") << std::endl;
        if (db) sqlite3_close(db);
        db = nullptr;
        return;
    }

    const char* sql_create =
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "nombre TEXT UNIQUE NOT NULL,"
        "clave TEXT NOT NULL,"
        "tipo TEXT NOT NULL"
        ");";
    char* err = nullptr;
    rc = sqlite3_exec(db, sql_create, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::cerr << "GestorBBDD: error creando tabla: " << (err?err:"") << std::endl;
        sqlite3_free(err);
    }

    ensureAdmin();
}

void GestorBBDD::ensureAdmin() {
    if (!db) return;
    const char* sql_insert =
        "INSERT OR IGNORE INTO users (nombre, clave, tipo) VALUES ('admin','admin123','admin');";
    char* err = nullptr;
    int rc = sqlite3_exec(db, sql_insert, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::cerr << "GestorBBDD: error insert admin: " << (err?err:"") << std::endl;
        sqlite3_free(err);
    }
}

std::shared_ptr<Usuario> GestorBBDD::obtenerUsuarioPorNombre(const std::string& nombre) {
    if (!db) return nullptr;
    const char* sql = "SELECT id, nombre, clave, tipo FROM users WHERE nombre = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return nullptr;
    }
    sqlite3_bind_text(stmt, 1, nombre.c_str(), -1, SQLITE_TRANSIENT);
    std::shared_ptr<Usuario> res = nullptr;
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char* nombre_c = sqlite3_column_text(stmt, 1);
        const unsigned char* clave_c = sqlite3_column_text(stmt, 2);
        const unsigned char* tipo_c  = sqlite3_column_text(stmt, 3);
        std::string nombre_s = nombre_c ? reinterpret_cast<const char*>(nombre_c) : "";
        std::string clave_s  = clave_c  ? reinterpret_cast<const char*>(clave_c)  : "";
        std::string tipo_s   = tipo_c   ? reinterpret_cast<const char*>(tipo_c)   : "";
        res = std::make_shared<Usuario>(id, nombre_s, clave_s, tipo_s);
    }
    sqlite3_finalize(stmt);
    return res;
}
