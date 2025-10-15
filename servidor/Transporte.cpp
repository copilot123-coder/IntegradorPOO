#include "Transporte_145.h"
#include <sstream>


static void agregarKV(string &s, string k, string v) {
    if (!s.empty()) s += ";";
    s += k + "=" + v;
}


string Transporte_145::enviar(Mensaje_145 m) {
   
    string s = "";
 
    {
    ostringstream oss; oss << m.getCodigoOperacion();
    agregarKV(s, "CODIGO", oss.str());
    }
    
    vector<Dato_145> lista = m.obtenerTodosLosDatos();
    for (size_t i = 0; i < lista.size(); i++) {
        agregarKV(s, lista[i].nombre, lista[i].valor);
    }
    return s;
}


string Transporte_145::enviar(MensajeIdentificado_145 m) {
    string s = "";
    agregarKV(s, "USUARIO", m.getIdUsuario());
    agregarKV(s, "DISPOSITIVO", m.getIdDispositivo());


    // Reusar empaquetado base para el resto
    string base = enviar((Mensaje_145)m);
    if (!base.empty()) {
    if (!s.empty()) s += ";";
        s += base;
    }
    return s;
}


void Transporte_145::desempaquetar(string paquete, Mensaje_145 &m) {

    m.setCodigoOperacion(0);
    m.limpiarDatos();


    size_t start = 0;
    while (start < paquete.size()) {
        size_t sep = paquete.find(';', start);
        string kv;
        if (sep == string::npos) { kv = paquete.substr(start); start = paquete.size(); }
        else { kv = paquete.substr(start, sep - start); start = sep + 1; }


        size_t eq = kv.find('=');
        if (eq != string::npos) {
            string k = kv.substr(0, eq);
            string v = kv.substr(eq + 1);
            if (k == "CODIGO") {
                int c = 0;
                for (size_t i = 0; i < v.size(); i++) {
                    if (v[i] >= '0' && v[i] <= '9') c = c * 10 + (v[i]-'0');
                }
            m.setCodigoOperacion(c);
            } else {
                m.agregarDato(k, v);
            }
        }
    }
}


void Transporte_145::desempaquetar(string paquete, MensajeIdentificado_145 &m) {
  
    m.setCodigoOperacion(0);
    m.limpiarDatos();
    m.setIdUsuario("");
    m.setIdDispositivo("");

    size_t start = 0;
    while (start < paquete.size()) {
        size_t sep = paquete.find(';', start);
        string kv = (sep == string::npos) ? paquete.substr(start)
                                          : paquete.substr(start, sep - start);
        start = (sep == string::npos) ? paquete.size() : sep + 1;

        size_t eq = kv.find('=');
        if (eq != string::npos) {
            string k = kv.substr(0, eq);
            string v = kv.substr(eq + 1);

            if (k == "CODIGO") {
                int c = 0;
                for (size_t i = 0; i < v.size(); i++) {
                    if (v[i] >= '0' && v[i] <= '9') c = c * 10 + (v[i] - '0');
                }
                m.setCodigoOperacion(c);
            } else if (k == "USUARIO") {
                m.setIdUsuario(v);          // No va a la colección
            } else if (k == "DISPOSITIVO") {
                m.setIdDispositivo(v);      // No va a la colección
            } else {
                m.agregarDato(k, v);        // El resto sí
            }
        }
    }
}