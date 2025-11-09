#ifndef GESTORBBDD_H
#define GESTORBBDD_H

#include <string>
#include <memory>
#include "Usuario.h"
#include <sqlite3.h>

class GestorBBDD {
public:
    explicit GestorBBDD(const std::string& dbPath = "servidor_users.db");
    ~GestorBBDD();

    void inicializar();
    std::shared_ptr<Usuario> obtenerUsuarioPorNombre(const std::string& nombre);

private:
    std::string dbPath;
    sqlite3* db = nullptr;
    void ensureAdmin();
};

#endif // GESTORBBDD_H
