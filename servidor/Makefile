CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I../inc -I../lib
LIBS = -lsqlite3 ../lib/XmlRpcClient.o ../lib/XmlRpcDispatch.o ../lib/XmlRpcServer.o ../lib/XmlRpcServerConnection.o ../lib/XmlRpcServerMethod.o ../lib/XmlRpcSocket.o ../lib/XmlRpcSource.o ../lib/XmlRpcUtil.o ../lib/XmlRpcValue.o
# Archivos objeto
OBJS = Usuario.o GestorBBDD.o
REPORTES_OBJS = GestorArchivos.o GestorReportes.o
GCODEG_OBJS = Serial.o GestorArchivos.o GestorCodigoG.o
SERVIDOR_OBJS = Usuario.o GestorBBDD.o GestorArchivos.o GestorReportes.o Serial.o GestorCodigoG.o ServidorRpc.o

# Objetivos principales
all: test_bbdd test_reportes test_gcodeg servidor_robot

# Compilar objetos
%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Programa de prueba BBDD
test_bbdd: $(OBJS) test_bbdd.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

# Programa de prueba Reportes
test_reportes: $(REPORTES_OBJS) test_reportes.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

# Programa de prueba GestorCodigoG
test_gcodeg: $(GCODEG_OBJS) test_gcodeg.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

# Servidor Robot completo
servidor_robot: $(SERVIDOR_OBJS) main_servidor.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

# Limpiar
clean:
	rm -f *.o test_bbdd test_reportes test_gcodeg servidor_robot 

.PHONY: all clean
