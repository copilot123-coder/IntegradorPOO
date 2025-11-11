#!/usr/bin/env python3
"""
Cliente principal (CLI) para el control del Robot RRR.
"""

import os  # Lo usaremos para verificar que existen los archivos a subir
from robot_rpc import ClienteRobotRPC

# --- Funciones Auxiliares para el Menú ---
# (Esto hace que el bucle 'main' sea mucho más limpio)

def _get_input_tipo(prompt, tipo_dato=str, valid_options=None):
    """
    Función auxiliar para pedir datos al usuario con validación.
    - 'prompt': Mensaje a mostrar.
    - 'tipo_dato': El tipo al que se debe convertir (float, int, str).
    - 'valid_options': Una lista de opciones válidas (ej: ['manual', 'auto']).
    """
    while True:
        user_input = input(prompt).strip()
        
        # 1. Validación de tipo
        try:
            value = tipo_dato(user_input)
        except ValueError:
            print(f"✗ Error: Valor inválido. Se esperaba un {tipo_dato.__name__}.")
            continue
            
        # 2. Validación de opciones
        if valid_options and value not in valid_options:
            print(f"✗ Error: Opción inválida. Opciones válidas: {valid_options}")
            continue
            
        return value

def _manejar_login(cliente):
    """
    Bucle para manejar el login inicial.
    """
    print(f"--- Cliente Robot RRR ---")
    print(f"Conectando a {cliente.url}...")
    
    while not cliente.esta_conectado():
        usuario = input("Usuario: ")
        clave = input("Clave: ")
        if not usuario or not clave:
            print("✗ Usuario y clave no pueden estar vacíos.")
            continue
        cliente.login(usuario, clave)
        if not cliente.esta_conectado():
            print("Intente nuevamente.")

def _manejar_config_modo(cliente):
    """Opción 3: Configurar modo"""
    print("\n--- Configurar Modo de Trabajo ---")
    modo_trabajo = _get_input_tipo(
        "Modo de trabajo (manual/automatico): ",
        str,
        ['manual', 'automatico']
    )
    modo_coord = _get_input_tipo(
        "Modo de coordenadas (absoluto/relativo): ",
        str,
        ['absoluto', 'relativo']
    )
    cliente.configurar_modo(modo_trabajo, modo_coord)

def _manejar_mover_robot(cliente):
    """Opción 4: Mover robot"""
    print("\n--- Mover Robot (Modo Manual) ---")
    x = _get_input_tipo("Coordenada X: ", float)
    y = _get_input_tipo("Coordenada Y: ", float)
    z = _get_input_tipo("Coordenada Z: ", float)
    
    vel_input = input("Velocidad (opcional, deje en blanco para usar la default): ")
    velocidad = None
    if vel_input:
        try:
            velocidad = float(vel_input)
        except ValueError:
            print("Velocidad inválida, se usará la default.")
            
    cliente.mover_robot(x, y, z, velocidad)

def _manejar_control_efector(cliente):
    """Opción 6: Activar/Desactivar efector"""
    print("\n--- Control Efector Final (Gripper) ---")
    accion = _get_input_tipo(
        "Acción (activar/desactivar): ",
        str,
        ['activar', 'desactivar']
    )
    cliente.control_efector(accion)

def _manejar_subir_archivo(cliente):
    """Opción 7: Subir archivo G-Code"""
    print("\n--- Subir Archivo G-Code ---")
    
    # Pedir el path local del archivo
    path_local = input("Ingrese la ruta completa al archivo .gcode local: ")
    
    # Verificar si el archivo existe
    if not os.path.exists(path_local):
        print(f"✗ Error: El archivo '{path_local}' no existe.")
        return
    
    # Pedir el nombre para guardarlo en el servidor
    nombre_servidor = input("Nombre para guardar en el servidor (ej: 'mi_trayectoria'): ")
    if not nombre_servidor:
        print("✗ Error: Debe proporcionar un nombre.")
        return
        
    # Leer el contenido del archivo
    try:
        with open(path_local, 'r') as f:
            contenido = f.read()
        
        print(f"✓ Archivo '{path_local}' leído, subiendo {len(contenido)} bytes...")
        cliente.subir_archivo(nombre_servidor, contenido)
        
    except Exception as e:
        print(f"✗ Error leyendo o subiendo el archivo: {e}")

