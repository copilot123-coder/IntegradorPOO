#!/usr/bin/env python3
"""
Cliente GUI (Tkinter) para el control del Robot RRR.
"""

import tkinter as tk
from tkinter import ttk  # Usamos los widgets "tematizados" (se ven mejor)
from tkinter import messagebox, filedialog, scrolledtext, simpledialog
import sys
import os
import math
import threading
import time

# Librerías para visualización 3D
try:
    import matplotlib.pyplot as plt
    from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
    from mpl_toolkits.mplot3d import Axes3D
    import numpy as np
    VISUALIZATION_AVAILABLE = True
except ImportError:
    print("Advertencia: matplotlib no disponible. Visualización 3D deshabilitada.")
    VISUALIZATION_AVAILABLE = False

# Librerías para sonido (opcional)
try:
    import pygame
    SOUND_AVAILABLE = True
except ImportError:
    print("Advertencia: pygame no disponible. Efectos de sonido deshabilitados.")
    SOUND_AVAILABLE = False

# Importamos la librería de comunicación que ya creamos y probamos
try:
    from robot_rpc import ClienteRobotRPC
except ImportError:
    print("Error: No se encontró el archivo 'robot_rpc.py'.")
    print("Asegúrate de que esté en la misma carpeta que 'gui_cliente.py'.")
    sys.exit(1)

