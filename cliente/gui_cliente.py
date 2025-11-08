#!/usr/bin/env python3
"""
Cliente GUI (Tkinter) para el control del Robot RRR.
"""

import tkinter as tk
from tkinter import ttk  # Usamos los widgets "tematizados" (se ven mejor)
from tkinter import messagebox, filedialog, scrolledtext
import sys
import os

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
        self.geometry("500x550") 
        self.resizable(False, False)

        # 1. Instanciamos nuestro cliente RPC
        try:
            self.cliente = ClienteRobotRPC('localhost', 8080)
        except SystemExit as e:
            messagebox.showerror("Error de Conexión", 
                                 "No se pudo conectar al servidor C++.\n"
                                 "Asegúrate de que el servidor esté en línea.")
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
        self._crear_widgets_control() # Esta función AHORA SÍ encontrará los callbacks

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
                self.original_stdout = sys.__stdout__ # Guardamos stdout original

            def write(self, message):
                # Escribimos también a la terminal original
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
        
        # --- Frame de Conexión (Admin) ---
        self.frame_admin = ttk.LabelFrame(self.frame_control, text="Conexión y Admin")
        self.frame_admin.pack(fill=tk.X, padx=10, pady=10)
        
        self.btn_conectar = ttk.Button(self.frame_admin, text="Conectar Robot", 
                                       command=self._conectar_robot)
        self.btn_conectar.pack(side=tk.LEFT, padx=5, pady=5, expand=True)
        
        self.btn_home = ttk.Button(self.frame_admin, text="Ir a Origen (Home)", 
                                   command=self._ir_a_origen)
        self.btn_home.pack(side=tk.LEFT, padx=5, pady=5, expand=True)

        # --- Frame de Movimiento Manual ---
        frame_mov = ttk.LabelFrame(self.frame_control, text="Movimiento Manual (X, Y, Z)")
        frame_mov.pack(fill=tk.X, padx=10, pady=5)

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
        
        self.btn_mover = ttk.Button(frame_mov, text="Mover a Posición", 
                                    command=self._mover_robot)
        self.btn_mover.grid(row=1, column=0, columnspan=6, pady=10, sticky="ew")

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

    # --- Lógica de la Aplicación (Callbacks de botones) ---

    def _intentar_login(self, event=None):
        """Callback del botón 'Ingresar'."""
        usuario = self.entry_usuario.get()
        clave = self.entry_clave.get()
        
        if not usuario or not clave:
            messagebox.showwarning("Login", "Usuario y clave no pueden estar vacíos.")
            return

        if self.cliente.login(usuario, clave):
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
            self.cliente.ir_a_origen()
        else:
            self.status_var.set("✗ Error: No ha iniciado sesión.")

    def _mover_robot(self):
        """Callback del botón 'Mover a Posición'."""
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
        
        self.cliente.mover_robot(x, y, z)

    def _control_efector(self, accion):
        """Callback para botones de efector."""
        if self.cliente.esta_conectado():
            self.cliente.control_efector(accion)
        else:
            self.status_var.set("✗ Error: No ha iniciado sesión.")

    def _control_motores(self, accion):
        """Callback para botones de motores."""
        if self.cliente.esta_conectado():
            self.cliente.control_motores(accion)
        else:
            self.status_var.set("✗ Error: No ha iniciado sesión.")

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
        """Callback para 'Ejecutar Archivo'."""
        if not self.cliente.esta_conectado():
            self.status_var.set("✗ Error: No ha iniciado sesión.")
            return
            
        nombre_servidor = self.entry_archivo_servidor.get()
        if not nombre_servidor:
            messagebox.showwarning("Error de Entrada", "Debe ingresar un nombre de archivo para ejecutar.")
            return
        
        self.cliente.ejecutar_archivo(nombre_servidor)

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
        """Muestra el reporte admin en una ventana nueva."""
        if not self.cliente.esta_conectado():
            self.status_var.set("✗ Error: No ha iniciado sesión.")
            return
        if self.cliente.tipo_usuario != 'admin':
            messagebox.showerror("Acceso Denegado", "Solo los administradores pueden ver este reporte.")
            return

        win_reporte = tk.Toplevel(self)
        win_reporte.title("Reporte Administrativo")
        win_reporte.geometry("450x300")
        
        txt_reporte = scrolledtext.ScrolledText(win_reporte, wrap=tk.WORD, width=60, height=15)
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
        
        self.cliente.reporte_admin()
        
        sys.stdout = old_stdout

    def _configurar_modo(self):
        """Callback para 'Configurar Modo'."""
        if not self.cliente.esta_conectado():
            self.status_var.set("✗ Error: No ha iniciado sesión.")
            return
        
        # (Esto se podría hacer con un Toplevel y radiobuttons,
        #  pero por ahora usamos un prompt simple)
        
        # (Esta es una forma simple de crear un popup, pero no es ideal)
        print("TODO: Implementar popup de Configurar Modo.")
        print("Configurando a 'manual' y 'absoluto' por defecto.")
        self.cliente.configurar_modo('manual', 'absoluto')


if __name__ == "__main__":
    app = App()
    app.mainloop()