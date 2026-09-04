# Cliente Operador

Cliente Python para consultar el servidor de telemetría por TCP. Incluye una
interfaz gráfica Tkinter y un servicio HTTP basado en la biblioteca estándar.

## Requisitos

- Python 3.9 o superior.
- Tkinter para ejecutar la interfaz gráfica.
- Acceso de red al dominio `geonodos.duckdns.org` por TCP `9002`.

## Configuración

El cliente usa por defecto:

- Host: `geonodos.duckdns.org`
- TCP: `9002`
- Timeout: 3 segundos

Se pueden cambiar sin editar código:

```bash
export TELEMETRY_SERVER_HOST=geonodos.duckdns.org
export TELEMETRY_SERVER_PORT=9002
export TELEMETRY_TIMEOUT=3
```

## Interfaz gráfica

Desde la raíz del repositorio:

```bash
python3 operator-client/gui/operator_gui.py
```

Las consultas se realizan en hilos secundarios para que la ventana no se
bloquee cuando el servidor no responda.

Para detenerla, cierra la ventana o presiona `Ctrl+C` desde la terminal.

## Servicio web

Iniciar el servicio:

```bash
python3 operator-client/web_service.py
```

Por defecto escucha en el puerto `3000`. Endpoints disponibles:

- `GET /health`
- `GET /api/status`
- `GET /api/nodes`
- `GET /api/alerts`

Ejemplos de prueba:

```bash
curl http://127.0.0.1:3000/health
curl http://127.0.0.1:3000/api/status
curl http://127.0.0.1:3000/api/nodes
curl http://127.0.0.1:3000/api/alerts
```

Si el backend no está disponible, los endpoints de consulta devuelven HTTP
`502` con un mensaje JSON explicando el error.

Para cambiar el puerto:

```bash
OPERATOR_WEB_PORT=3000 python3 operator-client/web_service.py
```
