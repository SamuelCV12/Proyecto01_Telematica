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
docker run -p 8080:8080 telemetria-server
```
