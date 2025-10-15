#ifndef TRANSPORTE_H
#define TRANSPORTE_H
#include <string>
#include "Mensaje.h"
using namespace std;


class Transporte {
    public:
        // Convierte un mensaje a texto. Estoy simulando como si lo empaqueto y lo envio por puerto serie
        string enviar(Mensaje m);


        // Reconstruyo el mensaje desde el texto empaquetado 'recibido' por puerto serie
        void desempaquetar(string paquete, Mensaje &m);
};


#endif