class App(tk.Tk):
    """
    Clase principal de la aplicación GUI.
    Hereda de tk.Tk (la ventana principal).
    """

    def __init__(self):
        super().__init__()

        self.title("Panel de Control Robot RRR - Cliente POO")
        # Geometría ajustada para los nuevos botones
        self.geometry("600x600") 
        self.resizable(False, False)

        # Estado del modo de trabajo
        self.modo_actual = "manual"  # "manual" o "automatico"
        self.coord_actual = "absoluto"  # "absoluto" o "relativo"

        # --- VARIABLE PARA ACCESO REMOTO (AÑADIDA) ---
        self.acceso_remoto_var = tk.BooleanVar(value=True)
        # ---------------------------------------------

        # Variables para visualización del robot
        self.posicion_robot = {'x': 0.0, 'y': 0.0, 'z': 0.0}  # Posición actual del efector final
        self.efector_activo = False
        self.robot_conectado = False
        
        # Parámetros del robot (basados en la imagen)
        self.L1 = 150  # Longitud del primer eslabón (mm)
        self.L2 = 150  # Longitud del segundo eslabón (mm) 
        self.L3 = 100  # Longitud del tercer eslabón (mm)
        self.altura_base = 50  # Altura de la base (mm)
        
        # Límites del espacio de trabajo
        self.R_MIN = 50   # Radio mínimo alcanzable
        self.R_MAX = 400  # Radio máximo alcanzable  
        self.Z_MIN = -50  # Altura mínima
        self.Z_MAX = 350  # Altura máxima
        
        # Ventana de visualización
        self.ventana_visualizacion = None
        self.figura_3d = None
        self.canvas_3d = None
        
        # Inicializar pygame para sonidos si está disponible
        if SOUND_AVAILABLE:
            try:
                pygame.mixer.init()
                self._crear_sonidos()
            except Exception as e:
                print(f"Error inicializando pygame: {e}")
                # No podemos cambiar la variable global desde aquí

        # 1. Instanciamos nuestro cliente RPC
        try:
            # Usamos el constructor por defecto de robot_rpc.py
            self.cliente = ClienteRobotRPC() 
        except SystemExit as e:
            messagebox.showerror("Error Fatal", "No se pudo conectar al servidor. Verifique que esté ejecutándose.")
            sys.exit(1)

        # 2. Creamos los "contenedores" para las pantallas
        self.frame_login = ttk.Frame(self)
        self.frame_control = ttk.Frame(self)
        
        # 3. Creamos una barra de estado al final
        self.status_var = tk.StringVar(value="Por favor, inicie sesión.")
        self.status_label = ttk.Label(self, textvariable=self.status_var, relief=tk.SUNKEN, anchor=tk.W)
        self.status_label.pack(side=tk.BOTTOM, fill=tk.X, ipady=5)

        # 4. Construimos las interfaces
        self._crear_widgets_login()
        self._crear_widgets_control() 

        # 5. Redirigimos la salida de 'print'
        self._redirigir_stdout()

        # 6. Mostramos la pantalla de Login al inicio
        self.frame_login.pack(fill=tk.BOTH, expand=True, padx=20, pady=20)

    def _redirigir_stdout(self):
        """Redirige 'print()' a la barra de estado de la GUI."""
        class ConsolaGUI:
            def __init__(self, status_var, status_label):
                self.status_var = status_var
                self.status_label = status_label
                self.original_stdout = sys.__stdout__ 

            def write(self, message):
                self.original_stdout.write(message) 
                
                msg = message.strip()
                if msg:
                    self.status_var.set(msg)
                    if msg.startswith("✗") or msg.startswith("Error"):
                        self.status_label.config(foreground="red")
                    else:
                        self.status_label.config(foreground="black")

            def flush(self):
                self.original_stdout.flush()
        
        sys.stdout = ConsolaGUI(self.status_var, self.status_label)

    def _crear_widgets_login(self):
        """Construye la UI de la pantalla de login."""
        ttk.Label(self.frame_login, text="Inicio de Sesión", 
                  font=("Arial", 16, "bold")).pack(pady=10)
        
        ttk.Label(self.frame_login, text="Usuario:").pack(pady=5)
        self.entry_usuario = ttk.Entry(self.frame_login, width=30)
        self.entry_usuario.pack()
        self.entry_usuario.insert(0, "admin")

        ttk.Label(self.frame_login, text="Clave:").pack(pady=5)
        self.entry_clave = ttk.Entry(self.frame_login, show="*", width=30)
        self.entry_clave.pack()
        self.entry_clave.insert(0, "admin")

        ttk.Button(self.frame_login, text="Ingresar", 
                   command=self._intentar_login).pack(pady=20, ipadx=10, ipady=5)
        
        self.entry_clave.bind("<Return>", self._intentar_login)

    def _crear_widgets_control(self):
        """Construye la UI del panel de control principal."""
        
        # --- Frame de Conexión (Admin) - (MODIFICADO) ---
        self.frame_admin = ttk.LabelFrame(self.frame_control, text="Conexión y Admin")
        self.frame_admin.pack(fill=tk.X, padx=10, pady=10)
        
        # Frame para botones de conexión (Izquierda)
        frame_botones_conexion = ttk.Frame(self.frame_admin)
        frame_botones_conexion.pack(side=tk.LEFT, padx=5, pady=5, expand=True, fill=tk.X)

        self.btn_conectar = ttk.Button(frame_botones_conexion, text="Conectar Robot", 
                                       command=self._conectar_robot)
        self.btn_conectar.pack(side=tk.LEFT, padx=5, pady=5, expand=True)
        
        self.btn_home = ttk.Button(frame_botones_conexion, text="Ir a Origen (Home)", 
                                   command=self._ir_a_origen)
        self.btn_home.pack(side=tk.LEFT, padx=5, pady=5, expand=True)

        # --- Frame para Acceso Remoto (AÑADIDO) ---
        frame_acceso = ttk.LabelFrame(self.frame_admin, text="Acceso Remoto")
        frame_acceso.pack(side=tk.LEFT, padx=10, pady=5)
        
        self.radio_acceso_on = ttk.Radiobutton(frame_acceso, text="Habilitado", 
                                               variable=self.acceso_remoto_var, 
                                               value=True, 
                                               command=self._configurar_acceso_remoto)
        self.radio_acceso_on.pack(anchor=tk.W)
        
        self.radio_acceso_off = ttk.Radiobutton(frame_acceso, text="Deshabilitado", 
                                                variable=self.acceso_remoto_var, 
                                                value=False, 
                                                command=self._configurar_acceso_remoto)
        self.radio_acceso_off.pack(anchor=tk.W)
        # --- FIN DE LA MODIFICACIÓN ---

        # --- Frame de Movimiento Manual ---
        frame_mov = ttk.LabelFrame(self.frame_control, text="Movimiento Manual")
        frame_mov.pack(fill=tk.X, padx=10, pady=5)

        # Coordenadas
        ttk.Label(frame_mov, text="X:").grid(row=0, column=0, padx=5, pady=5)
        self.entry_x = ttk.Entry(frame_mov, width=7)
        self.entry_x.grid(row=0, column=1)
        self.entry_x.insert(0, "50.0")

        ttk.Label(frame_mov, text="Y:").grid(row=0, column=2, padx=5, pady=5)
        self.entry_y = ttk.Entry(frame_mov, width=7)
        self.entry_y.grid(row=0, column=3)
        self.entry_y.insert(0, "150.0")

        ttk.Label(frame_mov, text="Z:").grid(row=0, column=4, padx=5, pady=5)
        self.entry_z = ttk.Entry(frame_mov, width=7)
        self.entry_z.grid(row=0, column=5)
        self.entry_z.insert(0, "100.0")
        
        # Velocidad
        ttk.Label(frame_mov, text="Vel:").grid(row=0, column=6, padx=5, pady=5)
        self.entry_velocidad = ttk.Entry(frame_mov, width=7)
        self.entry_velocidad.grid(row=0, column=7)
        self.entry_velocidad.insert(0, "1000")
        
        # Botones de movimiento
        self.btn_mover_sin_vel = ttk.Button(frame_mov, text="Mover (vel. default)", 
                                           command=self._mover_robot_sin_velocidad)
        self.btn_mover_sin_vel.grid(row=1, column=0, columnspan=4, pady=5, sticky="ew", padx=(0,5))
        
        self.btn_mover_con_vel = ttk.Button(frame_mov, text="Mover con velocidad", 
                                           command=self._mover_robot_con_velocidad)
        self.btn_mover_con_vel.grid(row=1, column=4, columnspan=4, pady=5, sticky="ew", padx=(5,0))
        
        # --- Frame de Trayectorias ---
        frame_tray = ttk.LabelFrame(self.frame_control, text="Aprendizaje de Trayectorias")
        frame_tray.pack(fill=tk.X, padx=10, pady=5)

        # Nombre de trayectoria
        ttk.Label(frame_tray, text="Nombre:").grid(row=0, column=0, padx=5, pady=5)
        self.entry_nombre_tray = ttk.Entry(frame_tray, width=15)
        self.entry_nombre_tray.grid(row=0, column=1, padx=5, pady=5)
        self.entry_nombre_tray.insert(0, "mi_trayectoria")
        
        # Botones de trayectoria
        self.btn_iniciar_tray = ttk.Button(frame_tray, text="Iniciar Aprendizaje", 
                                          command=self._iniciar_trayectoria)
        self.btn_iniciar_tray.grid(row=0, column=2, padx=5, pady=5)
        
        # Info label para aprendizaje
        self.lbl_info_aprendizaje = ttk.Label(frame_tray, text="", foreground="blue")
        self.lbl_info_aprendizaje.grid(row=0, column=3, padx=5, pady=5)
        
        self.btn_finalizar_tray = ttk.Button(frame_tray, text="Finalizar y Guardar", 
                                            command=self._finalizar_trayectoria)
        self.btn_finalizar_tray.grid(row=1, column=0, columnspan=2, pady=5, sticky="ew", padx=(0,5))
        self.btn_finalizar_tray.config(state=tk.DISABLED)
        
        self.btn_cancelar_tray = ttk.Button(frame_tray, text="Cancelar", 
                                           command=self._cancelar_trayectoria)
        self.btn_cancelar_tray.grid(row=1, column=2, columnspan=2, pady=5, sticky="ew", padx=(5,0))
        self.btn_cancelar_tray.config(state=tk.DISABLED)

        # --- Frame de Efector y Motores ---
        frame_efector = ttk.LabelFrame(self.frame_control, text="Controles Adicionales")
        frame_efector.pack(fill=tk.X, padx=10, pady=5)

        ttk.Button(frame_efector, text="Activar Efector", 
                   command=lambda: self._control_efector("activar")).pack(side=tk.LEFT, padx=5, pady=5, expand=True)
        ttk.Button(frame_efector, text="Desactivar Efector", 
                   command=lambda: self._control_efector("desactivar")).pack(side=tk.LEFT, padx=5, pady=5, expand=True)
        
        self.btn_motores_activar = ttk.Button(frame_efector, text="Activar Motores", 
                                              command=lambda: self._control_motores("activar"))
        self.btn_motores_activar.pack(side=tk.LEFT, padx=5, pady=5, expand=True)
        
        self.btn_motores_desactivar = ttk.Button(frame_efector, text="Desactivar Motores", 
                                                 command=lambda: self._control_motores("desactivar"))
        self.btn_motores_desactivar.pack(side=tk.LEFT, padx=5, pady=5, expand=True)

        # --- Frame de Modo Automático ---
        frame_auto = ttk.LabelFrame(self.frame_control, text="Modo Automático (G-Code)")
        frame_auto.pack(fill=tk.X, padx=10, pady=5)

        ttk.Button(frame_auto, text="Subir Archivo G-Code", 
                   command=self._subir_archivo).pack(side=tk.LEFT, padx=5, pady=5, expand=True)
        
        self.entry_archivo_servidor = ttk.Entry(frame_auto, width=20)
        self.entry_archivo_servidor.pack(side=tk.LEFT, padx=5, pady=5)
        self.entry_archivo_servidor.insert(0, "nombre_en_servidor")

        ttk.Button(frame_auto, text="Ejecutar Archivo", 
                   command=self._ejecutar_archivo).pack(side=tk.LEFT, padx=5, pady=5, expand=True)

        # --- Frame de Reportes ---
        frame_reportes = ttk.LabelFrame(self.frame_control, text="Reportes y Configuración")
        frame_reportes.pack(fill=tk.X, padx=10, pady=5)

        ttk.Button(frame_reportes, text="Ver Mi Reporte", 
                   command=self._ver_reporte_usuario).pack(side=tk.LEFT, padx=5, pady=5, expand=True)
        
        self.btn_reporte_admin = ttk.Button(frame_reportes, text="Ver Reporte Admin", 
                                            command=self._ver_reporte_admin)
        self.btn_reporte_admin.pack(side=tk.LEFT, padx=5, pady=5, expand=True)
        
        ttk.Button(frame_reportes, text="Configurar Modo", 
                   command=self._configurar_modo).pack(side=tk.LEFT, padx=5, pady=5, expand=True)
        
        # Nueva fila para visualización 3D
        frame_visualizacion = ttk.Frame(frame_reportes)
        frame_visualizacion.pack(fill=tk.X, pady=(5,0))
        
        ttk.Button(frame_visualizacion, text="[3D] Visualización Robot", 
                   command=self._mostrar_visualizacion_3d).pack(side=tk.LEFT, padx=5, expand=True)

    # --- Lógica de la Aplicación (Callbacks de botones) ---

    def _intentar_login(self, event=None):
        """Callback del botón 'Ingresar'."""
        usuario = self.entry_usuario.get()
        clave = self.entry_clave.get()
        
        # --- NODO AHORA ES 'localhost' POR DEFECTO ---
        nodo = "localhost" 

        if not usuario or not clave:
            messagebox.showwarning("Login", "Usuario y clave no pueden estar vacíos.")
            return

        # Enviamos el 'nodo' (localhost) al servidor
        if self.cliente.login(usuario, clave, nodo): 
            self.status_var.set(f"✓ ¡Bienvenido {usuario}! Conectado.")
            self.frame_login.pack_forget()
            self.frame_control.pack(fill=tk.BOTH, expand=True)
            
            # Lógica de permisos
            if self.cliente.tipo_usuario != 'admin':
                print("Iniciando sesión como usuario 'normal'. Deshabilitando funciones de admin.")
                self.btn_conectar.config(state=tk.DISABLED)
                self.btn_motores_activar.config(state=tk.DISABLED)
                self.btn_motores_desactivar.config(state=tk.DISABLED)
                self.btn_reporte_admin.config(state=tk.DISABLED)
                # --- LÍNEAS AÑADIDAS ---
                self.radio_acceso_on.config(state=tk.DISABLED)
                self.radio_acceso_off.config(state=tk.DISABLED)
                # -----------------------

        else:
            messagebox.showerror("Login Fallido", self.status_var.get())

    def _conectar_robot(self):
        """Callback del botón 'Conectar Robot'."""
        if self.cliente.esta_conectado():
            self.cliente.conectar_robot("conectar")
        else:
            self.status_var.set("✗ Error: No ha iniciado sesión.")

    def _ir_a_origen(self):
        """Callback del botón 'Ir a Origen'."""
        if self.cliente.esta_conectado():
            if self.cliente.ir_a_origen():
                # Actualizar visualización a posición de origen (0,0,0)
                self._actualizar_posicion_desde_movimiento(0, 0, 0)
                # Reproducir sonido de arranque/home
                self._reproducir_sonido("arranque")
        else:
            self.status_var.set("✗ Error: No ha iniciado sesión.")

    def _mover_robot(self):
        """Callback del botón 'Mover a Posición' (método original - mantener compatibilidad)."""
        self._mover_robot_sin_velocidad()
    
    def _mover_robot_sin_velocidad(self):
        """Callback para mover robot sin especificar velocidad."""
        if not self.cliente.esta_conectado():
            self.status_var.set("✗ Error: No ha iniciado sesión.")
            return

        try:
            x = float(self.entry_x.get())
            y = float(self.entry_y.get())
            z = float(self.entry_z.get())
        except ValueError:
            messagebox.showerror("Error de Entrada", "Coordenadas X, Y, Z deben ser números.")
            return
        
        # Si estamos en modo aprendizaje, agregamos el paso
        if self.btn_finalizar_tray.cget('state') == tk.NORMAL:
             self.cliente.agregar_paso_aprendizaje_automatico(x, y, z, None)
        
        if self.cliente.mover_robot(x, y, z):
            # Actualizar visualización si el movimiento fue exitoso
            self._actualizar_posicion_desde_movimiento(x, y, z)
    
    def _mover_robot_con_velocidad(self):
        """Callback para mover robot especificando velocidad."""
        if not self.cliente.esta_conectado():
            self.status_var.set("✗ Error: No ha iniciado sesión.")
            return

        try:
            x = float(self.entry_x.get())
            y = float(self.entry_y.get())
            z = float(self.entry_z.get())
            velocidad = float(self.entry_velocidad.get())
        except ValueError:
            messagebox.showerror("Error de Entrada", "Coordenadas X, Y, Z y velocidad deben ser números.")
            return
        
        # Si estamos en modo aprendizaje, agregamos el paso
        if self.btn_finalizar_tray.cget('state') == tk.NORMAL:
             self.cliente.agregar_paso_aprendizaje_automatico(x, y, z, velocidad)
        
        if self.cliente.mover_robot(x, y, z, velocidad):
            # Actualizar visualización si el movimiento fue exitoso
            self._actualizar_posicion_desde_movimiento(x, y, z)

    def _control_efector(self, accion):
        """Callback para botones de efector."""
        if self.cliente.esta_conectado():
            if self.cliente.control_efector(accion):
                # Actualizar estado del efector en la visualización
                activo = (accion == "activar")
                self._actualizar_estado_efector(activo)
        else:
            self.status_var.set("✗ Error: No ha iniciado sesión.")

    def _control_motores(self, accion):
        """Callback para botones de motores."""
        if self.cliente.esta_conectado():
            self.cliente.control_motores(accion)
        else:
            self.status_var.set("✗ Error: No ha iniciado sesión.")
    
    # --- Métodos para Aprendizaje de Trayectorias ---
    
    def _iniciar_trayectoria(self):
        """Callback para iniciar el aprendizaje de una trayectoria."""
        if not self.cliente.esta_conectado():
            self.status_var.set("✗ Error: No ha iniciado sesión.")
            return
        
        nombre = self.entry_nombre_tray.get().strip()
        if not nombre:
            messagebox.showerror("Error", "Debe proporcionar un nombre para la trayectoria.")
            return
        
        if self.cliente.iniciar_aprendizaje_trayectoria(nombre):
            # Habilitar botones de aprendizaje
            self.btn_finalizar_tray.config(state=tk.NORMAL)
            self.btn_cancelar_tray.config(state=tk.NORMAL)
            self.btn_iniciar_tray.config(state=tk.DISABLED)
            self.entry_nombre_tray.config(state=tk.DISABLED)
            
            # Mostrar info de captura automática
            self.lbl_info_aprendizaje.config(text="🎯 Capturando automáticamente...")
            
            self.status_var.set(f"✓ Aprendizaje iniciado: {nombre}. Los comandos se capturan automáticamente.")
        else:
            messagebox.showerror("Error", "No se pudo iniciar el aprendizaje de trayectoria.")
    

    def _finalizar_trayectoria(self):
        """Callback para finalizar y guardar la trayectoria."""
        if not self.cliente.esta_conectado():
            return
        
        if self.cliente.finalizar_aprendizaje_trayectoria():
            self._resetear_controles_trayectoria()
            self.status_var.set("✓ Trayectoria guardada exitosamente")
            messagebox.showinfo("Éxito", "Trayectoria guardada correctamente.")
        else:
            messagebox.showerror("Error", "No se pudo guardar la trayectoria.")
    
    def _cancelar_trayectoria(self):
        """Callback para cancelar el aprendizaje de trayectoria."""
        if messagebox.askyesno("Confirmar", "¿Está seguro de cancelar el aprendizaje? Se perderán los pasos agregados."):
            if self.cliente.cancelar_aprendizaje_trayectoria():
                self._resetear_controles_trayectoria()
                self.status_var.set("Aprendizaje cancelado")
    
    def _resetear_controles_trayectoria(self):
        """Resetea los controles de trayectoria al estado inicial."""
        self.btn_finalizar_tray.config(state=tk.DISABLED)
        self.btn_cancelar_tray.config(state=tk.DISABLED)
        self.btn_iniciar_tray.config(state=tk.NORMAL)
        self.entry_nombre_tray.config(state=tk.NORMAL)
        self.lbl_info_aprendizaje.config(text="")

    def _subir_archivo(self):
        """Callback para 'Subir Archivo G-Code'."""
        if not self.cliente.esta_conectado():
            self.status_var.set("✗ Error: No ha iniciado sesión.")
            return
        
        path_local = filedialog.askopenfilename(
            title="Seleccionar archivo G-Code",
            filetypes=(("G-Code files", "*.gcode"), ("Text files", "*.txt"), ("All files", "*.*"))
        )
        
        if not path_local:
            self.status_var.set("Subida cancelada.")
            return
            
        nombre_base = os.path.basename(path_local).split('.')[0]
        
        try:
            with open(path_local, 'r') as f:
                contenido = f.read()
            
            print(f"Subiendo archivo {path_local}...")
            nombre_subido = self.cliente.subir_archivo(nombre_base, contenido)
            if nombre_subido:
                self.entry_archivo_servidor.delete(0, tk.END)
                self.entry_archivo_servidor.insert(0, nombre_subido)
                
        except Exception as e:
            messagebox.showerror("Error al Subir", f"No se pudo leer o subir el archivo: {e}")

    def _ejecutar_archivo(self):
        """Callback para 'Ejecutar Archivo' - Muestra lista de archivos según permisos."""
        if not self.cliente.esta_conectado():
            self.status_var.set("✗ Error: No ha iniciado sesión.")
            return
        
        # Mostrar ventana de selección de archivo
        self._mostrar_selector_archivos()
    
    def _mostrar_selector_archivos(self):
        """Muestra ventana para seleccionar archivos G-Code disponibles."""
        win_archivos = tk.Toplevel(self)
        win_archivos.title("Seleccionar Archivo G-Code")
        win_archivos.geometry("400x300")
        
        ttk.Label(win_archivos, text="Archivos G-Code Disponibles", 
                  font=("Arial", 12, "bold")).pack(pady=10)
        
        # Frame para la lista
        frame_lista = ttk.Frame(win_archivos)
        frame_lista.pack(padx=20, pady=10, fill=tk.BOTH, expand=True)
        
        # Listbox con scrollbar
        scrollbar = ttk.Scrollbar(frame_lista)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        listbox = tk.Listbox(frame_lista, yscrollcommand=scrollbar.set)
        listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.config(command=listbox.yview)
        
        # Poblar lista con archivos
        archivos_disponibles = self._obtener_archivos_disponibles()
        for archivo in archivos_disponibles:
            listbox.insert(tk.END, archivo)
        
        # Frame para entrada manual
        frame_manual = ttk.Frame(win_archivos)
        frame_manual.pack(padx=20, pady=5, fill=tk.X)
        
        ttk.Label(frame_manual, text="O ingrese nombre manualmente:").pack(anchor=tk.W)
        entry_manual = ttk.Entry(frame_manual, width=40)
        entry_manual.pack(fill=tk.X, pady=5)
        
        # Frame para botones
        frame_botones = ttk.Frame(win_archivos)
        frame_botones.pack(pady=15)
        
        def ejecutar_seleccionado():
            nombre_archivo = ""
            
            seleccion = listbox.curselection()
            if seleccion:
                nombre_archivo = listbox.get(seleccion[0])
            elif entry_manual.get().strip():
                nombre_archivo = entry_manual.get().strip()
            else:
                messagebox.showwarning("Selección Requerida", "Debe seleccionar un archivo o ingresar un nombre.")
                return
            
            win_archivos.destroy()
            
            if messagebox.askyesno("Confirmar Ejecución", f"¿Ejecutar archivo '{nombre_archivo}'?\n\nEsto iniciará el modo automático."):
                self.cliente.ejecutar_archivo(nombre_archivo)
        
        def ejecutar_con_doble_clic(event):
            ejecutar_seleccionado()
        
        listbox.bind('<Double-Button-1>', ejecutar_con_doble_clic)
        
        ttk.Button(frame_botones, text="Ejecutar", command=ejecutar_seleccionado).pack(side=tk.LEFT, padx=5)
        ttk.Button(frame_botones, text="Cancelar", command=win_archivos.destroy).pack(side=tk.LEFT, padx=5)
    
    def _obtener_archivos_disponibles(self):
        """Obtiene la lista de archivos G-Code disponibles desde el servidor."""
        try:
            resultado = self.cliente.servidor.ListarArchivos(self.cliente.session_id)
            if resultado['exito']:
                archivos = []
                if 'archivos' in resultado:
                    for i in range(resultado['totalArchivos']):
                        archivos.append(resultado['archivos'][i])
                return archivos
            else:
                messagebox.showerror("Error", f"No se pudieron obtener los archivos: {resultado['mensaje']}")
                return []
        except Exception as e:
            print(f"WARN: No se pudo llamar a ListarArchivos: {e}. Use entrada manual.")
            return ["(Use entrada manual)"]

    def _ver_reporte_usuario(self):
        """Muestra el reporte de usuario en una ventana nueva."""
        if not self.cliente.esta_conectado():
            self.status_var.set("✗ Error: No ha iniciado sesión.")
            return

        win_reporte = tk.Toplevel(self)
        win_reporte.title("Reporte de Usuario")
        win_reporte.geometry("400x300")
        
        txt_reporte = scrolledtext.ScrolledText(win_reporte, wrap=tk.WORD, width=50, height=15)
        txt_reporte.pack(padx=10, pady=10, fill=tk.BOTH, expand=True)
        txt_reporte.config(state=tk.DISABLED)

        class ConsolaReporte:
            def __init__(self, texto_widget):
                self.texto_widget = texto_widget
            def write(self, message):
                self.texto_widget.config(state=tk.NORMAL)
                self.texto_widget.insert(tk.END, message)
                self.texto_widget.config(state=tk.DISABLED)
            def flush(self):
                pass
        
        old_stdout = sys.stdout
        sys.stdout = ConsolaReporte(txt_reporte)
        
        self.cliente.reporte_usuario()
        
        sys.stdout = old_stdout

    def _ver_reporte_admin(self):
        """Muestra ventana de selección de reporte administrativo."""
        if not self.cliente.esta_conectado():
            self.status_var.set("✗ Error: No ha iniciado sesión.")
            return
        if self.cliente.tipo_usuario != 'admin':
            messagebox.showerror("Acceso Denegado", "Solo los administradores pueden ver este reporte.")
            return

        win_seleccion = tk.Toplevel(self)
        win_seleccion.title("Seleccionar Tipo de Reporte")
        win_seleccion.geometry("400x300")
        win_seleccion.resizable(False, False)
        
        ttk.Label(win_seleccion, text="Reportes Administrativos", 
                  font=("Arial", 14, "bold")).pack(pady=10)
        
        ttk.Button(win_seleccion, text="Reporte General de Sesiones", 
                   command=lambda: self._ejecutar_reporte_admin(win_seleccion, 'general')).pack(pady=5, padx=20, fill=tk.X)
        
        ttk.Button(win_seleccion, text="Reporte Filtrado por Usuario", 
                   command=lambda: self._ejecutar_reporte_admin(win_seleccion, 'usuario')).pack(pady=5, padx=20, fill=tk.X)
        
        ttk.Button(win_seleccion, text="Reporte Filtrado por Código", 
                   command=lambda: self._ejecutar_reporte_admin(win_seleccion, 'codigo')).pack(pady=5, padx=20, fill=tk.X)
        
        ttk.Button(win_seleccion, text="Log CSV con Filtros Avanzados", 
                   command=lambda: self._ejecutar_reporte_admin(win_seleccion, 'csv_avanzado')).pack(pady=5, padx=20, fill=tk.X)
        
        ttk.Button(win_seleccion, text="Cerrar", 
                   command=win_seleccion.destroy).pack(pady=20)

    def _ejecutar_reporte_admin(self, ventana_padre, tipo_reporte):
        """Ejecuta el reporte administrativo seleccionado."""
        ventana_padre.destroy()
        
        filtros = {}
        
        if tipo_reporte == 'usuario':
            usuario = tk.simpledialog.askstring("Filtro por Usuario", "Ingrese nombre de usuario:")
            if not usuario: return
            filtros['filtro1'] = usuario
            filtros['filtro2'] = ''
            
        elif tipo_reporte == 'codigo':
            codigo = tk.simpledialog.askstring("Filtro por Código", "Ingrese código de respuesta (ej: 200, ERROR):")
            if not codigo: return
            filtros['filtro1'] = ''
            filtros['filtro2'] = codigo
            
        elif tipo_reporte == 'csv_avanzado':
            self._mostrar_filtros_csv_avanzados()
            return
        else:
            filtros['filtro1'] = ''
            filtros['filtro2'] = ''
        
        self._mostrar_ventana_reporte(tipo_reporte, filtros)
    
    def _mostrar_filtros_csv_avanzados(self):
        """Muestra ventana para configurar filtros CSV avanzados."""
        win_filtros = tk.Toplevel(self)
        win_filtros.title("Filtros CSV Avanzados")
        win_filtros.geometry("450x350")
        
        ttk.Label(win_filtros, text="Configurar Filtros", 
                  font=("Arial", 12, "bold")).pack(pady=10)
        
        frame_campos = ttk.Frame(win_filtros)
        frame_campos.pack(padx=20, pady=10, fill=tk.X)
        
        ttk.Label(frame_campos, text="Fecha (desde) - formato: YYYY-MM-DD HH:MM:SS:").pack(anchor=tk.W)
        entry_fecha_desde = ttk.Entry(frame_campos, width=40)
        entry_fecha_desde.pack(pady=(0,10), fill=tk.X)
        entry_fecha_desde.insert(0, "2024-01-01 00:00:00")
        
        ttk.Label(frame_campos, text="Fecha (hasta) - formato: YYYY-MM-DD HH:MM:SS:").pack(anchor=tk.W)
        entry_fecha_hasta = ttk.Entry(frame_campos, width=40)
        entry_fecha_hasta.pack(pady=(0,10), fill=tk.X)
        entry_fecha_hasta.insert(0, "2025-12-31 23:59:59")
        
        ttk.Label(frame_campos, text="Usuario (opcional):").pack(anchor=tk.W)
        entry_usuario = ttk.Entry(frame_campos, width=40)
        entry_usuario.pack(pady=(0,10), fill=tk.X)
        
        ttk.Label(frame_campos, text="Código (opcional):").pack(anchor=tk.W)
        entry_codigo = ttk.Entry(frame_campos, width=40)
        entry_codigo.pack(pady=(0,10), fill=tk.X)
        
        frame_botones = ttk.Frame(win_filtros)
        frame_botones.pack(pady=20)
        
        def aplicar_filtros():
            filtros_csv = {
                'fecha_desde': entry_fecha_desde.get(),
                'fecha_hasta': entry_fecha_hasta.get(),
                'usuario': entry_usuario.get(),
                'codigo': entry_codigo.get()
            }
            win_filtros.destroy()
            self._mostrar_ventana_reporte_csv(filtros_csv)
        
        ttk.Button(frame_botones, text="Aplicar Filtros", command=aplicar_filtros).pack(side=tk.LEFT, padx=5)
        ttk.Button(frame_botones, text="Cancelar", command=win_filtros.destroy).pack(side=tk.LEFT, padx=5)

    def _mostrar_ventana_reporte(self, tipo_reporte, filtros):
        """Muestra la ventana con el reporte generado."""
        win_reporte = tk.Toplevel(self)
        win_reporte.title(f"Reporte Administrativo - {tipo_reporte}")
        win_reporte.geometry("600x400")
        
        txt_reporte = scrolledtext.ScrolledText(win_reporte, wrap=tk.WORD, width=70, height=20)
        txt_reporte.pack(padx=10, pady=10, fill=tk.BOTH, expand=True)
        txt_reporte.config(state=tk.DISABLED)

        class ConsolaReporte:
            def __init__(self, texto_widget):
                self.texto_widget = texto_widget
            def write(self, message):
                self.texto_widget.config(state=tk.NORMAL)
                self.texto_widget.insert(tk.END, message)
                self.texto_widget.config(state=tk.DISABLED)
                self.texto_widget.see(tk.END)
            def flush(self):
                pass
        
        old_stdout = sys.stdout
        sys.stdout = ConsolaReporte(txt_reporte)
        
        self.cliente.reporte_admin(filtros.get('filtro1', ''), filtros.get('filtro2', ''))
        
        sys.stdout = old_stdout

    def _mostrar_ventana_reporte_csv(self, filtros_csv):
        """Muestra la ventana con el reporte CSV filtrado."""
        win_reporte = tk.Toplevel(self)
        win_reporte.title("Reporte CSV con Filtros Avanzados")
        win_reporte.geometry("700x500")
        
        txt_reporte = scrolledtext.ScrolledText(win_reporte, wrap=tk.WORD, width=80, height=25)
        txt_reporte.pack(padx=10, pady=10, fill=tk.BOTH, expand=True)
        txt_reporte.config(state=tk.DISABLED)

        class ConsolaReporte:
            def __init__(self, texto_widget):
                self.texto_widget = texto_widget
            def write(self, message):
                self.texto_widget.config(state=tk.NORMAL)
                self.texto_widget.insert(tk.END, message)
                self.texto_widget.config(state=tk.DISABLED)
                self.texto_widget.see(tk.END)
            def flush(self):
                pass
        
        old_stdout = sys.stdout
        sys.stdout = ConsolaReporte(txt_reporte)
        
        try:
            self.cliente.reporte_log_csv(
                filtros_csv['fecha_desde'], 
                filtros_csv['fecha_hasta'],
                filtros_csv['usuario'],
                filtros_csv['codigo']
            )
        except AttributeError:
            print("Error: La función 'reporte_log_csv' no está implementada en robot_rpc.py")
        
        sys.stdout = old_stdout

    def _configurar_modo(self):
        """Callback para 'Configurar Modo' - Abre ventana de configuración completa."""
        if not self.cliente.esta_conectado():
            self.status_var.set("✗ Error: No ha iniciado sesión.")
            return
        
        win_modo = tk.Toplevel(self)
        win_modo.title("Configurar Modo de Trabajo")
        win_modo.geometry("450x350")
        win_modo.resizable(False, False)
        
        ttk.Label(win_modo, text="Configuración del Robot", 
                  font=("Arial", 14, "bold")).pack(pady=15)
        
        frame_trabajo = ttk.LabelFrame(win_modo, text="Modo de Trabajo")
        frame_trabajo.pack(padx=20, pady=10, fill=tk.X)
        
        var_trabajo = tk.StringVar(value="manual")
        ttk.Radiobutton(frame_trabajo, text="Manual - Control directo del robot", 
                       variable=var_trabajo, value="manual").pack(anchor=tk.W, padx=10, pady=5)
        ttk.Radiobutton(frame_trabajo, text="Automático - Ejecución de archivos G-Code", 
                       variable=var_trabajo, value="automatico").pack(anchor=tk.W, padx=10, pady=5)
        
        frame_coord = ttk.LabelFrame(win_modo, text="Modo de Coordenadas")
        frame_coord.pack(padx=20, pady=10, fill=tk.X)
        
        var_coord = tk.StringVar(value="absoluto")
        ttk.Radiobutton(frame_coord, text="Absoluto - Coordenadas respecto al origen", 
                       variable=var_coord, value="absoluto").pack(anchor=tk.W, padx=10, pady=5)
        ttk.Radiobutton(frame_coord, text="Relativo - Coordenadas respecto a posición actual", 
                       variable=var_coord, value="relativo").pack(anchor=tk.W, padx=10, pady=5)
        
        frame_botones = ttk.Frame(win_modo)
        frame_botones.pack(pady=20)
        
        def aplicar_configuracion():
            modo_trabajo = var_trabajo.get()
            modo_coord = var_coord.get()
            
            if self.cliente.configurar_modo(modo_trabajo, modo_coord):
                win_modo.destroy()
                self.status_var.set(f"✓ Modo configurado: {modo_trabajo}/{modo_coord}")
                self._actualizar_interfaz_segun_modo(modo_trabajo)
            else:
                messagebox.showerror("Error", "No se pudo configurar el modo")
        
        ttk.Button(frame_botones, text="Aplicar", command=aplicar_configuracion).pack(side=tk.LEFT, padx=5)
        ttk.Button(frame_botones, text="Cancelar", command=win_modo.destroy).pack(side=tk.LEFT, padx=5)
    
    def _actualizar_interfaz_segun_modo(self, modo_trabajo):
        """Actualiza la interfaz según el modo de trabajo seleccionado."""
        self.modo_actual = modo_trabajo
        
        if modo_trabajo == "manual":
            self._habilitar_controles_manuales(True)
            self._habilitar_controles_automaticos(False)
            self.status_var.set("Modo Manual - Control directo disponible")
        else:
            self._habilitar_controles_manuales(False)
            self._habilitar_controles_automaticos(True)
            self.status_var.set("Modo Automático - Solo ejecución de archivos G-Code")
    
    def _habilitar_controles_manuales(self, habilitar):
        """Habilita o deshabilita los controles del modo manual."""
        estado = tk.NORMAL if habilitar else tk.DISABLED
        
        self.entry_x.config(state=estado)
        self.entry_y.config(state=estado)
        self.entry_z.config(state=estado)
        self.entry_velocidad.config(state=estado)
        self.btn_mover_sin_vel.config(state=estado)
        self.btn_mover_con_vel.config(state=estado)
        self.btn_home.config(state=estado)
        
        self.entry_nombre_tray.config(state=estado)
        self.btn_iniciar_tray.config(state=estado)
        if not habilitar:
            self._resetear_controles_trayectoria()
    
    def _habilitar_controles_automaticos(self, habilitar):
        """Habilita o deshabilita los controles del modo automático."""
        estado = tk.NORMAL if habilitar else tk.DISABLED
        pass  # La funcionalidad de archivos está disponible en ambos modos

    # --- FUNCIÓN DE CALLBACK PARA ACCESO REMOTO (AÑADIDA) ---
    def _configurar_acceso_remoto(self):
        """Callback para los botones de radio de Acceso Remoto."""
        if not self.cliente.esta_conectado():
            self.status_var.set("✗ Error: No ha iniciado sesión.")
            return
        
        if self.cliente.tipo_usuario != 'admin':
            messagebox.showerror("Acceso Denegado", "Solo los administradores pueden cambiar esta configuración.")
            return

        try:
            habilitar = self.acceso_remoto_var.get()
            # La función en robot_rpc.py ya imprime el resultado en la barra de estado
            self.cliente.configurar_acceso_remoto(habilitar)
        except Exception as e:
            messagebox.showerror("Error", f"No se pudo cambiar la configuración: {e}")
    # --- FIN DE LA FUNCIÓN AÑADIDA ---

    # === MÉTODOS DE VISUALIZACIÓN 3D Y SONIDOS ===
    
    def _crear_sonidos(self):
        """Crea los efectos de sonido usando pygame."""
        global SOUND_AVAILABLE
        if not SOUND_AVAILABLE:
            return
        try:
            # Verificar que pygame esté funcionando
            pygame.mixer.get_init()
            print("✓ Efectos de sonido habilitados")
        except Exception as e:
            print(f"Error configurando sonidos: {e}")
            SOUND_AVAILABLE = False
    
    def _reproducir_sonido(self, tipo):
        """Reproduce un sonido específico."""
        if not SOUND_AVAILABLE:
            return
        
        # Evitar reproducir el mismo sonido repetidamente
        current_time = time.time()
        if hasattr(self, '_ultimo_sonido') and hasattr(self, '_tiempo_ultimo_sonido'):
            if (self._ultimo_sonido == tipo and 
                current_time - self._tiempo_ultimo_sonido < 1.0):  # 1 segundo de cooldown
                return
        
        self._ultimo_sonido = tipo
        self._tiempo_ultimo_sonido = current_time
        
        try:
            print(f"🔊 Reproduciendo: {tipo}")
            # Usar threading para no bloquear la GUI
            def play():
                if tipo == "arranque":
                    self._beep(440, 0.5)
                elif tipo == "parada":
                    self._beep(220, 0.3)
                elif tipo == "movimiento":
                    self._beep(330, 0.2)
                elif tipo == "efector_on":
                    self._beep(500, 0.3)
                elif tipo == "efector_off":
                    self._beep(300, 0.3)
                elif tipo == "alarma":
                    self._beep(1000, 0.8)
            
            threading.Thread(target=play, daemon=True).start()
        except Exception as e:
            print(f"Error reproduciendo sonido: {e}")
    
    def _beep(self, frecuencia, duracion):
        """Genera un beep usando pygame."""
        if not SOUND_AVAILABLE:
            return
        
        try:
            sample_rate = 22050
            frames = int(duracion * sample_rate)
            arr = np.sin(2 * np.pi * frecuencia * np.linspace(0, duracion, frames))
            arr = (arr * 32767).astype(np.int16)
            
            stereo_arr = np.zeros((frames, 2), dtype=np.int16)
            stereo_arr[:, 0] = arr  
            stereo_arr[:, 1] = arr
            
            sound = pygame.sndarray.make_sound(stereo_arr)
            sound.play()
            # No hacer sleep aquí para evitar bloquear la GUI
            pygame.time.wait(int(duracion * 1000))  # Usar pygame.time.wait en su lugar
        except Exception as e:
            print(f"Error generando beep: {e}")
    
    def _mostrar_visualizacion_3d(self):
        """Abre la ventana de visualización 3D del robot."""
        if not VISUALIZATION_AVAILABLE:
            messagebox.showwarning("Visualización no disponible", 
                                 "matplotlib no está instalado. Para habilitar la visualización 3D, instale:\npip install matplotlib numpy")
            return
        
        if self.ventana_visualizacion is None or not self.ventana_visualizacion.winfo_exists():
            self._crear_ventana_visualizacion()
        else:
            self.ventana_visualizacion.lift()
    
    def _crear_ventana_visualizacion(self):
        """Crea la ventana de visualización 3D."""
        self.ventana_visualizacion = tk.Toplevel(self)
        self.ventana_visualizacion.title("Visualización 3D - Robot RRR")
        self.ventana_visualizacion.geometry("800x600")
        
        # Crear figura matplotlib
        self.figura_3d = plt.Figure(figsize=(10, 8), dpi=80)
        self.ax_3d = self.figura_3d.add_subplot(111, projection='3d')
        
        # Configurar canvas
        self.canvas_3d = FigureCanvasTkAgg(self.figura_3d, self.ventana_visualizacion)
        self.canvas_3d.get_tk_widget().pack(side=tk.TOP, fill=tk.BOTH, expand=True)
        
        # Agregar toolbar de navegación
        toolbar = NavigationToolbar2Tk(self.canvas_3d, self.ventana_visualizacion)
        toolbar.update()
        
        # Panel de control de visualización
        frame_control = ttk.Frame(self.ventana_visualizacion)
        frame_control.pack(side=tk.BOTTOM, fill=tk.X, padx=5, pady=5)
        
        ttk.Button(frame_control, text="Actualizar Posición", 
                  command=self._actualizar_posicion_robot).pack(side=tk.LEFT, padx=5)
        ttk.Button(frame_control, text="Vista Frontal", 
                  command=lambda: self._cambiar_vista(0, 0)).pack(side=tk.LEFT, padx=5)
        ttk.Button(frame_control, text="Vista Superior", 
                  command=lambda: self._cambiar_vista(90, 0)).pack(side=tk.LEFT, padx=5)
        ttk.Button(frame_control, text="Vista Isométrica", 
                  command=lambda: self._cambiar_vista(30, 45)).pack(side=tk.LEFT, padx=5)
        
        # Configurar la visualización inicial
        self._configurar_visualizacion_3d()
        self._dibujar_robot_3d()
        
        # Actualizar automáticamente cada 2 segundos
        self._iniciar_actualizacion_automatica()
    
    def _configurar_visualizacion_3d(self):
        """Configura los ejes y límites de la visualización 3D."""
        self.ax_3d.clear()
        
        # Configurar límites basados en el espacio de trabajo
        limite = max(self.R_MAX, self.Z_MAX) * 1.1
        self.ax_3d.set_xlim([-limite, limite])
        self.ax_3d.set_ylim([-limite, limite])
        self.ax_3d.set_zlim([self.Z_MIN - 50, self.Z_MAX + 50])
        
        # Etiquetas y título
        self.ax_3d.set_xlabel('X (mm)')
        self.ax_3d.set_ylabel('Y (mm)')
        self.ax_3d.set_zlabel('Z (mm)')
        self.ax_3d.set_title('Robot RRR - Espacio de Trabajo y Posición Actual')
        
        # Dibujar espacio de trabajo
        self._dibujar_espacio_trabajo()
    
    def _dibujar_espacio_trabajo(self):
        """Dibuja el espacio de trabajo del robot."""
        # Dibujar cilindro del espacio de trabajo
        theta = np.linspace(0, 2*np.pi, 50)
        
        # Círculo exterior (R_MAX)
        x_max = self.R_MAX * np.cos(theta)
        y_max = self.R_MAX * np.sin(theta)
        z_max_inf = np.full_like(theta, self.Z_MIN)
        z_max_sup = np.full_like(theta, self.Z_MAX)
        
        # Círculo interior (R_MIN)
        x_min = self.R_MIN * np.cos(theta)
        y_min = self.R_MIN * np.sin(theta)
        z_min_inf = np.full_like(theta, self.Z_MIN)
        z_min_sup = np.full_like(theta, self.Z_MAX)
        
        # Dibujar límites
        self.ax_3d.plot(x_max, y_max, z_max_inf, 'b--', alpha=0.5, label='Límite exterior (inferior)')
        self.ax_3d.plot(x_max, y_max, z_max_sup, 'b--', alpha=0.5, label='Límite exterior (superior)')
        self.ax_3d.plot(x_min, y_min, z_min_inf, 'r--', alpha=0.5, label='Límite interior (inferior)')
        self.ax_3d.plot(x_min, y_min, z_min_sup, 'r--', alpha=0.5, label='Límite interior (superior)')
        
        # Líneas verticales de conexión (algunas muestras)
        for i in range(0, len(theta), 10):
            self.ax_3d.plot([x_max[i], x_max[i]], [y_max[i], y_max[i]], 
                          [self.Z_MIN, self.Z_MAX], 'b-', alpha=0.3)
            self.ax_3d.plot([x_min[i], x_min[i]], [y_min[i], y_min[i]], 
                          [self.Z_MIN, self.Z_MAX], 'r-', alpha=0.3)
    
    def _dibujar_robot_3d(self):
        """Dibuja el robot en la visualización 3D."""
        # Obtener posición actual
        x, y, z = self.posicion_robot['x'], self.posicion_robot['y'], self.posicion_robot['z']
        
        # Base del robot
        self.ax_3d.plot([0], [0], [0], 'ko', markersize=15, label='Base')
        
        # Primer eslabón (vertical)
        eslabon1_z = self.altura_base
        self.ax_3d.plot([0, 0], [0, 0], [0, eslabon1_z], 'g-', linewidth=8, label='Eslabón 1')
        
        # Calcular posiciones intermedias (cinemática aproximada)
        r = math.sqrt(x**2 + y**2)
        if r > 0:
            # Ángulos aproximados
            theta1 = math.atan2(y, x)  # Rotación base
            
            # Posición del segundo joint (aproximada)
            x2 = (r * 0.6) * math.cos(theta1)
            y2 = (r * 0.6) * math.sin(theta1)
            z2 = eslabon1_z + (z - eslabon1_z) * 0.6
            
            # Segundo eslabón
            self.ax_3d.plot([0, x2], [0, y2], [eslabon1_z, z2], 'b-', linewidth=6, label='Eslabón 2')
            
            # Tercer eslabón (al efector final)
            self.ax_3d.plot([x2, x], [y2, y], [z2, z], 'm-', linewidth=4, label='Eslabón 3')
            
            # Joints
            self.ax_3d.plot([0], [0], [eslabon1_z], 'ro', markersize=8)
            self.ax_3d.plot([x2], [y2], [z2], 'ro', markersize=8)
        
        # Efector final
        color_efector = 'red' if self.efector_activo else 'gray'
        marker_efector = 'o' if self.efector_activo else 's'
        self.ax_3d.plot([x], [y], [z], marker_efector, color=color_efector, 
                       markersize=12, label=f'Efector {"ACTIVO" if self.efector_activo else "inactivo"}')
        
        # Verificar si está fuera del espacio de trabajo
        r_actual = math.sqrt(x**2 + y**2)
        fuera_espacio = (r_actual < self.R_MIN or r_actual > self.R_MAX or 
                        z < self.Z_MIN or z > self.Z_MAX)
        
        if fuera_espacio:
            # Dibujar alarma visual (sin emojis para evitar problemas de font)
            mensaje_alarma = f"*** FUERA DE RANGO ***\nR={r_actual:.1f} (min:{self.R_MIN}, max:{self.R_MAX})\nZ={z:.1f} (min:{self.Z_MIN}, max:{self.Z_MAX})"
            self.ax_3d.text(x, y, z + 20, mensaje_alarma, color='red', 
                           fontsize=10, weight='bold', ha='center')
            # Reproducir alarma sonora solo una vez por posición
            pos_key = f"{x:.1f},{y:.1f},{z:.1f}"
            if not hasattr(self, '_ultima_alarma_pos') or self._ultima_alarma_pos != pos_key:
                self._ultima_alarma_pos = pos_key
                self._reproducir_sonido("alarma")
        else:
            # Resetear alarma si volvemos al espacio válido
            if hasattr(self, '_ultima_alarma_pos'):
                self._ultima_alarma_pos = None
        
        # Mostrar coordenadas
        self.ax_3d.text2D(0.02, 0.98, f'Posición: X={x:.1f}, Y={y:.1f}, Z={z:.1f}', 
                         transform=self.ax_3d.transAxes, verticalalignment='top',
                         bbox=dict(boxstyle="round,pad=0.3", facecolor="yellow", alpha=0.7))
        
        # Leyenda
        self.ax_3d.legend(loc='upper right')
    
    def _cambiar_vista(self, elevacion, azimut):
        """Cambia la vista 3D."""
        self.ax_3d.view_init(elev=elevacion, azim=azimut)
        self.canvas_3d.draw()
    
    def _actualizar_posicion_robot(self):
        """Actualiza la posición del robot desde el servidor."""
        if not self.cliente.esta_conectado():
            return
        
        try:
            # Obtener las coordenadas actuales de los entry widgets
            try:
                x = float(self.entry_x.get() or 0)
                y = float(self.entry_y.get() or 0) 
                z = float(self.entry_z.get() or 0)
                self._actualizar_posicion_desde_movimiento(x, y, z)
            except ValueError:
                pass  # Si no hay valores válidos, mantener posición actual
                
        except Exception as e:
            print(f"Error actualizando posición: {e}")
    
    def _iniciar_actualizacion_automatica(self):
        """Inicia la actualización automática de la visualización."""
        def actualizar():
            if (self.ventana_visualizacion and 
                self.ventana_visualizacion.winfo_exists() and 
                self.cliente.esta_conectado()):
                
                self._actualizar_posicion_robot()
                
                # Programar próxima actualización
                self.ventana_visualizacion.after(3000, actualizar)
        
        # Iniciar el ciclo
        if self.ventana_visualizacion:
            self.ventana_visualizacion.after(1000, actualizar)
    
    def _actualizar_posicion_desde_movimiento(self, x, y, z):
        """Actualiza la posición cuando se mueve el robot."""
        # Solo actualizar si la posición realmente cambió
        if (hasattr(self, 'posicion_robot') and 
            self.posicion_robot['x'] == float(x) and 
            self.posicion_robot['y'] == float(y) and 
            self.posicion_robot['z'] == float(z)):
            return  # No hay cambio, no hacer nada
            
        self.posicion_robot['x'] = float(x)
        self.posicion_robot['y'] = float(y) 
        self.posicion_robot['z'] = float(z)
        
        # Reproducir sonido de movimiento
        self._reproducir_sonido("movimiento")
        
        # Actualizar visualización si está abierta
        if (self.ventana_visualizacion and 
            self.ventana_visualizacion.winfo_exists()):
            self._redibujar_visualizacion()
    
    def _redibujar_visualizacion(self):
        """Redibuja solo la visualización sin reconfigurar todo."""
        if (self.ventana_visualizacion and 
            self.ventana_visualizacion.winfo_exists() and
            hasattr(self, 'ax_3d')):
            try:
                self._configurar_visualizacion_3d()
                self._dibujar_robot_3d()
                self.canvas_3d.draw()
            except Exception as e:
                print(f"Error redibujando visualización: {e}")
    
    def _actualizar_estado_efector(self, activo):
        """Actualiza el estado del efector final."""
        # Solo actualizar si el estado cambió
        if hasattr(self, 'efector_activo') and self.efector_activo == activo:
            return  # No hay cambio
            
        self.efector_activo = activo
        
        # Reproducir sonido
        if activo:
            self._reproducir_sonido("efector_on")
        else:
            self._reproducir_sonido("efector_off")
        
        # Actualizar visualización si está abierta
        if (self.ventana_visualizacion and 
            self.ventana_visualizacion.winfo_exists()):
            self._redibujar_visualizacion()


if __name__ == "__main__":
    app = App()
    app.mainloop()