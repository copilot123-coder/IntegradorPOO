#include <iostream>
#include <fstream>
#include "Gestor_Archivos.h"

int main(){
    Gestor_Archivos g("demo.txt", "alumno");

    const std::string ruta = "demo.txt";
    if(!g.open(ruta)){
        std::cerr << "No se pudo abrir " << ruta << std::endl;
        return 1;
    }

    g.WriteLine("Linea de prueba 1");
    g.WriteLine("Linea de prueba 2");

    std::cout << "Contenido de " << ruta << ":\n";
    std::cout << g.ReadAll() << std::endl;

    g.close();

    // Crear un CSV de ejemplo
    const std::string csv = "demo.csv";
    {
        std::ofstream ofs(csv);
        ofs << "id,nombre,valor\n";
        ofs << "1,AA,10\n";
        ofs << "2,BB,20\n";
        ofs << "3,CC,30\n";
    }

    std::cout << "\nImprimiendo CSV en formato tabla:\n";
    // Usamos un gestor temporal para imprimir
    Gestor_Archivos g2("demo.csv", "alumno");
    if(!g2.ImprimirTablaCSV(csv, 3)){
        std::cerr << "No se pudo abrir/leer el CSV " << csv << std::endl;
    }

    return 0;
}
