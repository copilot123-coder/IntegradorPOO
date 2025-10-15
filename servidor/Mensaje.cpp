#include "Mensaje.h"
#include <iostream>
#include <fstream>


Mensaje::Mensaje() {
  codigoOperacion = 0;
}


void Mensaje::setCodigoOperacion(int c) {
  codigoOperacion = c;
}


int Mensaje::getCodigoOperacion() {
  return codigoOperacion;
}


void Mensaje::agregarDato(string nombre, string valor) {
  Dato d;
  d.nombre = nombre;
  d.valor = valor;
  datos.push_back(d);
}


string Mensaje::obtenerDato(string nombre) {
  for (size_t i = 0; i < datos.size(); i++) {
    if (datos[i].nombre == nombre) {
      return datos[i].valor;
    }
    }
  return "";
}


void Mensaje::limpiarDatos() {
  datos.clear();
}


bool Mensaje::guardarDatosEnArchivo(string ruta) {
  ofstream out(ruta.c_str());
  if (!out.is_open()) return false;
  // una línea por dato -> nombre=valor
  for (size_t i = 0; i < datos.size(); i++) {
    out << datos[i].nombre << "=" << datos[i].valor << std::endl;
}
out.close();
return true;
}


vector<Dato> Mensaje::obtenerTodosLosDatos() {
  return datos; 
}


bool Mensaje::cargarDatosDeArchivo(string ruta) {
  ifstream in(ruta.c_str());
  if (!in.is_open()) return false;
  datos.clear();
  string linea;
  while (getline(in, linea)) {
      
      if (!linea.empty() && linea.back() == '\r') {
          linea.pop_back();
      }
      size_t pos = linea.find('=');
      if (pos != string::npos) {
          string nom = linea.substr(0, pos);
          string val = linea.substr(pos + 1);
          agregarDato(nom, val);
      }
    }
  in.close();
  return true;
}


void Mensaje::imprimir() {
  cout << "Codigo de operacion: " << codigoOperacion << endl;
  cout << "Datos (" << datos.size() << "):" << endl;
  for (size_t i = 0; i < datos.size(); i++) {
    cout << " - " << datos[i].nombre << " = " << datos[i].valor << endl;
}
}