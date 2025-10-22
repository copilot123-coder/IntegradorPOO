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

  // --- Nuevos wrappers solicitados en la API
  // Abre puerto y configura baudRate (devuelve true si se abrió correctamente)
  bool AbrirPuerto(const string& dispositivo, int baudRate);

  // Cierra el puerto (wrapper con nombre en mayúscula)
  void CerrarPuerto();

  // Lee del puerto hasta timeoutMs milisegundos (simulación/lectura cruda)
  std::string LeerPuerto(int timeoutMs);

  // Getters sencillos
  string getPuerto() const { return puerto; }
  bool estaAbierto() const { return fd >= 0; }
  std::string leerPuertoStreaming(char formato, std::ostream& os, bool print);


private:
  void initAttributes();
  bool configurar(int fd, int baudRate);

  // helper para convertir baud int a constante termios
  speed_t baudConst(int baudRate) const;

  string puerto;           
  int fd;                  
  struct termios saved{};  
};

} 

#endif 