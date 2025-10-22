#include <iostream>
#include <fstream>
#include "GestorReportes.h"

static void writeSampleFiles() {
    std::ofstream u("users.csv");
    // id,nombre,privilegios,activo
    u << "1,alice,admin:1,1\n";
    u << "2,bob,user,1\n";
    u << "3,charlie,moderator,0\n";
    u.close();

    std::ofstream l("logs.csv");
    // fecha,evento,detalle
    l << "2025-10-10,login,alice\n";
    l << "2025-10-12,logout,bob\n";
    l << "2025-10-14,update,charlie\n";
    l.close();
}

int main(){
    writeSampleFiles();
    GestorReportes gr;
    std::cout << "--- ReporteGeneral ---\n";
    std::cout << gr.ReporteGeneral() << std::endl;

    std::cout << "--- ReporteAdmin(1) ---\n";
    std::cout << gr.ReporteAdmin(1) << std::endl;

    std::cout << "--- ReporteLog 2025-10-11..2025-10-13 ---\n";
    std::cout << gr.ReporteLog("2025-10-11","2025-10-13") << std::endl;

    std::cout << "--- Filtrar 'bob' ---\n";
    std::cout << gr.Filtrar("bob") << std::endl;
    return 0;
}
