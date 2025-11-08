# --- Variables de Compilación ---
CXX = g++
# Agregamos -MMD -MP para la generación automática de dependencias
CXXFLAGS = -std=c++17 -Wall -Wextra -I../inc -I../lib -MMD -MP

# --- Librerías ---
# La línea de LIBS que ya corregimos, apuntando a los .o de la librería
LIBS = -lsqlite3 

# --- Archivos Fuente (.cpp) ---
# Lista de todos los .cpp que SÍ son parte del servidor
# (Excluimos los backups y otros .cpp que no se usan)
SERVER_SRCS := main_servidor.cpp \
               ServidorRpc.cpp \
               GestorCodigoG.cpp \
               Serial.cpp \
               GestorReportes.cpp \
               GestorArchivos.cpp \
               GestorBBDD.cpp \
               Usuario.cpp

# --- Archivos Fuente (.cpp) para los tests ---
TEST_BBDD_SRCS := test_bbdd.cpp GestorBBDD.cpp Usuario.cpp
TEST_REPORTES_SRCS := test_reportes.cpp GestorReportes.cpp GestorArchivos.cpp
TEST_GCODEG_SRCS := test_gcodeg.cpp GestorCodigoG.cpp Serial.cpp GestorArchivos.cpp

# --- Generación Automática de Archivos Objeto (.o) ---
# Convierte todas las listas de .cpp a .o
SERVER_OBJS := $(patsubst %.cpp,%.o,$(SERVER_SRCS))
TEST_BBDD_OBJS := $(patsubst %.cpp,%.o,$(TEST_BBDD_SRCS))
TEST_REPORTES_OBJS := $(patsubst %.cpp,%.o,$(TEST_REPORTES_SRCS))
TEST_GCODEG_OBJS := $(patsubst %.cpp,%.o,$(TEST_GCODEG_SRCS))

# --- Objetivos (Targets) ---
TARGETS = servidor_robot test_bbdd test_reportes test_gcodeg

# El objetivo 'all' (por defecto) compila todo
all: $(TARGETS)

# --- Reglas de Enlazado (Linking) ---
# Regla para construir el servidor principal
servidor_robot: $(SERVER_OBJS)
	@echo "Enlazando $@..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

# Regla para construir el test de BBDD
test_bbdd: $(TEST_BBDD_OBJS)
	@echo "Enlazando $@..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

# Regla para construir el test de Reportes
test_reportes: $(TEST_REPORTES_OBJS)
	@echo "Enlazando $@..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

# Regla para construir el test de G-Code
test_gcodeg: $(TEST_GCODEG_OBJS)
	@echo "Enlazando $@..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

# --- Regla de Compilación Genérica ---
# Esta regla compila CUALQUIER .cpp a un .o
# (No necesita el .h)
%.o: %.cpp
	@echo "Compilando $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# --- Limpieza ---
# El 'clean' ahora también borra los archivos de dependencia (.d)
clean:
	@echo "Limpiando archivos compilados..."
	rm -f $(TARGETS) *.o *.d

# --- Inclusión de Dependencias ---
# Incluye todos los archivos .d (listas de dependencias de headers)
# que generó el compilador.
# Si un .h cambia, 'make' ahora lo sabrá.
-include $(SERVER_OBJS:.o=.d)
-include $(TEST_BBDD_OBJS:.o=.d)
-include $(TEST_REPORTES_OBJS:.o=.d)
-include $(TEST_GCODEG_OBJS:.o=.d)

# Declara los objetivos que no son archivos (son "falsos")
.PHONY: all clean