def _manejar_ejecutar_archivo(cliente):
    """Opción 8: Ejecutar archivo G-Code en servidor"""
    print("\n--- Ejecutar Archivo G-Code (Modo Automático) ---")
    nombre_servidor = input("Nombre del archivo en el servidor a ejecutar: ")
    if not nombre_servidor:
        print("✗ Error: Debe proporcionar un nombre.")
        return
    cliente.ejecutar_archivo(nombre_servidor)

def _manejar_aprender_trayectoria(cliente):
    """Opción 9: Aprender trayectoria paso a paso"""
    print("\n--- Aprendizaje de Trayectoria (Modo Manual) ---")
    
    nombre_trayectoria = input("Nombre de la trayectoria: ")
    if not nombre_trayectoria:
        print("✗ Error: Debe proporcionar un nombre.")
        return
    
    if not cliente.iniciar_aprendizaje_trayectoria(nombre_trayectoria):
        return
    
    print("\n=== APRENDIZAJE INICIADO ===")
    print("Comandos disponibles:")
    print("  'paso' - Agregar paso con coordenadas actuales")
    print("  'mover X Y Z V' - Mover y agregar paso (ej: mover 100 200 150 1000)")
    print("  'finalizar' - Terminar y guardar trayectoria")
    print("  'cancelar' - Cancelar aprendizaje")
    
    while True:
        comando = input("\nComando > ").strip().lower()
        
        if comando == 'finalizar':
            if cliente.finalizar_aprendizaje_trayectoria():
                print("✓ Trayectoria guardada exitosamente.")
                break
            else:
                print("✗ Error guardando trayectoria.")
                
        elif comando == 'cancelar':
            if cliente.cancelar_aprendizaje_trayectoria():
                print("✓ Aprendizaje cancelado.")
                break
                
        elif comando.startswith('mover '):
            try:
                partes = comando.split()
                if len(partes) >= 4:
                    x = float(partes[1])
                    y = float(partes[2])
                    z = float(partes[3])
                    v = float(partes[4]) if len(partes) > 4 else 1000.0
                    
                    if cliente.agregar_paso_trayectoria(x, y, z, v):
                        print(f"✓ Paso agregado: X={x}, Y={y}, Z={z}, V={v}")
                    else:
                        print("✗ Error agregando paso.")
                else:
                    print("Uso: mover X Y Z [velocidad]")
            except ValueError:
                print("✗ Error: Coordenadas deben ser números.")
                
        elif comando == 'paso':
            x = _get_input_tipo("Coordenada X: ", float)
            y = _get_input_tipo("Coordenada Y: ", float)
            z = _get_input_tipo("Coordenada Z: ", float)
            v = _get_input_tipo("Velocidad (opcional, Enter para default): ", float)
            
            if cliente.agregar_paso_trayectoria(x, y, z, v if v else 1000):
                print(f"✓ Paso agregado: X={x}, Y={y}, Z={z}, V={v if v else 1000}")
            else:
                print("✗ Error agregando paso.")
                
        else:
            print("Comando no reconocido. Use: 'paso', 'mover X Y Z V', 'finalizar', 'cancelar'")

# --- Funciones de Administrador ---

def _manejar_conectar_robot(cliente):
    """Opción A1: Conectar/Desconectar Robot"""
    print("\n--- Conexión Robot/Simulador ---")
    accion = _get_input_tipo(
        "Acción (conectar/desconectar): ",
        str,
        ['conectar', 'desconectar']
    )
    cliente.conectar_robot(accion)

def _manejar_control_motores(cliente):
    """Opción A2: Habilitar/Deshabilitar Motores"""
    print("\n--- Control Motores ---")
    accion = _get_input_tipo(
        "Acción (activar/desactivar): ",
        str,
        ['activar', 'desactivar']
    )
    cliente.control_motores(accion)

