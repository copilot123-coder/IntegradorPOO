#include <cstring>
#include <iostream>
#include <vector>
#include <variant>
#include <fstream>
#include <algorithm>
#include <iomanip>

#include "File_P.h"


File_P::File_P(){
    this->ruta = "./Sin_Nombre.txt";
    this->FileType = 'n'; // No hay ningun tipo de archivo especificado

}

File_P::~File_P(){
    fs.close();

}

void File_P::set_fs(const std::string &ruta, const std::string &Modo){// Modo es rw, r o w
    if(fs.is_open()){
        fs.close();
    }
    fs.clear();

    this->ruta = ruta;

    if(Modo == "r"){
        this->fs.open(ruta, std::ios::in);

    } else if(Modo == "rw"){
        this->fs.open(ruta, std::ios::out | std::ios::ate | std::ios::in);

    }else if(Modo == "w"){
        this->fs.open(ruta, std::ios::out | std::ios::app);

    }
}

void File_P::set_FileType(char tipo_de_archivo){
    this->FileType = tipo_de_archivo;
}

//=====================================================================================================

void File_P::Leer_Archivo(){
    this->set_fs(ruta, "rw");

    std::vector<std::string> lineas;
    std::string line;

    lineas.clear();
    fs.clear();
    fs.seekg(0);

    while(getline(fs, line)){
        lineas.push_back(line);
    }

    for (int i=0; i<lineas.size(); i++){
        std::cout << lineas[i] << std::endl;

    }
        fs.close();
    
}

void File_P::EscribirArchivo(){

    this->set_fs(ruta, "w");

    fs << std::endl;
    std::cout << "ya puede escribir, para parar ingrese 'stop'" << std::endl;

    std::string line;
    while(true){
        getline(std::cin, line);

        if(line == "stop"){
            break;
        }
        fs << line << std::endl;
    }

    fs.close();
}

void File_P::Escribir_Al_Final(std::string &frase){
    this->set_fs(ruta, "w");

    fs << frase << std::endl;
}

/*void File_P::Reescribir_Linea(std::string &frase, int N_linea){
    // Apertura del archivo

    this->set_fs(ruta, "rw");

    // Leemos el archivo y obtenemos las lineas
    std::string linea_aux;
    int linea_actual = 0; // ultima linea escrita
    size_t pos = 0;
#include <cstring>
#include <iostream>
#include <vector>
#include <variant>
#include <fstream>
#include <algorithm>
#include <iomanip>

#include "File_P.h"


File_P::File_P(){
    fs.clear();
    fs.seekp(0);
    fs.seekg(0);

    while(getline(fs, linea_aux)){
        linea_actual++;

        pos += linea_aux.size() + 1;

        if(linea_actual == N_linea){
            break;
        }
    }

    // Agregamos espacios para completar la linea
    std::string espacios = "";

    getline(fs, linea_aux);
    for(int i = 0; i < (linea_aux.size() - frase.size()); i++){
        espacios += " ";
    }
    // Escribimos la linea
    fs.clear();
    fs.seekp(pos);
    fs << frase + espacios;
}*/

void File_P::Reescribir_Linea(std::string &frase, int N_linea) {

    // Abrir para lectura
    set_fs(ruta, "r");
    if (!fs.is_open()) {
        std::cerr << "No se pudo abrir el archivo para lectura.\n";
        return;
    }

    std::vector<std::string> lineas;
    std::string linea;

    while (std::getline(fs, linea)) {
        lineas.push_back(linea);
    }
    fs.close();

    // Verificar que la línea exista
    if (N_linea < 1 || N_linea > (int)lineas.size()) {
        std::cerr << "Número de línea inválido.\n";
        return;
    }

    // Reemplazar la línea
    lineas[N_linea - 1] = frase;

    // Reescribir todo el archivo
    set_fs(ruta, "rw");
    if (!fs.is_open()) {
        std::cerr << "No se pudo abrir el archivo para escribir.\n";
        return;
    }

    fs.clear();
    fs.seekp(0, std::ios::beg);
    for (const auto &l : lineas) {
        fs << l << '\n';
    }

    fs.close();

    std::cout << "✅ Línea " << N_linea << " reescrita correctamente.\n";
}


