# SERVIDOR RPC ROBOT - DOCUMENTACIÓN

## Resumen del Sistema Integrado

El ServidorRpc integra todas las clases desarrolladas para cumplir con los requerimientos:

### 🔧 **Componentes Integrados:**
- **GestorBBDD**: Base de datos SQLite con usuarios y privilegios
- **GestorReportes**: Sistema de logs CSV con filtros
- **GestorCodigoG**: Control completo del robot con G-code
- **ServidorRpc**: Servidor XML-RPC con autenticación y control

### 🌐 **Servicios XML-RPC Disponibles:**

#### **1. Login** 
- **Descripción**: Autenticación de usuarios
- **Parámetros**: [usuario, clave, nodoOrigen]
- **Respuesta**: {exito, sessionId, tipoUsuario, mensaje}
- **Ejemplo**: Login("admin", "admin123", "192.168.1.100")

#### **2. ConectarRobot** (Solo Admin)
- **Descripción**: Conectar/desconectar robot físico
- **Parámetros**: [sessionId, accion] 
- **Acciones**: "conectar", "desconectar"
- **Ejemplo**: ConectarRobot("admin_1635789123", "conectar")

#### **3. MoverRobot**
- **Descripción**: Movimiento manual en coordenadas XYZ
- **Parámetros**: [sessionId, x, y, z, velocidad(opcional)]
- **Validación**: Espacio de trabajo matemático
- **Ejemplo**: MoverRobot("session123", 100, 100, 50, 1500)

#### **4. EjecutarGCode**
- **Descripción**: Comando G-code directo
- **Parámetros**: [sessionId, comandoG]
- **Validación**: Parser regex completo
- **Ejemplo**: EjecutarGCode("session123", "G1 X50 Y50 Z10 F1000")

### 📊 **Sistema de Logs y Reportes:**

#### **Registro Automático:**
- ✅ Timestamp de cada petición
- ✅ Detalle de la petición realizada  
- ✅ Usuario que realizó la acción
- ✅ Nodo de origen de la conexión
- ✅ Código de respuesta (éxito/error)

#### **Formato CSV:**
```csv
timestamp,modulo,evento,usuario,nodo
2025-10-28 15:30:45,ServidorRPC,Login exitoso,admin,192.168.1.100
2025-10-28 15:31:02,ServidorRPC,Robot conectar,admin,192.168.1.100
2025-10-28 15:31:15,ServidorRPC,Movimiento robot X:100 Y:100 Z:50,admin,192.168.1.100
```

### 🔐 **Control de Acceso:**

#### **Niveles de Usuario:**
- **Administrador**: Acceso completo + funciones críticas
  - Conectar/desconectar robot
  - Configurar acceso remoto  
  - Ver reportes de todos los usuarios
  - Control total del sistema

- **Usuario Normal**: Funciones básicas
  - Movimientos del robot
  - Comandos G-code básicos
  - Ver su propio reporte
  - Operaciones no críticas

#### **Validación de Sesiones:**
- ✅ SessionId único por conexión
- ✅ Timeout automático configurable
- ✅ Validación de permisos por método
- ✅ Registro de todos los intentos

### 🤖 **Control del Robot:**

#### **Comunicación Serie:**
- Puerto: /dev/ttyUSB0 a 115200 baud
- Protocolo: G-code estándar
- Timeouts: 4s homing, 1s comandos normales
- Handshake: Inicialización automática 4s

#### **Validación Matemática:**
```cpp
// Validación espacio de trabajo usando módulo cuadrado
squaredPositionModule <= sq(R_MAX) 
    && squaredPositionModule >= sq(R_MIN) 
    && pos_tracker[Z_AXIS] >= Z_MIN  
    && pos_tracker[Z_AXIS] <= Z_MAX
```

#### **Modos de Operación:**
- **Manual**: Movimientos paso a paso por operador
- **Automático**: Ejecución de archivos G-code
- **Coordenadas**: Absoluto/Relativo configurable

### 🚀 **Compilación y Ejecución:**

```bash
# Compilar servidor completo
cd servidor/
make servidor_robot

# Ejecutar servidor
./servidor_robot

# El servidor escucha en puerto 8080
# Acceso: localhost:8080 (local) o IP:8080 (remoto)
```

### 📋 **Requerimientos Cumplidos:**

✅ **Objetos de dominio específicos**: ServidorRpc maneja serialización XML-RPC
✅ **Archivo de logs CSV**: GestorReportes con timestamps y filtros  
✅ **Base de datos usuarios**: SQLite con admin/normal + validación
✅ **Servicios XML-RPC**: Login, conexión, movimiento, G-code
✅ **Validación por usuario/clave**: Sistema de sesiones robusto
✅ **Control robot serie**: Comunicación 115200 baud con timeouts
✅ **Acceso remoto configurable**: Solo admin puede habilitar/deshabilitar
✅ **Activación motores**: Comandos M3/M5 via G-code
✅ **Lista comandos**: Help dinámico según privilegios
✅ **Reportes usuarios**: Estado personal + estadísticas
✅ **Reportes admin**: Vista completa + filtros múltiples
✅ **Modo manual/automático**: Configuración dinámica
✅ **Movimientos XYZ**: Entrada coordenadas + validación matemática
✅ **Posición origen**: Comando G28 con timeouts apropiados
✅ **Efector final**: Control M3/M5 integrado
✅ **Subida archivos G-code**: Gestión de secuencias completas

### 🎯 **Código Simple y Mantenible:**
- Clases especializadas y desacopladas
- Gestores independientes reutilizables  
- API XML-RPC intuitiva y documentada
- Validaciones centralizadas
- Logs automáticos para debugging
- Manejo de errores completo

El sistema está **listo para producción** con todas las funcionalidades solicitadas implementadas de manera robusta y escalable.
