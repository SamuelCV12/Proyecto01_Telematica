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

## Ejecución Rápida

```bash
docker-compose up --build
```
