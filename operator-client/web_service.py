"""Servicio HTTP liviano para consultar la telemetría desde un navegador."""

import json
import os
import sys
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse

sys.path.insert(0, os.path.dirname(__file__))
from protocol_client import (  # noqa: E402
    ErrorConexionServidor,
    ErrorProtocoloServidor,
    listar_nodos_activos,
    obtener_alertas,
    obtener_estado_sistema,
)


class Handler(BaseHTTPRequestHandler):
    def _respuesta(self, estado, datos):
        cuerpo = json.dumps(datos, ensure_ascii=False).encode("utf-8")
        self.send_response(estado)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(cuerpo)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(cuerpo)

    def do_GET(self):
        ruta = urlparse(self.path).path
        try:
            if ruta == "/health":
                self._respuesta(200, {"status": "ok"})
            elif ruta == "/api/status":
                self._respuesta(200, obtener_estado_sistema())
            elif ruta == "/api/nodes":
                self._respuesta(200, {"nodos": listar_nodos_activos()})
            elif ruta == "/api/alerts":
                self._respuesta(200, {"alertas": obtener_alertas()})
            else:
                self._respuesta(404, {"error": "Ruta no encontrada"})
        except (ErrorConexionServidor, ErrorProtocoloServidor) as error:
            self._respuesta(502, {"error": str(error)})

    def log_message(self, formato, *args):
        print(f"[web] {formato % args}")


if __name__ == "__main__":
    puerto = int(os.getenv("OPERATOR_WEB_PORT", "3000"))
    servidor = ThreadingHTTPServer(("0.0.0.0", puerto), Handler)
    print(f"Servicio web escuchando en http://0.0.0.0:{puerto}")
    try:
        servidor.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        servidor.server_close()
