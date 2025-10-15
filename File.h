#ifndef FILE_H
#define FILE_H
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include "Trans_Rec.h"
namespace Tp2_Act_1 {
class File {
protected:
std::fstream Archivo;
std::string ruta;
char FileType;
Tp2_Act_1::Trans_Rec* Mje; 
public:
File(std::fstream& Archivo, std::string ruta, char FileType, Tp2_Act_1::Trans_Rec*
Mje);
bool abrir(std::ios::openmode modo);
void escribir(const std::string& data, int Saltos_de_Linea);
std::uintmax_t getTamano();
std::string getFechaDeCreacion();
std::string leer();
void cerrar();
char GetFileType() const;
~File();
void eliminar();
void Formato();
std::vector<std::string> Separador(std::string str);
};
std::string interpretar(const std::string& input, char formato);
}
#endif