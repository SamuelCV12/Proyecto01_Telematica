"""Cliente TCP para el protocolo de telemetría."""

import os
import socket

SERVER_HOST = os.getenv("TELEMETRY_SERVER_HOST", "geonodos.duckdns.org")
SERVER_PORT = int(os.getenv("TELEMETRY_SERVER_PORT", "9002"))
TIMEOUT_SEGUNDOS = float(os.getenv("TELEMETRY_TIMEOUT", "3"))


class ErrorConexionServidor(Exception):
    """No se pudo conectar o comunicar con el servidor."""


class ErrorProtocoloServidor(Exception):
    """El servidor devolvió una respuesta inválida."""


def enviar_comando(comando: str) -> str:
    """Envía una orden TCP y lee la respuesta completa hasta cerrar el socket."""
    if not comando or "\n" in comando or "\r" in comando:
        raise ValueError("El comando debe ser una línea no vacía")

    try:
        with socket.create_connection(
            (SERVER_HOST, SERVER_PORT), timeout=TIMEOUT_SEGUNDOS
        ) as conexion:
            conexion.sendall(f"{comando}\n".encode("utf-8"))
            conexion.settimeout(TIMEOUT_SEGUNDOS)
            fragmentos = []
            while True:
                fragmento = conexion.recv(4096)
                if not fragmento:
                    break
                fragmentos.append(fragmento)
            respuesta = b"".join(fragmentos).decode("utf-8").strip()
            if not respuesta:
                raise ErrorProtocoloServidor("El servidor devolvió una respuesta vacía")
            return respuesta
    except ErrorProtocoloServidor:
        raise
    except (socket.timeout, ConnectionRefusedError, OSError, UnicodeError) as error:
        raise ErrorConexionServidor(
            f"No se pudo comunicar con el servidor "
            f"({SERVER_HOST}:{SERVER_PORT}): {error}"
        ) from error


def _partes(respuesta: str, tipo: str, minimo: int) -> list[str]:
    partes = respuesta.split("|")
    if len(partes) < minimo or partes[0] != tipo:
        if partes and partes[0] == "ERROR":
            raise ErrorProtocoloServidor("|".join(partes[1:]) or "Error del servidor")
        raise ErrorProtocoloServidor(
            f"Respuesta inválida para {tipo}: {respuesta}"
        )
    return partes


def obtener_estado_nodo(node_id: str) -> dict:
    partes = _partes(enviar_comando(f"GET_STATUS|{node_id}"), "STATUS", 14)
    if any(partes[pos] != etiqueta for pos, etiqueta in
           [(2, "TEMP"), (4, "HUM"), (6, "CONSUMO"), (8, "VIBRACION"),
            (10, "ESTADO"), (12, "CONEXION")]):
        raise ErrorProtocoloServidor("Campos STATUS inválidos")
    try:
        return {
            "id": partes[1],
            "temperatura": float(partes[3]),
            "humedad": float(partes[5]),
            "consumo": float(partes[7]),
            "vibracion": float(partes[9]),
            "estado": partes[11],
            "conexion": partes[13],
        }
    except ValueError as error:
        raise ErrorProtocoloServidor("Valores STATUS inválidos") from error


def listar_nodos_activos() -> list[str]:
    partes = _partes(enviar_comando("LIST_NODES"), "NODES", 2)
    try:
        cantidad = int(partes[1])
    except ValueError as error:
        raise ErrorProtocoloServidor("Cantidad de nodos inválida") from error
    if cantidad < 0 or len(partes) != cantidad + 2:
        raise ErrorProtocoloServidor("Respuesta NODES incompleta")
    return partes[2:]


def obtener_alertas() -> list[dict]:
    partes = _partes(enviar_comando("GET_ALERTS"), "ALERTS", 2)
    try:
        cantidad = int(partes[1])
    except ValueError as error:
        raise ErrorProtocoloServidor("Cantidad de alertas inválida") from error
    if cantidad < 0 or len(partes) != cantidad + 2:
        raise ErrorProtocoloServidor("Respuesta ALERTS incompleta")

    alertas = []
    for item in partes[2:]:
        campos = item.split(":")
        if len(campos) != 4:
            raise ErrorProtocoloServidor(f"Alerta inválida: {item}")
        try:
            alertas.append({
                "nodo_id": campos[0],
                "variable": campos[1],
                "valor": float(campos[2]),
                "timestamp": int(campos[3]),
            })
        except ValueError as error:
            raise ErrorProtocoloServidor(f"Valores de alerta inválidos: {item}") from error
    return alertas


def obtener_estado_sistema() -> dict:
    partes = _partes(enviar_comando("GET_SYSTEM_STATUS"), "SYSTEM_STATUS", 9)
    etiquetas = ["NODOS_TOTAL", "NODOS_ACTIVOS", "ALERTAS_TOTAL", "UPTIME_SEG"]
    if any(partes[pos] != etiqueta for pos, etiqueta in
           [(1, etiquetas[0]), (3, etiquetas[1]), (5, etiquetas[2]), (7, etiquetas[3])]):
        raise ErrorProtocoloServidor("Campos SYSTEM_STATUS inválidos")
    try:
        return {
            "nodos_total": int(partes[2]),
            "nodos_activos": int(partes[4]),
            "alertas_total": int(partes[6]),
            "uptime_segundos": int(partes[8]),
        }
    except ValueError as error:
        raise ErrorProtocoloServidor("Valores SYSTEM_STATUS inválidos") from error


if __name__ == "__main__":
    print("Estado del sistema:", obtener_estado_sistema())
    print("Nodos activos:", listar_nodos_activos())
    print("Alertas:", obtener_alertas())