def _manejar_reportes_admin(cliente):
    """Opción A3: Reportes administrativos avanzados"""
    print("\n--- Reportes Administrativos ---")
    print("1. Reporte general de sesiones")
    print("2. Reporte filtrado por usuario")
    print("3. Reporte filtrado por código de respuesta")
    print("4. Log CSV con filtros avanzados")
    
    tipo_reporte = _get_input_tipo(
        "Seleccione tipo de reporte (1-4): ",
        str,
        ['1', '2', '3', '4']
    )
    
    if tipo_reporte == '1':
        # Reporte general
        cliente.reporte_admin()
    elif tipo_reporte == '2':
        # Filtrado por usuario
        usuario = input("Ingrese nombre de usuario a filtrar: ")
        cliente.reporte_admin(usuario, '')
    elif tipo_reporte == '3':
        # Filtrado por código
        codigo = input("Ingrese código de respuesta a filtrar (ej: 200, ERROR): ")
        cliente.reporte_admin('', codigo)
    elif tipo_reporte == '4':
        # Log CSV con filtros avanzados
        print("\nFiltros disponibles (deje en blanco para omitir):")
        desde = input("Fecha desde (YYYY-MM-DD HH:MM:SS): ")
        hasta = input("Fecha hasta (YYYY-MM-DD HH:MM:SS): ")
        filtro_usuario = input("Usuario: ")
        filtro_codigo = input("Código de respuesta: ")
        filtro_texto1 = input("Filtro de texto 1: ")
        filtro_texto2 = input("Filtro de texto 2: ")
        
        cliente.reporte_log_csv(desde, hasta, filtro_usuario, filtro_codigo, filtro_texto1, filtro_texto2)

# --- Menú Principal ---

def mostrar_menu_principal(tipo_usuario):
    """Muestra el menú de opciones según el tipo de usuario"""
    print("\n" + "="*40)
    print("--- PANEL DE CONTROL ROBOT ---")
    print(" 1. Listar comandos disponibles")
    print(" 2. Ver mi reporte de actividad")
    print(" 3. Configurar modo (manual/auto, abs/rel)")
    print(" 4. Mover robot a posición (X, Y, Z)")
    print(" 5. Mover robot a Origen (Home)")
    print(" 6. Activar/Desactivar efector (Gripper)")
    print(" 7. Subir archivo G-Code local")
    print(" 8. Ejecutar archivo G-Code en servidor")
    print(" 9. Aprender trayectoria paso a paso")
    
    if tipo_usuario == 'admin':
        print("\n--- ZONA ADMINISTRADOR ---")
        print(" A1. Conectar/Desconectar robot")
        print(" A2. Habilitar/Deshabilitar motores")
        print(" A3. Reportes administrativos avanzados")
        
    print("\n S. Salir")
    print("="*40)
    return input("Seleccione una opción: ").strip().upper()

# --- Bucle Principal de la Aplicación ---

def main():
    """Función principal de la aplicación cliente"""
    
    # 1. Conectar al servidor (la librería maneja el error si no está)
    cliente = ClienteRobotRPC('localhost', 8080)
    
    # 2. Forzar Login
    _manejar_login(cliente)
        
    # 3. Bucle principal del Menú
    opcion = ''
    while opcion != 'S':
        opcion = mostrar_menu_principal(cliente.tipo_usuario)
        
        try:
            # Opciones de Usuario Normal
            if opcion == '1':
                cliente.listar_comandos()
            
            elif opcion == '2':
                cliente.reporte_usuario()
            
            elif opcion == '3':
                _manejar_config_modo(cliente)

            elif opcion == '4':
                _manejar_mover_robot(cliente)

            elif opcion == '5':
                cliente.ir_a_origen()
            
            elif opcion == '6':
                _manejar_control_efector(cliente)
                
            elif opcion == '7':
                _manejar_subir_archivo(cliente)
            
            elif opcion == '8':
                _manejar_ejecutar_archivo(cliente)
            
            elif opcion == '9':
                _manejar_aprender_trayectoria(cliente)
                
            # Opciones de Administrador
            elif opcion == 'A1' and cliente.tipo_usuario == 'admin':
                _manejar_conectar_robot(cliente)
            
            elif opcion == 'A2' and cliente.tipo_usuario == 'admin':
                _manejar_control_motores(cliente)
                
            elif opcion == 'A3' and cliente.tipo_usuario == 'admin':
                _manejar_reportes_admin(cliente)
                
            # Salida
            elif opcion == 'S':
                print("Desconectando... Adiós.")
            
            else:
                if opcion != 'S':
                    print("Opción no válida.")
                    
        except Exception as e:
            # Captura de error general para que la app no crashee
            print(f"!! Error inesperado en la operación: {e}")
            print("!! Si el error persiste, reinicie el cliente.")

if __name__ == '__main__':
    main()