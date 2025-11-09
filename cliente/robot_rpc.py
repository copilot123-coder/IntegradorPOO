#!/usr/bin/env python3
"""
Librería cliente para el servidor XML-RPC del robot POO
"""

import xmlrpc.client
import sys

class ClienteRobotRPC:
    """
    Clase que encapsula la comunicación XML-RPC con el servidor
    de control del robot.
    """
    
    def __init__(self, host='localhost', puerto=8080):
        """
        Inicializa el proxy del servidor.
        """
        self.url = f'http://{host}:{puerto}'
        try:
            self.servidor = xmlrpc.client.ServerProxy(self.url)
            # Prueba una llamada simple para asegurar la conexión
            self.servidor.system.listMethods() 
        except Exception as e:
            print(f"✗ Error fatal: No se pudo conectar al servidor en {self.url}. {e}")
            print("  Asegúrate de que el servidor C++ esté ejecutándose.")
            sys.exit(1)
            
        self.session_id = None
        self.tipo_usuario = None
        print(f"✓ Proxy XML-RPC conectado a {self.url}")

    def login(self, usuario, clave, nodo='localhost'):
        """
        Autenticación en el servidor.
        Retorna True si fue exitoso, False en caso contrario.
        """
        try:
            resultado = self.servidor.Login(usuario, clave, nodo)
            if resultado['exito']:
                self.session_id = resultado['sessionId']
                self.tipo_usuario = resultado['tipoUsuario']
                print(f"✓ Login exitoso como {usuario} ({self.tipo_usuario})")
                return True
            else:
                print(f"✗ Error login: {resultado['mensaje']}")
                return False
        except Exception as e:
            print(f"✗ Error de conexión en Login: {e}")
            return False
    
    def esta_conectado(self):
        """
        Verifica si el cliente tiene una sesión activa.
        """
        return self.session_id is not None

    def listar_comandos(self):
        """
        Obtiene y muestra la lista de comandos disponibles
        según el tipo de usuario.
        """
        if not self.esta_conectado():
            print("✗ Error: Debe iniciar sesión primero.")
            return False
            
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
        """
        (Admin) Conecta o desconecta el robot.
        accion: 'conectar' o 'desconectar'
        """
        if not self.esta_conectado():
            print("✗ Error: Debe iniciar sesión primero.")
            return False
            
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
        """
        Mueve el robot a una posición específica.
        Si 'velocidad' no es None, la envía como parámetro.
        """
        if not self.esta_conectado():
            print("✗ Error: Debe iniciar sesión primero.")
            return False
            
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
        """
        Ejecuta un comando G-Code directo en el robot.
        """
        if not self.esta_conectado():
            print("✗ Error: Debe iniciar sesión primero.")
            return False
            
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
        """
        Obtiene el reporte de actividad del usuario actual.
        """
        if not self.esta_conectado():
            print("✗ Error: Debe iniciar sesión primero.")
            return False
            
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
        """
        Configura el modo de trabajo del robot.
        modo_trabajo: 'manual' o 'automatico'
        modo_coordenadas: 'absoluto' o 'relativo'
        """
        if not self.esta_conectado():
            print("✗ Error: Debe iniciar sesión primero.")
            return False
            
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
        """
        Mueve el robot a su posición de origen (Home).
        """
        if not self.esta_conectado():
            print("✗ Error: Debe iniciar sesión primero.")
            return False
            
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
        """
        Sube un archivo G-Code al servidor.
        'contenido' debe ser un string con todo el G-Code.
        """
        if not self.esta_conectado():
            print("✗ Error: Debe iniciar sesión primero.")
            return False
            
        try:
            # El servidor C++ espera el contenido como string
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

    def ejecutar_archivo(self, nombre_archivo):
        """
        Ejecuta un archivo G-Code previamente subido al servidor.
        """
        if not self.esta_conectado():
            print("✗ Error: Debe iniciar sesión primero.")
            return False
            
        try:
            resultado = self.servidor.EjecutarArchivo(self.session_id, nombre_archivo)
            if resultado['exito']:
                print(f"✓ {resultado['mensaje']}")
                return True
            else:
                print(f"✗ {resultado['mensaje']}")
                return False
        except Exception as e:
            print(f"✗ Error ejecutando archivo: {e}")
            return False

    def configurar_acceso_remoto(self, habilitar):
        """
        (Admin) Habilita o deshabilita el acceso remoto de otros usuarios.
        'habilitar' debe ser un booleano (True/False).
        """
        if not self.esta_conectado():
            print("✗ Error: Debe iniciar sesión primero.")
            return False
            
        try:
            resultado = self.servidor.ConfigurarAccesoRemoto(self.session_id, habilitar)
            if resultado['exito']:
                print(f"✓ {resultado['mensaje']}")
                return True
            else:
                print(f"✗ {resultado['mensaje']}")
                return False
        except Exception as e:
            print(f"✗ Error: {e}")
            return False

    def control_motores(self, accion):
        """
        Activa o desactiva los motores del robot.
        accion: 'activar' o 'desactivar'
        """
        if not self.esta_conectado():
            print("✗ Error: Debe iniciar sesión primero.")
            return False
            
        try:
            resultado = self.servidor.ControlMotores(self.session_id, accion)
            if resultado['exito']:
                print(f"✓ {resultado['mensaje']}")
                return True
            else:
                print(f"✗ {resultado['mensaje']}")
                return False
        except Exception as e:
            print(f"✗ Error: {e}")
            return False

    def control_efector(self, accion):
        """
        Activa o desactiva el efector final (gripper).
        accion: 'activar' o 'desactivar'
        """
        if not self.esta_conectado():
            print("✗ Error: Debe iniciar sesión primero.")
            return False
            
        try:
            resultado = self.servidor.ControlEfector(self.session_id, accion)
            if resultado['exito']:
                print(f"✓ {resultado['mensaje']}")
                return True
            else:
                print(f"✗ {resultado['mensaje']}")
                return False
        except Exception as e:
            print(f"✗ Error: {e}")
            return False

    def aprender_trayectoria(self, accion, nombre=None, pos=None):
        """
        Controla el aprendizaje de trayectorias.
        - accion: 'iniciar', 'agregar', 'finalizar'
        - nombre: (string) requerido para 'iniciar'
        - pos: (dict) {'x':, 'y':, 'z':, 'vel':} requerido para 'agregar'
        """
        if not self.esta_conectado():
            print("✗ Error: Debe iniciar sesión primero.")
            return False
        
        try:
            if accion == 'iniciar' and nombre:
                params = [self.session_id, 'iniciar', nombre]
                resultado = self.servidor.AprenderTrayectoria(*params)
            elif accion == 'agregar' and pos:
                params = [self.session_id, 'agregar', pos['x'], pos['y'], pos['z'], pos['vel']]
                resultado = self.servidor.AprenderTrayectoria(*params)
            elif accion == 'finalizar':
                params = [self.session_id, 'finalizar']
                resultado = self.servidor.AprenderTrayectoria(*params)
            else:
                print("✗ Error: Parámetros inválidos para 'aprender_trayectoria'")
                return False
                
            if resultado['exito']:
                print(f"✓ {resultado['mensaje']}")
                return True
            else:
                print(f"✗ {resultado['mensaje']}")
                return False
        except Exception as e:
            print(f"✗ Error: {e}")
            return False

    def reporte_admin(self, filtro1='', filtro2=''):
        """
        (Admin) Obtiene el reporte administrativo completo.
        """
        if not self.esta_conectado():
            print("✗ Error: Debe iniciar sesión primero.")
            return False
            
        try:
            resultado = self.servidor.ReporteAdmin(self.session_id, filtro1, filtro2)
            if resultado['exito']:
                print(f"\n=== REPORTE ADMINISTRATIVO ===")
                print(f"  Sesiones activas: {resultado['totalSesiones']}")
                if 'sesiones' in resultado:
                    for i, sesion in enumerate(resultado['sesiones']):
                        print(f"  Sesión {i+1}: {sesion['usuario']}@{sesion['nodo']} (Cmds: {sesion['comandos']}, Errs: {sesion['errores']})")
                return True
            else:
                print(f"✗ {resultado['mensaje']}")
                return False
        except Exception as e:
            print(f"✗ Error: {e}")
            return False

#
# Esta parte se deja fuera para que el archivo sea solo una librería.
# El código de 'main' y 'test_admin' iría en tu 'cliente_robot.py'
#
# if __name__ == '__main__':
#     print("Este archivo es una librería y no está pensado para ser ejecutado directamente.")
#     print("Ejecuta 'cliente_robot.py' en su lugar.")
#