void File_P::Leer_Tipo_De_Archivo(char tipo_de_archivo) {
    // Apertura del archivo en modo lectura
    this->set_fs(ruta, "r");

    // Guardamos todo el archivo en un vector de strings
    std::vector<std::string> contenido; 
    contenido.clear();

    std::string linea_aux = "";
    while(getline(fs, linea_aux)){
        contenido.push_back(linea_aux);
    }

    // Manejo de los tipos de archivo a la hora de leer
    size_t pos = 0;

    std::string str_aux = "";
    std::string key_w;
    std::string valor;

    std::vector<datos> info_k;  
    std::vector<datos> info_v;  
    info_k.clear();
    info_v.clear();


    switch (FileType)
    {
        case 'x':
            for (int i=0; i < contenido.size();i++){
                str_aux = contenido[i];
                
                str_aux.erase(std::remove(str_aux.begin(), str_aux.end(), ' '), str_aux.end());// eliminamos los espacios
                pos = str_aux.find("<"); 
                size_t pos_f = str_aux.find(">");

                if(pos != std::string::npos || pos_f != std::string::npos){

                    key_w = str_aux.substr(pos+1, pos_f-(pos+1));
                    info_k.push_back(key_w);

                    pos = str_aux.find("</");
                    valor = str_aux.substr(pos_f+1, pos-(pos_f+1));


                    try {
                        int num = std::stoi(valor);
                        info_v.push_back(num);
                        
                    }
                    catch (...) {
                        try {
                            double num = std::stod(valor);
                            info_v.push_back(num);
                        }
                        catch (...) {
                            info_v.push_back(valor); // valor es un string si llegamos a este punto
                        }
                    }

                } else{
                    continue;
                }   
            }
        break;

        case 'c':
            for (int i=0; i < contenido.size();i++){
                str_aux = contenido[i];

                str_aux.erase(std::remove(str_aux.begin(), str_aux.end(), ' '), str_aux.end());// eliminamos los espacios

                pos = str_aux.find(","); // Buscamos ","
                size_t pos_f = str_aux.find(",", pos+1);
                size_t pos_inicial = 0;

                while(pos_f != std::string::npos && pos != std::string::npos){

                    pos = str_aux.find(",", pos_inicial); // Buscamos ","
                    pos_f = str_aux.find(",", pos+1);

                    if(pos != std::string::npos){

                        key_w = str_aux.substr(pos_inicial, pos-pos_inicial); 

                        if(pos_f == std::string::npos){
                            valor = str_aux.substr(pos+1);
                        }else{
                            valor = str_aux.substr(pos+1, pos_f-(pos+1));
                        }


                        info_k.push_back(key_w);

                        try {
                            int num = std::stoi(valor);
                            info_v.push_back(num);
                            
                        }
                        catch (...) {
                            try {
                                double num = std::stod(valor);
                                info_v.push_back(num);
                            }
                            catch (...) {
                                info_v.push_back(valor); // valor es un string si llegamos a este punto
                            }
                        }
                        
                        pos_inicial = pos_f+1;
                    } else{
                        continue;
                    }   
                }
            }
        break;

        case 'j':
            for (int i=0; i < contenido.size();i++){
                str_aux = contenido[i];

                str_aux.erase(std::remove(str_aux.begin(), str_aux.end(), ' '), str_aux.end());// eliminamos los espacios
                pos = str_aux.find(":"); // Buscamos ":"
                if(pos != std::string::npos){

                    key_w = str_aux.substr(0, pos); 
                    valor = str_aux.substr(pos+1);

                    info_k.push_back(key_w);

                    try {
                        int num = std::stoi(valor);
                        info_v.push_back(num);
                        
                    }
                    catch (...) {
                        try {
                            double num = std::stod(valor);
                            info_v.push_back(num);
                        }
                        catch (...) {
                            info_v.push_back(valor); // valor es un string si llegamos a este punto
                        }
                    }

                } else{
                    continue;
                }   
            }
        break;
    
        default:
            std::cout << "No es un tipo de archivo valido" << std::endl;

        break;
    }

    for(int i=0; i < info_k.size() ;i++){
        std::string tipo_d_k = QueTipoEs(info_k[i]);
        std::string tipo_d_v = QueTipoEs(info_v[i]);

        if(tipo_d_k == "int"){
            std::cout << *std::get_if<int>(&info_k[i]) << ":";

        }else if(tipo_d_k == "double"){
            std::cout << *std::get_if<double>(&info_k[i]) << ":";

        }else if(tipo_d_k == "string"){
            std::cout << *std::get_if<std::string>(&info_k[i]) << ":";
        
        }
        if(tipo_d_v == "int"){
            std::cout << *std::get_if<int>(&info_v[i]) << std::endl;
        
        }else if(tipo_d_v == "double"){
            std::cout << *std::get_if<double>(&info_v[i]) << std::endl;
        
        }else if(tipo_d_v == "string"){
            std::cout << *std::get_if<std::string>(&info_v[i]) << std::endl;
        
        }
    }

    /*std::cout << "\nContenido parseado:\n";
    for (size_t i = 0; i < info_k.size() && i < info_v.size(); ++i) {
        std::cout << std::left << std::setw(20);
        std::visit([](auto&& arg) { std::cout << arg; }, info_k[i]);
        std::cout << " -> ";
        std::visit([](auto&& arg) { std::cout << arg; }, info_v[i]);
        std::cout << "\n";
    }*/
}


void File_P::Leer_Linea(int N_linea){
    // Apertura del archivo
    this->set_fs(ruta, "rw");

    // Leemos el archivo y obtenemos las lineas
    std::string linea_aux;
    int linea_actual = 0; // ultima linea escrita

    fs.clear();
    fs.seekp(0);
    fs.seekg(0);

    bool leida = false;
    while(getline(fs, linea_aux)){
        linea_actual++;

        if(linea_actual == N_linea){
            leida= true;
            break;
        }
    }

    if(leida){
        std::cout << linea_aux << std::endl;

    }else{
        std::cout << "La linea no se pudo leer" << std::endl;
    }
}
    



void File_P::Borrar_Archivo(){

}

void File_P::MostrarDatos(){

}

void File_P::MostrarArchivo(){
    this->set_fs(ruta, "r");
    std::string linea_aux = "";
    std::string contenido;

    while(getline(fs, linea_aux)){
        contenido += linea_aux + "\n";
    }
    std::cout << contenido << std::endl;
}



// Utiles========================================

std::string File_P::QueTipoEs(const datos& var){ // devuelve un string "double" "int" o "string" segun el tipo de dato del elemento tipo variant
    
    if(std::holds_alternative<int>(var)){
        return "int";

    }else if(std::holds_alternative<double>(var)){
        return "double";

    }else if(std::holds_alternative<std::string>(var)){
        return "string";
    }

    return "-1";
}