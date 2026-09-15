# Telemetría EAFIT

Proyecto de arquitectura cliente-servidor y nodos IoT para el monitoreo de telemetría y alertas.

## Estructura del Repositorio

- `server/`: Backend central en C (procesamiento de conexiones, lógica de telemetría y persistencia/redirección).
- `nodes/`: Nodos IoT simulados / emisores de telemetría.
- `operator-client/`: Cliente operador con interfaz gráfica y servicio web.
- `protocol/`: Especificación formal del protocolo de comunicación de telemetría.
- `wireshark/`: Capturas `.pcap` y análisis de tráfico de red.
- `docs/`: Diagramas de arquitectura y guías de despliegue en la nube (AWS/EC2).

## Requisitos Previos

- GCC / Clang y `make`
- Docker & Docker Compose
- Python 3 y Tkinter para el cliente operador

## Ejecución Rápida

```bash
docker-compose up --build
```

El servicio `telemetry-nodes` inicia cinco nodos IoT simulados por defecto y
envía telemetría UDP al servicio `telemetry-server`. La cantidad de nodos se
puede cambiar con `NODE_COUNT`:

```bash
NODE_COUNT=5 docker-compose up --build telemetry-server telemetry-nodes
```

Para ejecutar un nodo individual fuera de Docker:

```bash
python -m nodes.node NODE01
python -m nodes.node NODE01 --force-anomaly TEMP
```

La documentación específica está en `nodes/README.md`.
