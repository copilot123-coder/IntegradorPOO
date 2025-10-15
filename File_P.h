#ifndef FILE_P
#define FILE_P

#include <cstring>
#include <iostream>
#include <vector>
#include <variant>
     
#include <fstream>

using datos = std::variant<int, double, std::string>;

class File_P{
    public:
        File_P();
        ~File_P();

        void set_FileType(char tipo_de_archivo);
        void set_fs(const std::string &ruta, const std::string &Modo);

        //==============================================================================================================

        void Leer_Archivo();
        void Leer_Tipo_De_Archivo(char tipo_de_archivo);// Primero hay que configurar filetype a leer y luego ingresamos el
                                                        // tipo_de_archivo que queremos mostrar por terminal
        void Leer_Linea(int N_linea);

        void EscribirArchivo(); // Escribe varias lineas al final del archivo
        void Escribir_Al_Final(std::string &frase);
        void Reescribir_Linea(std::string &frase,int N_linea);

        void Borrar_Archivo();

        void MostrarDatos();
        void MostrarArchivo();

        // Utiles==================================================================
        std::string QueTipoEs(const datos &var);


    protected:
        std::fstream fs;
        std::vector<datos> V_dato;
        std::string ruta; // Ruta relativa desde el ejecutable hasta el archivo fs
        char FileType;


};

#endif
