#ifndef GESTORREPORTES_H
#define GESTORREPORTES_H

#include <string>

class GestorReportes {
public:
    // Genera un CSV resumen con información de users.csv y logs.csv
    static std::string ReporteGeneral();

    // Genera un CSV con los usuarios asociados al admin dado
    static std::string ReporteAdmin(int idAdmin);

    // Genera un CSV con logs entre las fechas (inclusive)
    static std::string ReporteLog(const std::string &FechaDesde, const std::string &FechaHasta);

    // Filtra users.csv y logs.csv por el texto dado
    static std::string Filtrar(const std::string &filtro);
};

#endif // GESTORREPORTES_H
