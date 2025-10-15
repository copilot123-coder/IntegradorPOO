#include "File.h"
#include <filesystem>
#include <algorithm>
#include <cctype>
namespace Tp2_Act_1 {
char File::GetFileType() const { return FileType; }
File::File(std::fstream& Archivo, std::string ruta, char FileType,
Tp2_Act_1::Trans_Rec* Mje) {
this->Archivo.swap(Archivo);
this->ruta = ruta;
this->FileType = FileType;
this->Mje = Mje;
}

File::~File() {
 if (Archivo.is_open()) {
 Archivo.close();
 }
 std::cout << "Destructor de File: archivo cerrado (no eliminado)." << std::endl;
 }
 void File::eliminar() {
 try {
 if (std::filesystem::remove(ruta)) {
 std::cout << ruta << " eliminado" << std::endl;
 } else {
 std::cout << "No se pudo eliminar (no existe?): " << ruta << "\n";
 }
 } catch (const std::filesystem::filesystem_error& e) {
 std::cerr << "Error al borrar: " << e.what() << std::endl;
 }
 }
 bool File::abrir(std::ios::openmode modo) {
 Archivo.open(ruta, modo);
 return Archivo.is_open();
 }
 void File::escribir(const std::string& data, int Saltos_de_Linea)
 if (Archivo.is_open()) {
Archivo << data;
for (int i = 0; i < Saltos_de_Linea; i++) {
Archivo << std::endl;
}
}
}
std::uintmax_t File::getTamano() {
try {
return std::filesystem::file_size(ruta);
} catch (std::filesystem::filesystem_error& e) {
std::cerr << "Error obteniendo tamaño: " << e.what() << std::endl;
return 0;
}
}
std::string File::getFechaDeCreacion() {
try {
auto ftime = std::filesystem::last_write_time(ruta);
auto sctp =
std::chrono::time_point_cast<std::chrono::system_clock::duration>(
ftime - decltype(ftime)::clock::now() + std::chrono::system_clock::now()
);
std::time_t cftime = std::chrono::system_clock::to_time_t(sctp); {
    std::stringstream ss;
ss << std::put_time(std::localtime(&cftime), "%Y-%m-%d %H:%M:%S");
return ss.str();
} catch (std::filesystem::filesystem_error& e) {
std::cerr << "Error obteniendo fecha: " << e.what() << std::endl;
return "Fecha no disponible";
}
}
std::string interpretar(const std::string& input, char formato) {
std::string value;
switch(formato) {
case 'x': { // XML
size_t ini = input.find('<');
size_t fin = input.find('>');
size_t cierre = input.rfind('<');
if (ini != std::string::npos && fin != std::string::npos && cierre !=
std::string::npos) {
value = input.substr(fin + 1, cierre - fin - 1);
}
break;
}
case 'j': { // JSON
size_t ini = input.find(':');
if (ini != std::string::npos) {
value = input.substr(ini + 1);
// quitar espacios iniciales
while (!value.empty() && value[0] == ' ')
value.erase(0,1);
// si empieza con comillas → es string
if (!value.empty() && value[0] == '"') {
value.erase(0,1); // quitar la primera comilla
size_t fin = value.find('"');
if (fin != std::string::npos) {
value = value.substr(0, fin); // quedarnos solo con el texto
}
} else {
// si es número → quitar comas o llaves al final
while (!value.empty() && (value.back() == ',' || value.back() == '}'))
value.pop_back();
}
}
break;
}
case 'c': { // CSV
value = input; // se devuelve tal cual
break;
}
default:
return "Formato no reconocido";
}
return value;
}
std::vector<std::string> File::Separador(std::string str) {
std::vector<std::string> listas;
std::string token;
std::stringstream ss(str);
if (FileType == 'c') {
// CSV → separar por comas
while (std::getline(ss, token, ',')) {
if (!token.empty()) listas.push_back(token);
}
} else {
// XML/JSON → separar por líneas
while (std::getline(ss, token, '\n')) {
if (!token.empty()) listas.push_back(token);
}
}
return listas;
}
void File::Formato() {
const std::string header = "Id Dispositivo
Caudal";
Apertura
std::string str_file;
// --- Si el archivo está vacío, escribimos cabeceras ---
bool archivoVacio = (getTamano() == 0);
if (archivoVacio) {
str_file = "Directorio: ./mios/";
escribir(str_file, 1);
str_file = "Archivo: " + ruta;
escribir(str_file, 1);
str_file = "Fecha de modificacion: " + getFechaDeCreacion();
escribir(str_file, 1);
str_file = "Tamaño: " + std::to_string(getTamano());
escribir(str_file, 1);
str_file = "Contenido: ";
escribir(str_file, 1);
escribir(header, 1);
}
if (Mje) {
std::string recibido = Mje->GetCadena();
std::vector<std::string> lineas = Separador(recibido);
std::string str_aux;
std::string linea_actual="";
int n = 0;
std::string espacios;
if(Archivo.is_open()) Archivo.close();
Archivo.open(ruta, std::ios::app); // Abrir en modo append para no
sobrescribir
if (FileType == 'c') {
// CSV: usar los campos separados
std::stringstream ss(recibido);
std::string campo;
int col = 0;
while (std::getline(ss, campo, ',')) {
int pos = 0;
if (col == 0) pos = header.find("Id Dispositivo");
else if (col == 1) pos = header.find("Apertura");
else if (col == 2) pos = header.find("Nivel");
else if (col == 3) pos = header.find("Caudal");
std::string espacios(pos - (int)linea_actual.size(), ' ');
Archivo << espacios << campo;
linea_actual += espacios + campo;
col++;
}
Archivo << std::endl;
}
else if (FileType == 'x') {
for (int i = 2; i < (lineas.size()-1); i++) {
str_aux = interpretar(lineas[i], FileType);
if (i == 2) {
Archivo << str_aux;
linea_actual += str_aux;
} else if (i == 3) {
n = header.find("Apertura");
espacios = std::string(n - (int)linea_actual.size(), ' ');
str_aux = espacios + str_aux;
Archivo << str_aux;
linea_actual += str_aux;
} else if (i == 4) {
n = header.find("Nivel");
espacios = std::string(n - (int)linea_actual.size(), ' ');
str_aux = espacios + str_aux;
Archivo << str_aux;
linea_actual += str_aux;
} else if (i == 5) {
n = header.find("Caudal");
espacios = std::string(n - (int)linea_actual.size(), ' ');
str_aux = espacios + str_aux;
Archivo << str_aux;
linea_actual += str_aux;
}
}
Archivo << std::endl;
}
else{
for (int i = 1; i < (lineas.size()-1); i++) {
str_aux = interpretar(lineas[i], FileType);
if (i == 1) {
Archivo << str_aux;
linea_actual += str_aux;
} else if (i == 2) {
n = header.find("Apertura");
espacios = std::string(n - (int)linea_actual.size(), ' ');
str_aux = espacios + str_aux;
Archivo << str_aux;
linea_actual += str_aux;
} else if (i == 3) {
n = header.find("Nivel");
espacios = std::string(n - (int)linea_actual.size(), ' ');
str_aux = espacios + str_aux;
Archivo << str_aux;
linea_actual += str_aux;
} else if (i == 4) {
n = header.find("Caudal");
espacios = std::string(n - (int)linea_actual.size(), ' ');
str_aux = espacios + str_aux;
Archivo << str_aux;
linea_actual += str_aux;
}
}
Archivo << std::endl;
}
}
}
std::string File::leer() {
std::string linea, contenido;
if (Archivo.is_open()) {
while (std::getline(Archivo, linea)) {
    contenido += linea + "\n";
}
}
return contenido;
}
void File::cerrar() {
if (Archivo.is_open()) {
Archivo.close();
}
}
}