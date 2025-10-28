# Makefile para Servidor XML-RPC Completo
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -O2
LIBS = -lsqlite3 -lpthread

# Directorios
SRCDIR = .
SERVDIR = servidor
INCDIR = inc
LIBDIR = lib

# Archivos fuente del servidor
SERVER_SOURCES = $(SERVDIR)/ServidorRpc.cpp \
                 $(SERVDIR)/GestorBBDD.cpp \
                 $(SERVDIR)/GestorReportes.cpp \
                 $(SERVDIR)/GestorCodigoG.cpp \
                 $(SERVDIR)/Usuario.cpp \
                 $(SERVDIR)/Serial.cpp \
                 $(SERVDIR)/GestorArchivos.cpp

# Librerías XML-RPC precompiladas
XMLRPC_OBJECTS = $(LIBDIR)/XmlRpcServer.o \
                 $(LIBDIR)/XmlRpcServerMethod.o \
                 $(LIBDIR)/XmlRpcServerConnection.o \
                 $(LIBDIR)/XmlRpcValue.o \
                 $(LIBDIR)/XmlRpcDispatch.o \
                 $(LIBDIR)/XmlRpcSource.o \
                 $(LIBDIR)/XmlRpcSocket.o \
                 $(LIBDIR)/XmlRpcUtil.o

# Archivos objeto
SERVER_OBJECTS = $(SERVER_SOURCES:.cpp=.o)

# Ejecutables
TARGET_SERVIDOR = test_servidor_completo

# Regla principal
all: $(TARGET_SERVIDOR)

# Compilar el servidor completo
$(TARGET_SERVIDOR): test_servidor_completo.o $(SERVER_OBJECTS) $(XMLRPC_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

# Reglas de compilación para archivos objeto
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -I$(SERVDIR) -c $< -o $@

# Limpiar archivos generados
clean:
	rm -f $(TARGET_SERVIDOR) *.o $(SERVDIR)/*.o

# Ejecutar el servidor
run: $(TARGET_SERVIDOR)
	./$(TARGET_SERVIDOR)

# Verificar dependencias
check:
	@echo "Verificando dependencias..."
	@pkg-config --exists sqlite3 && echo "✓ SQLite3 encontrado" || echo "✗ SQLite3 no encontrado"
	@echo "✓ Compilador C++17: $(CXX)"
	@echo "✓ Flags: $(CXXFLAGS)"

# Mostrar ayuda
help:
	@echo "Comandos disponibles:"
	@echo "  make all      - Compilar servidor completo"
	@echo "  make run      - Compilar y ejecutar servidor"
	@echo "  make clean    - Limpiar archivos compilados"
	@echo "  make check    - Verificar dependencias"
	@echo "  make help     - Mostrar esta ayuda"

.PHONY: all clean run check help
