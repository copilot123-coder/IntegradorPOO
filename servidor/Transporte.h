#ifndef TRANSPORTE_H
#define TRANSPORTE_H
#include <string>
#include "Mensaje_145.h"
#include "MensajeIdentificado_145.h"
using namespace std;


class Transporte_145 {
    public:
        // Convierte un mensaje a texto. Estoy simulando como si lo empaqueto y lo envio por puerto serie
        string enviar(Mensaje_145 m);
        string enviar(MensajeIdentificado_145 m);


        // Reconstruyo el mensaje desde el texto empaquetado 'recibido' por puerto serie
        void desempaquetar(string paquete, Mensaje_145 &m);
        void desempaquetar(string paquete, MensajeIdentificado_145 &m);
};


#endif