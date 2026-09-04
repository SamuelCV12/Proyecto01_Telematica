"""Interfaz Tkinter del cliente operador."""

import os
import queue
import sys
import threading
import tkinter as tk
from datetime import datetime
from tkinter import ttk

sys.path.append(os.path.join(os.path.dirname(__file__), ".."))

from protocol_client import (  # noqa: E402
    ErrorConexionServidor,
    ErrorProtocoloServidor,
    listar_nodos_activos,
    obtener_alertas,
    obtener_estado_nodo,
    obtener_estado_sistema,
)

INTERVALO_REFRESCO_MS = 5000


class OperadorGUI:
    def __init__(self, ventana):
        self.ventana = ventana
        self.ventana.title("Cliente Operador - Plataforma de Telemetría")
        self.ventana.geometry("650x450")
        self.resultados = queue.Queue()
        self.actualizacion_en_curso = False

        self.label_conexion = tk.Label(
            ventana, text="Conectando...", fg="gray", anchor="w"
        )
        self.label_conexion.pack(fill="x", padx=8, pady=(8, 0))
        self.pestañas = ttk.Notebook(ventana)
        self.pestañas.pack(expand=True, fill="both", padx=8, pady=8)
        self._crear_pestaña_estado()
        self._crear_pestaña_nodos()
        self._crear_pestaña_alertas()
        self._procesar_resultados()
        self.refrescar_todo()

    def _crear_pestaña_estado(self):
        self.tab_estado = ttk.Frame(self.pestañas)
        self.pestañas.add(self.tab_estado, text="Estado")
        self.label_estado = tk.Label(
            self.tab_estado, text="Cargando...", font=("Courier", 13), justify="left"
        )
        self.label_estado.pack(padx=20, pady=20, anchor="w")

    def _crear_pestaña_nodos(self):
        self.tab_nodos = ttk.Frame(self.pestañas)
        self.pestañas.add(self.tab_nodos, text="Nodos")
        marco_lista = ttk.Frame(self.tab_nodos)
        marco_lista.pack(side="left", fill="y", padx=10, pady=10)
        tk.Label(marco_lista, text="Nodos activos:").pack(anchor="w")
        self.lista_nodos = tk.Listbox(marco_lista, width=20, height=15)
        self.lista_nodos.pack()
        self.lista_nodos.bind("<<ListboxSelect>>", self._on_seleccionar_nodo)
        marco_detalle = ttk.Frame(self.tab_nodos)
        marco_detalle.pack(side="left", fill="both", expand=True, padx=10, pady=10)
        tk.Label(marco_detalle, text="Detalle del nodo seleccionado:").pack(anchor="w")
        self.label_detalle_nodo = tk.Label(
            marco_detalle, text="Selecciona un nodo de la lista", justify="left",
            font=("Courier", 11), anchor="nw"
        )
        self.label_detalle_nodo.pack(anchor="w", pady=10)

    def _crear_pestaña_alertas(self):
        self.tab_alertas = ttk.Frame(self.pestañas)
        self.pestañas.add(self.tab_alertas, text="Alertas")
        columnas = ("nodo", "variable", "valor", "fecha")
        self.tabla_alertas = ttk.Treeview(
            self.tab_alertas, columns=columnas, show="headings", height=15
        )
        for col, titulo in zip(columnas, ["Nodo", "Variable", "Valor", "Fecha/Hora"]):
            self.tabla_alertas.heading(col, text=titulo)
        self.tabla_alertas.pack(fill="both", expand=True, padx=10, pady=10)

    def _consultar_actualizacion(self):
        try:
            datos = {
                "sistema": obtener_estado_sistema(),
                "nodos": listar_nodos_activos(),
                "alertas": obtener_alertas(),
            }
            self.resultados.put(("actualizacion", datos))
        except (ErrorConexionServidor, ErrorProtocoloServidor) as error:
            self.resultados.put(("error", str(error)))

    def _aplicar_actualizacion(self, datos):
        sistema = datos["sistema"]
        uptime = self._formatear_uptime(sistema["uptime_segundos"])
        self.label_estado.config(
            text=(
                f"Nodos registrados (histórico):  {sistema['nodos_total']}\n"
                f"Nodos activos ahora:            {sistema['nodos_activos']}\n"
                f"Alertas generadas (histórico):  {sistema['alertas_total']}\n"
                f"Tiempo activo del servidor:     {uptime}"
            )
        )
        seleccion = self.lista_nodos.curselection()
        nodo_seleccionado = self.lista_nodos.get(seleccion[0]) if seleccion else None
        self.lista_nodos.delete(0, tk.END)
        for nodo_id in datos["nodos"]:
            self.lista_nodos.insert(tk.END, nodo_id)
        if nodo_seleccionado in datos["nodos"]:
            indice = datos["nodos"].index(nodo_seleccionado)
            self.lista_nodos.selection_set(indice)
        self.tabla_alertas.delete(*self.tabla_alertas.get_children())
        for alerta in reversed(datos["alertas"]):
            fecha = datetime.fromtimestamp(alerta["timestamp"]).strftime("%Y-%m-%d %H:%M:%S")
            self.tabla_alertas.insert(
                "", tk.END,
                values=(alerta["nodo_id"], alerta["variable"], alerta["valor"], fecha),
            )

    def _consultar_detalle(self, nodo_id):
        try:
            self.resultados.put(("detalle", obtener_estado_nodo(nodo_id)))
        except (ErrorConexionServidor, ErrorProtocoloServidor) as error:
            self.resultados.put(("detalle_error", str(error)))

    def _on_seleccionar_nodo(self, _evento):
        seleccion = self.lista_nodos.curselection()
        if seleccion:
            nodo_id = self.lista_nodos.get(seleccion[0])
            self.label_detalle_nodo.config(text="Consultando...")
            threading.Thread(
                target=self._consultar_detalle, args=(nodo_id,), daemon=True
            ).start()

    def _procesar_resultados(self):
        try:
            while True:
                tipo, contenido = self.resultados.get_nowait()
                if tipo == "actualizacion":
                    self._aplicar_actualizacion(contenido)
                    self.label_conexion.config(
                        text=f"Conectado — última actualización: "
                        f"{datetime.now().strftime('%H:%M:%S')}", fg="green"
                    )
                    self.actualizacion_en_curso = False
                elif tipo == "error":
                    self.label_conexion.config(text=f"Error de conexión: {contenido}", fg="red")
                    self.actualizacion_en_curso = False
                elif tipo == "detalle":
                    datos = contenido
                    if "error" in datos:
                        self.label_detalle_nodo.config(text=f"Error: {datos['error']}")
                    else:
                        self.label_detalle_nodo.config(
                            text=(
                                f"ID:          {datos['id']}\n"
                                f"Temperatura: {datos['temperatura']} °C\n"
                                f"Humedad:     {datos['humedad']} %\n"
                                f"Consumo:     {datos['consumo']} kWh\n"
                                f"Vibración:   {datos['vibracion']} mm/s\n"
                                f"Estado:      {datos['estado']}\n"
                                f"Conexión:    {datos['conexion']}"
                            )
                        )
                elif tipo == "detalle_error":
                    self.label_detalle_nodo.config(text=f"Error de conexión: {contenido}")
        except queue.Empty:
            pass
        self.ventana.after(50, self._procesar_resultados)

    @staticmethod
    def _formatear_uptime(segundos):
        horas, resto = divmod(segundos, 3600)
        minutos, segs = divmod(resto, 60)
        return f"{horas}h {minutos}m {segs}s"

    def refrescar_todo(self):
        if not self.actualizacion_en_curso:
            self.actualizacion_en_curso = True
            threading.Thread(target=self._consultar_actualizacion, daemon=True).start()
        self.ventana.after(INTERVALO_REFRESCO_MS, self.refrescar_todo)


if __name__ == "__main__":
    root = tk.Tk()
    OperadorGUI(root)
    root.mainloop()
