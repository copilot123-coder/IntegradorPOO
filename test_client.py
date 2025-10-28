#!/usr/bin/env python3
"""
Cliente de prueba para el servidor XML-RPC del robot
"""

import xmlrpc.client
import sys
import time

class ClienteRobotRPC:
    def __init__(self, host='localhost', puerto=8080):
        self.servidor = xmlrpc.client.ServerProxy(f'http://{host}:{puerto}')
        self.session_id = None
        self.tipo_usuario = None
        
    def login(self, usuario, clave, nodo='localhost'):
        """Autenticación en el servidor"""
        try:
            resultado = self.servidor.Login(usuario, clave, nodo)
            if resultado['exito']:
                self.session_id = resultado['sessionId']
                self.tipo_usuario = resultado['tipoUsuario']
                print(f"✓ Login exitoso como {usuario} ({self.tipo_usuario})")
                print(f"  Session ID: {self.session_id}")
                return True
            else:
                print(f"✗ Error login: {resultado['mensaje']}")
                return False
        except Exception as e:
            print(f"✗ Error de conexión: {e}")
            return False
    
    def listar_comandos(self):
        """Obtener lista de comandos disponibles"""
        try:
            resultado = self.servidor.ListarComandos(self.session_id)
            if resultado['exito']:
                print(f"\n=== COMANDOS DISPONIBLES ({resultado['tipoUsuario']}) ===")
                for comando, descripcion in resultado['comandos'].items():
                    print(f"  {comando}: {descripcion}")
                return True
            else:
                print(f"✗ Error: {resultado['mensaje']}")
                return False
        except Exception as e:
            print(f"✗ Error: {e}")
            return False
    
    def conectar_robot(self, accion='conectar'):
        """Conectar/desconectar robot (solo admin)"""
        try:
            resultado = self.servidor.ConectarRobot(self.session_id, accion)
            if resultado['exito']:
                print(f"✓ {resultado['mensaje']}")
                return True
            else:
                print(f"✗ {resultado['mensaje']}")
                return False
        except Exception as e:
            print(f"✗ Error: {e}")
            return False
    
    def mover_robot(self, x, y, z, velocidad=None):
        """Mover robot a posición específica"""
        try:
            if velocidad:
                resultado = self.servidor.MoverRobot(self.session_id, x, y, z, velocidad)
            else:
                resultado = self.servidor.MoverRobot(self.session_id, x, y, z)
            
            if resultado['exito']:
                print(f"✓ {resultado['mensaje']} - Posición: X={x}, Y={y}, Z={z}")
                return True
            else:
                print(f"✗ {resultado['mensaje']}")
                return False
        except Exception as e:
            print(f"✗ Error: {e}")
            return False
    
    def ejecutar_gcode(self, comando):
        """Ejecutar comando G-Code directo"""
        try:
            resultado = self.servidor.EjecutarGCode(self.session_id, comando)
            if resultado['exito']:
                print(f"✓ {resultado['mensaje']} - Comando: {comando}")
                return True
            else:
                print(f"✗ {resultado['mensaje']}")
                return False
        except Exception as e:
            print(f"✗ Error: {e}")
            return False
    
    def reporte_usuario(self):
        """Obtener reporte de actividad del usuario"""
        try:
            resultado = self.servidor.ReporteUsuario(self.session_id)
            if resultado['exito']:
                print(f"\n=== REPORTE DE ACTIVIDAD ===")
                print(f"  Usuario: {resultado['usuario']}")
                print(f"  Comandos ejecutados: {resultado['comandosEjecutados']}")
                print(f"  Comandos erróneos: {resultado['comandosErroneos']}")
                print(f"  Tiempo conexión: {resultado['tiempoConexion'].strip()}")
                print(f"  Estado robot: {resultado['estadoRobot']}")
                print(f"  Posición actual: {resultado['posicionActual']}")
                return True
            else:
                print(f"✗ {resultado['mensaje']}")
                return False
        except Exception as e:
            print(f"✗ Error: {e}")
            return False
    
    def configurar_modo(self, modo_trabajo, modo_coordenadas):
        """Configurar modo de trabajo del robot"""
        try:
            resultado = self.servidor.ConfigurarModo(self.session_id, modo_trabajo, modo_coordenadas)
            if resultado['exito']:
                print(f"✓ {resultado['mensaje']}")
                return True
            else:
                print(f"✗ {resultado['mensaje']}")
                return False
        except Exception as e:
            print(f"✗ Error: {e}")
            return False
    
    def ir_a_origen(self):
        """Mover robot a posición de origen"""
        try:
            resultado = self.servidor.IrAOrigen(self.session_id)
            if resultado['exito']:
                print(f"✓ {resultado['mensaje']}")
                return True
            else:
                print(f"✗ {resultado['mensaje']}")
                return False
        except Exception as e:
            print(f"✗ Error: {e}")
            return False
    
    def subir_archivo(self, nombre, contenido):
        """Subir archivo G-Code"""
        try:
            resultado = self.servidor.SubirGCode(self.session_id, nombre, contenido)
            if resultado['exito']:
                print(f"✓ {resultado['mensaje']} - Archivo: {resultado['archivo']}")
                return resultado['archivo']
            else:
                print(f"✗ {resultado['mensaje']}")
                return None
        except Exception as e:
            print(f"✗ Error: {e}")
            return None

