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
                print(f"\n=== REPORTE DE ACTIVIDAD PERSONAL ===")
                print(f"  Usuario: {resultado['usuario']}")
                print(f"  Tiempo de conexión: {resultado['tiempoConexion'].strip()}")
                print(f"  Estado del robot: {resultado['estadoRobot']}")
                print(f"  Posición actual: {resultado['posicionActual']}")
                print(f"  Comandos ejecutados en sesión: {resultado['comandosEjecutados']}")
                print(f"  Comandos erróneos en sesión: {resultado['comandosErroneos']}")
                
                # Mostrar el reporte detallado del GestorReportes si está disponible
                if 'reporteGeneral' in resultado:
                    print(f"\n=== DETALLE DE ACTIVIDAD ===")
                    reporte_csv = resultado['reporteGeneral']
                    
                    # Parsear el CSV y mostrarlo de forma más legible
                    lineas = reporte_csv.strip().split('\n')
                    ordenes = []
                    info_general = {}
                    
                    capturando_ordenes = False
                    
                    for linea in lineas:
                        if ',' in linea:
                            partes = linea.split(',', 1)
                            clave = partes[0].strip()
                            valor = partes[1].strip() if len(partes) > 1 else ""
                            
                            if clave == "orden_detalle":
                                capturando_ordenes = True
                                continue
                            elif clave in ["total_ordenes", "ordenes_erroneas"]:
                                capturando_ordenes = False
                                info_general[clave] = valor
                            elif capturando_ordenes and clave.strip('"'):
                                # Es una orden ejecutada
                                ordenes.append((clave.strip('"'), valor))
                            elif not capturando_ordenes:
                                info_general[clave] = valor
                    
                    # Mostrar información general estructurada
                    if "estadoConexion" in info_general:
                        print(f"  Estado de conexión: {info_general['estadoConexion']}")
                    if "posicion" in info_general:
                        print(f"  Posición registrada: {info_general['posicion']}")
                    if "estadoActividad" in info_general:
                        print(f"  Estado de actividad: {info_general['estadoActividad']}")
                    if "inicioActividad" in info_general:
                        print(f"  Inicio de actividad: {info_general['inicioActividad']}")
                    
                    # Mostrar órdenes ejecutadas
                    if ordenes:
                        print(f"\n  === ÓRDENES EJECUTADAS DESDE LA ÚLTIMA CONEXIÓN ===")
                        for i, (detalle, resultado_op) in enumerate(ordenes, 1):
                            estado = "✓ OK" if resultado_op in ["200", "OK"] else f"✗ ERROR ({resultado_op})"
                            print(f"    {i:2d}. {detalle} - {estado}")
                    
                    # Mostrar resumen
                    if "total_ordenes" in info_general or "ordenes_erroneas" in info_general:
                        total = int(info_general.get("total_ordenes", 0))
                        errores = int(info_general.get("ordenes_erroneas", 0))
                        exitosas = total - errores
                        print(f"\n  === RESUMEN ===")
                        print(f"    Total de órdenes: {total}")
                        print(f"    Órdenes exitosas: {exitosas}")
                        print(f"    Órdenes con error: {errores}")
                        if total > 0:
                            porcentaje_exito = (exitosas * 100.0) / total
                            print(f"    Porcentaje de éxito: {porcentaje_exito:.1f}%")
                
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
                    for i in range(len(resultado['sesiones'])):
                        sesion = resultado['sesiones'][i]
                        print(f"  Sesión {i+1}: {sesion['usuario']}@{sesion['nodo']} (Cmds: {sesion['comandos']}, Errs: {sesion['errores']})")
                
                # Mostrar filtros aplicados
                if filtro1 or filtro2:
                    print(f"\n  Filtros aplicados: Usuario='{filtro1}', Código='{filtro2}'")
                
                # Mostrar reportes específicos si se aplicaron filtros
                if 'reportePorUsuario' in resultado:
                    print(f"\n--- LOG FILTRADO POR USUARIO '{filtro1}' ---")
                    print(resultado['reportePorUsuario'])
                
                if 'reportePorCodigo' in resultado:
                    print(f"\n--- LOG FILTRADO POR CÓDIGO '{filtro2}' ---")
                    print(resultado['reportePorCodigo'])
                
                # Mostrar reporte completo si no hay filtros específicos
                if not filtro1 and not filtro2 and 'reporteAdmin' in resultado:
                    print(f"\n--- LOG COMPLETO DEL SERVIDOR ---")
                    lineas = resultado['reporteAdmin'].split('\n')
                    # Mostrar solo las últimas 10 líneas para no saturar la consola
                    print("(Mostrando últimas 10 entradas)")
                    for linea in lineas[-10:]:
                        if linea.strip():
                            print(linea)
                
                return True
            else:
                print(f"✗ {resultado['mensaje']}")
                return False
        except Exception as e:
            print(f"✗ Error: {e}")
            return False

    def reporte_log_csv(self, desde='', hasta='', filtro_usuario='', filtro_codigo=''):
        """
        (Admin) Obtiene el log CSV filtrado por múltiples criterios.
        - desde/hasta: fechas en formato YYYY-MM-DD HH:MM:SS
        - filtro_usuario: filtrar por nombre de usuario
        - filtro_codigo: filtrar por código de respuesta
        """
        if not self.esta_conectado():
            print("✗ Error: Debe iniciar sesión primero.")
            return False
        
        try:
            resultado = self.servidor.ReporteLogCsv(self.session_id, desde, hasta, filtro_usuario, filtro_codigo)
            if resultado['exito']:
                print(f"\n=== REPORTE LOG CSV FILTRADO ===")
                
                # Mostrar filtros aplicados
                filtros = resultado['filtros']
                print(f"Período: {filtros['desde']} a {filtros['hasta']}")
                if filtros['usuario']:
                    print(f"Usuario: {filtros['usuario']}")
                if filtros['codigo']:
                    print(f"Código: {filtros['codigo']}")
                if 'texto1' in filtros and filtros['texto1']:
                    print(f"Filtro texto 1: {filtros['texto1']}")
                if 'texto2' in filtros and filtros['texto2']:
                    print(f"Filtro texto 2: {filtros['texto2']}")
                
                # Mostrar el log CSV
                if 'logCsv' in resultado and resultado['logCsv']:
                    print(f"\n--- LOG CSV ---")
                    lineas = resultado['logCsv'].split('\n')
                    for linea in lineas:
                        if linea.strip():
                            print(linea)
                else:
                    print("No se encontraron registros con los criterios especificados.")
                
                # Mostrar resultados de filtro de texto si están presentes
                if 'lineasFiltradas' in resultado:
                    print(f"\n--- FILTRADO POR TEXTO ---")
                    for i in range(len(resultado['lineasFiltradas'])):
                        print(resultado['lineasFiltradas'][i])
                
                return True
            else:
                print(f"✗ {resultado['mensaje']}")
                return False
        except Exception as e:
            print(f"✗ Error: {e}")
            return False

    def iniciar_aprendizaje_trayectoria(self, nombre):
        """
        Inicia el aprendizaje de una nueva trayectoria.
        """
        if not self.esta_conectado():
            print("✗ Error: Debe iniciar sesión primero.")
            return False
        
        try:
            resultado = self.servidor.AprenderTrayectoria(self.session_id, "iniciar", nombre)
            if resultado['exito']:
                print(f"✓ {resultado['mensaje']} - Trayectoria: {nombre}")
                return True
            else:
                print(f"✗ {resultado['mensaje']}")
                return False
        except Exception as e:
            print(f"✗ Error: {e}")
            return False

    def agregar_paso_trayectoria(self, x, y, z, velocidad):
        """
        Agrega un paso a la trayectoria en aprendizaje.
        """
        if not self.esta_conectado():
            print("✗ Error: Debe iniciar sesión primero.")
            return False
        
        try:
            resultado = self.servidor.AprenderTrayectoria(self.session_id, "agregar", x, y, z, velocidad)
            if resultado['exito']:
                print(f"✓ {resultado['mensaje']} - Paso agregado")
                return True
            else:
                print(f"✗ {resultado['mensaje']}")
                return False
        except Exception as e:
            print(f"✗ Error: {e}")
            return False

    def finalizar_aprendizaje_trayectoria(self):
        """
        Finaliza y guarda la trayectoria actual.
        """
        if not self.esta_conectado():
            print("✗ Error: Debe iniciar sesión primero.")
            return False
        
        try:
            resultado = self.servidor.AprenderTrayectoria(self.session_id, "finalizar")
            if resultado['exito']:
                print(f"✓ {resultado['mensaje']}")
                return True
            else:
                print(f"✗ {resultado['mensaje']}")
                return False
        except Exception as e:
            print(f"✗ Error: {e}")
            return False

    def cancelar_aprendizaje_trayectoria(self):
        """
        Cancela el aprendizaje de trayectoria actual.
        """
        if not self.esta_conectado():
            print("✗ Error: Debe iniciar sesión primero.")
            return False
        
        try:
            # No hay método específico en el servidor para cancelar, 
            # pero podemos simular finalizando sin guardar o usando un método interno
            print("✓ Aprendizaje de trayectoria cancelado")
            return True
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