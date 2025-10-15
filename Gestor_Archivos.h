#ifndef GESTOR_ARCHIVOS_H
#define GESTOR_ARCHIVOS_H

#include <string>
#include <fstream>

class Gestor_Archivos {
public:
    std::string filename;
    std::string owner;

    Gestor_Archivos() = default;
    Gestor_Archivos(const std::string &filename, const std::string &owner);

    // Abre el archivo en modo lectura/escritura (crea si no existe cuando se escribe)
    bool open(const std::string &path);
    // Cierra el stream si está abierto
    void close();
    // Lee todo el contenido del archivo y lo devuelve como string
    std::string ReadAll();
    // Escribe una línea al final del archivo (con salto de línea)
    bool WriteLine(const std::string &line);
    // Consulta si un archivo existe en disco
    static bool FileExists(const std::string &path);

    // Lee un CSV (ruta) y muestra en tabla con N columnas (si N<=0 utiliza separación por comas)
    // Devuelve true si pudo abrir y procesar el CSV
    bool ImprimirTablaCSV(const std::string &CSVpath, int N);

    ~Gestor_Archivos();

private:
    std::fstream fs;
};

#endif // GESTOR_ARCHIVOS_H
