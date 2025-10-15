#ifndef MENSAJE_H
#define MENSAJE_H
#include <vector>
#include <string>
#include "Dato_145.h"
using namespace std;


class Mensaje_145 {
    private:
        int codigoOperacion;
        vector<Dato_145> datos;
    public:
        Mensaje_145();
        void setCodigoOperacion(int c);
        int getCodigoOperacion();
        void agregarDato(string nombre, string valor);
        string obtenerDato(string nombre);
        vector<Dato_145> obtenerTodosLosDatos();
        void limpiarDatos();


        bool guardarDatosEnArchivo(string ruta);
        bool cargarDatosDeArchivo(string ruta);


        void imprimir();
};


#endif