#ifndef GESTOR_REPORTES_H
#define GESTOR_REPORTES_H

#include <string>

class GestorReportes {
public:
    GestorReportes() = default;

    std::string ReporteGeneral();
    std::string ReporteAdmin(int idAdmin);
    std::string ReporteLog(const std::string &FechaDesde, const std::string &FechaHasta);
    std::string Filtrar(const std::string &filtro);
};

#endif // GESTOR_REPORTES_H
