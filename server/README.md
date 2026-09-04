# Servidor Backend (C)

Módulo backend del sistema de telemetría implementado en lenguaje C con sockets POSIX y concurrencia.

## Compilación y Ejecución Local

1. Compilar el proyecto:
   ```bash
   make
   ```

2. Ejecutar el servidor:
   ```bash
   ./build/server
   ```

3. Limpiar compilación:
   ```bash
   make clean
   ```

## Ejecución con Docker

```bash
docker build -t telemetria-server .
docker volume create telemetry-data
docker run --rm \
  -p 9001:9001/udp -p 9002:9002/tcp \
  -v telemetry-data:/app/data \
  telemetria-server
```

La base de datos SQLite se guarda en `/app/data/telemetria.db`. En Docker
Compose, el volumen `telemetry-data` conserva los nodos y alertas cuando el
contenedor se reinicia o se recrea.