def test_usuario_normal():
    """Test completo como usuario normal"""
    print("\n" + "="*50)
    print("TESTING COMO USUARIO NORMAL")
    print("="*50)
    
    client = ClienteRobotRPC()
    
    # Login como usuario normal
    if not client.login('user', 'user123'):
        return False
    
    # Listar comandos disponibles
    client.listar_comandos()
    
    # Configurar modo manual
    print(f"\n--- Configurando modo de trabajo ---")
    client.configurar_modo('manual', 'absoluto')
    
    # Movimientos del robot
    print(f"\n--- Probando movimientos ---")
    client.mover_robot(10, 20, 5, 100)
    client.mover_robot(0, 0, 10)
    
    # Comandos G-Code
    print(f"\n--- Ejecutando comandos G-Code ---")
    client.ejecutar_gcode('G28')  # Home
    client.ejecutar_gcode('G1 X50 Y30 Z15 F1500')
    
    # Ir a origen
    print(f"\n--- Volviendo a origen ---")
    client.ir_a_origen()
    
    # Subir y ejecutar archivo
    print(f"\n--- Gestion de archivos ---")
    contenido_gcode = """G28
G1 X10 Y10 Z5 F1000
G1 X20 Y20 Z5 F1000
G1 X0 Y0 Z0 F1000"""
    
    archivo = client.subir_archivo('test_trayectoria', contenido_gcode)
    if archivo:
        try:
            resultado = client.servidor.EjecutarArchivo(client.session_id, archivo)
            if resultado['exito']:
                print(f"✓ {resultado['mensaje']}")
            else:
                print(f"✗ {resultado['mensaje']}")
        except Exception as e:
            print(f"✗ Error ejecutando archivo: {e}")
    
    # Reporte final
    print(f"\n--- Reporte de actividad ---")
    client.reporte_usuario()
    
    return True

def test_admin():
    """Test completo como administrador"""
    print("\n" + "="*50)
    print("TESTING COMO ADMINISTRADOR")
    print("="*50)
    
    client = ClienteRobotRPC()
    
    # Login como admin
    if not client.login('admin', 'admin123'):
        return False
    
    # Listar comandos (debería incluir comandos admin)
    client.listar_comandos()
    
    # Conectar robot
    print(f"\n--- Gestión de conexión robot ---")
    client.conectar_robot('conectar')
    
    # Configurar acceso remoto
    print(f"\n--- Configurando acceso remoto ---")
    try:
        resultado = client.servidor.ConfigurarAccesoRemoto(client.session_id, True)
        if resultado['exito']:
            print(f"✓ {resultado['mensaje']}")
        else:
            print(f"✗ {resultado['mensaje']}")
    except Exception as e:
        print(f"✗ Error: {e}")
    
    # Control de motores
    print(f"\n--- Control de motores ---")
    try:
        resultado = client.servidor.ControlMotores(client.session_id, 'activar')
        if resultado['exito']:
            print(f"✓ {resultado['mensaje']}")
        else:
            print(f"✗ {resultado['mensaje']}")
    except Exception as e:
        print(f"✗ Error: {e}")
    
    # Reporte administrativo
    print(f"\n--- Reporte administrativo ---")
    try:
        resultado = client.servidor.ReporteAdmin(client.session_id, '', '')
        if resultado['exito']:
            print(f"✓ Reporte admin - Sesiones activas: {resultado['totalSesiones']}")
            for i, sesion in enumerate(resultado['sesiones']):
                print(f"  Sesión {i+1}: {sesion['usuario']} desde {sesion['nodo']}")
        else:
            print(f"✗ {resultado['mensaje']}")
    except Exception as e:
        print(f"✗ Error: {e}")
    
    return True

def main():
    print("=== CLIENTE DE PRUEBA SERVIDOR XML-RPC ROBOT ===")
    print("Conectando al servidor en localhost:8080...")
    
    # Esperar un momento para que el servidor esté listo
    time.sleep(1)
    
    try:
        # Test como usuario normal
        if test_usuario_normal():
            print("\n✓ Test usuario normal completado")
        
        # Test como administrador  
        if test_admin():
            print("\n✓ Test administrador completado")
        
        print(f"\n=== TODOS LOS TESTS COMPLETADOS ===")
        print("El servidor XML-RPC está funcionando correctamente!")
        
    except KeyboardInterrupt:
        print("\n\nTest interrumpido por el usuario")
    except Exception as e:
        print(f"\nError general: {e}")

if __name__ == '__main__':
    main()
