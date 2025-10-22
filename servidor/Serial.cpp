#include "Serial.h"

#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <cctype>
#include <ostream>
#include <iostream>

namespace tp2 {

using std::string;


static void set_raw_8N1(termios& tty, speed_t spd) {
  cfmakeraw(&tty);
  tty.c_cflag &= ~CSIZE;
  tty.c_cflag |= CS8;
  tty.c_cflag |= (CLOCAL | CREAD);
  tty.c_cflag &= ~PARENB;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CRTSCTS;

  tty.c_cc[VMIN]  = 0;
  // VTIME in deciseconds -> set small default
  tty.c_cc[VTIME] = 1;

  cfsetispeed(&tty, spd);
  cfsetospeed(&tty, spd);
}

Serial::Serial() { initAttributes(); }

Serial::~Serial() { cerrarPuerto(); }

void Serial::initAttributes() {
  puerto = "/dev/ttyUSB0"; 
  fd = -1;
  std::memset(&saved, 0, sizeof(saved));
}

bool Serial::configurar(int fd_) {
  termios tty{};
  if (tcgetattr(fd_, &tty) != 0) return false;
  saved = tty;                
  set_raw_8N1(tty, B19200);
  tcflush(fd_, TCIFLUSH);
  return (tcsetattr(fd_, TCSANOW, &tty) == 0);
}

bool Serial::configurar(int fd_, int baudRate) {
  termios tty{};
  if (tcgetattr(fd_, &tty) != 0) return false;
  saved = tty;
  speed_t spd = B19200;
  switch (baudRate) {
    case 9600: spd = B9600; break;
    case 19200: spd = B19200; break;
    case 38400: spd = B38400; break;
    case 57600: spd = B57600; break;
    case 115200: spd = B115200; break;
    default: spd = B19200; break;
  }
  set_raw_8N1(tty, spd);
  tcflush(fd_, TCIFLUSH);
  return (tcsetattr(fd_, TCSANOW, &tty) == 0);
}

bool Serial::abrirPuerto(const string& dispositivo) {
  if (fd >= 0) cerrarPuerto();
  puerto = dispositivo;
  fd = ::open(puerto.c_str(), O_RDWR | O_NOCTTY);
  if (fd < 0) return false;
  if (!configurar(fd)) {
    cerrarPuerto();
    return false;
  }
  return true;
}

bool Serial::abrirPuerto() {
  return abrirPuerto(puerto);
}

void Serial::cerrarPuerto() {
  if (fd >= 0) {
    ::close(fd);
    fd = -1;
  }
}

string Serial::leerPuerto(char formato) {
  if (fd < 0) {
    if (!abrirPuerto(puerto)) return std::string();
  }
  char cmd = std::tolower(static_cast<unsigned char>(formato));
  if (cmd != 'c' && cmd != 'j' && cmd != 'x') return std::string();

  tcflush(fd, TCIFLUSH);

  // Enviar el comando
  if (::write(fd, &cmd, 1) != 1) return std::string();
  tcdrain(fd);

  auto completo = [&](const std::string& s) -> bool {
    if (cmd == 'c') return s.find('\n') != std::string::npos || s.find('\r') != std::string::npos;
    if (cmd == 'j') return s.find('}')  != std::string::npos;
    if (cmd == 'x') return s.find("</registro>") != std::string::npos;
    return false;
  };

  std::string out;
  const int step_ms = 80;        
  const int max_total_ms = 1000; 
  int elapsed = 0;

  while (elapsed < max_total_ms && !completo(out)) {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    timeval tv{0, step_ms * 1000};

    int r = ::select(fd + 1, &rfds, nullptr, nullptr, &tv);
    if (r > 0 && FD_ISSET(fd, &rfds)) {
      char buf[256];
      ssize_t n = ::read(fd, buf, sizeof(buf));
      if (n > 0) out.append(buf, buf + n);
    } else if (r == 0) {
      elapsed += step_ms; 
    } else if (errno == EINTR) {
      continue;
    } else {
      break; 
    }
  }

  if (cmd == 'c') {
    size_t e = out.find_first_of("\r\n");
    if (e != std::string::npos) out.erase(e);
  }

  return out;
}

bool Serial::AbrirPuerto(const string& dispositivo, int baudRate) {
  if (fd >= 0) cerrarPuerto();
  puerto = dispositivo;
  baudRate_ = baudRate;
  fd = ::open(puerto.c_str(), O_RDWR | O_NOCTTY);
  if (fd < 0) return false;
  if (!configurar(fd, baudRate_)) {
    cerrarPuerto();
    return false;
  }
  return true;
}

void Serial::CerrarPuerto() {
  cerrarPuerto();
}

std::string Serial::LeerPuerto(int timeoutMs) {
  if (fd < 0) {
    if (!abrirPuerto(puerto)) return std::string();
  }

  // lectura simplificada: espera hasta timeoutMs y recoge lo que haya
  std::string out;
  const int step_ms = 80;
  int elapsed = 0;
  while (elapsed < timeoutMs) {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    timeval tv{0, step_ms * 1000};
    int r = ::select(fd + 1, &rfds, nullptr, nullptr, &tv);
    if (r > 0 && FD_ISSET(fd, &rfds)) {
      char buf[256];
      ssize_t n = ::read(fd, buf, sizeof(buf));
      if (n > 0) out.append(buf, buf + n);
      else break;
    } else if (r == 0) {
      elapsed += step_ms;
    } else if (errno == EINTR) {
      continue;
    } else {
      break;
    }
  }
  return out;
}

std::string Serial::leerPuertoStreaming(char formato, std::ostream& os, bool print=true) {
  if (fd < 0) {
    if (!abrirPuerto(puerto)) return std::string();
  }

  char cmd = std::tolower(static_cast<unsigned char>(formato));
  if (cmd != 'c' && cmd != 'j' && cmd != 'x') return std::string();

  tcflush(fd, TCIFLUSH);

  if (::write(fd, &cmd, 1) != 1) return std::string();
  tcdrain(fd);

  auto completo = [&](const std::string& s) -> bool {
    if (cmd == 'c') return s.find('\n') != std::string::npos || s.find('\r') != std::string::npos;
    if (cmd == 'j') return s.find('}')  != std::string::npos;
    if (cmd == 'x') return s.find("</registro>") != std::string::npos;
    return false;
  };

  std::string out;
  const int step_ms = 80;
  const int max_total_ms = 1500;
  int elapsed = 0;

  while (elapsed < max_total_ms && !completo(out)) {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    timeval tv{0, step_ms * 1000};

    int r = ::select(fd + 1, &rfds, nullptr, nullptr, &tv);
    if (r > 0 && FD_ISSET(fd, &rfds) && print) {
      char buf[256];
      ssize_t n = ::read(fd, buf, sizeof(buf));
      if (n > 0) {
        out.append(buf, buf + n);
        os.write(buf, n);   
        os.flush();         
      }
    } else if (r == 0) {
      elapsed += step_ms;
    } else if (errno == EINTR) {
      continue;
    } else {
      break;
    }
  }

  
  if (cmd == 'c') {
    size_t e = out.find_first_of("\r\n");
    if (e != std::string::npos) {
      
      out.erase(e);
    }
  }
  return out;
}

} 
