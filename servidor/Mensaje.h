#ifndef MENSAJE_H
#define MENSAJE_H
#include <vector>
#include <string>
#include "Dato.h"
using namespace std;


class Mensaje {
    private:
        int codigoOperacion;
        vector<Dato> datos;
    public:
        Mensaje();
        void setCodigoOperacion(int c);
        int getCodigoOperacion();
        void agregarDato(string nombre, string valor);
        string obtenerDato(string nombre);
        vector<Dato> obtenerTodosLosDatos();
        void limpiarDatos();


        bool guardarDatosEnArchivo(string ruta);
        bool cargarDatosDeArchivo(string ruta);


        void imprimir();
};


#endif