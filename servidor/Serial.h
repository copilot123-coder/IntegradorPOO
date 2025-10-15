#ifndef SERIAL_H
#define SERIAL_H

#include <string>
#include <termios.h>

namespace tp2 {

using std::string;

class Serial
{
public:
  // Constructor y destructor
  Serial();
  ~Serial();

  bool abrirPuerto(const string& dispositivo);
  bool abrirPuerto(); 
  void cerrarPuerto();

  // Envía 'c','x','j' (minúsculas) y devuelve lo leído (CSV/XML/JSON).
  string leerPuerto(char formato);

  // Getters sencillos
  string getPuerto() const { return puerto; }
  bool estaAbierto() const { return fd >= 0; }
  std::string leerPuertoStreaming(char formato, std::ostream& os, bool print);


private:
  void initAttributes();
  bool configurar(int fd);

  string puerto;           
  int fd;                  
  struct termios saved{};  
};

} 

#endif 