# Nodos IoT Simulados

Modulo Python para la simulacion de nodos de telemetria. Los nodos generan
mediciones graduales y las envian por UDP usando el protocolo de texto del
proyecto.

## Configuracion inicial

La configuracion se carga con esta prioridad:

1. Variables de entorno del proceso.
2. Archivo `nodes/.env` local.
3. Valores predeterminados definidos en `config.py`.

Copiar `.env.example` como `.env` para personalizar la ejecucion. El servidor
se identifica por hostname DNS (`geonodos.duckdns.org`), usa UDP `9001` para
telemetria y reserva TCP `9002` para el cliente operador.

El proyecto usa exclusivamente la biblioteca estandar de Python; no requiere
instalar dependencias desde `requirements.txt`.

Los rangos normales y umbrales de anomalia definidos en `config.py` reflejan
los valores encontrados en `server/include/config.h`. `ESTADO` se conserva
como variable discreta de simulacion, pero el servidor calcula su estado
interno a partir de las mediciones UDP.

## Ejecucion

Ejecutar un nodo individual desde la raiz:

```text
python -m nodes.node NODE01
```

Para forzar la siguiente medicion anomala:

```text
python -m nodes.node NODE01 --force-anomaly TEMP
```

Ejecutar cinco nodos concurrentes:

```text
python -m nodes.launcher --count 5
```

Con Docker Compose desde la raiz:

```text
docker compose up --build telemetry-server telemetry-nodes
```

El servicio usa `telemetry-server` como hostname DNS interno por defecto. Para
apuntarlo a otro servidor, define `SERVER_HOSTNAME` y los demas valores en un
archivo `.env` de la raiz. La cantidad se cambia con `NODE_COUNT`. En el
contenedor se usa el modo no interactivo y los nodos permanecen activos hasta
detener el servicio.

El launcher acepta `start NODE01`, `stop NODE01` y `stopall`. Cada nodo se
ejecuta en un hilo independiente; un fallo de red o la detencion de un nodo no
interrumpe a los demas. Cada mensaje `TELEMETRY` incluye una secuencia propia:

```text
TELEMETRY|NODE01|SEQ:1|TEMP|24.80
```

Al detener el launcher se generan `nodes/stats/telemetry_stats.csv` y
`nodes/stats/telemetry_summary.txt`. El CSV separa mensajes intentados,
transmitidos localmente, fallos de envio, ACK recibidos y saltos de secuencia.
Como el servidor no responde ACK UDP, `acknowledgements_received` permanece en
`0`, la columna `local_loss_percentage` calcula los fallos locales y
`network_loss_percentage` se reporta como `N/D`. Por tanto, la perdida real en
la red no puede determinarse desde los nodos. Los saltos de `SEQ` se detectan
en el servidor y quedan registrados en sus logs. Un nodo individual tambien
muestra este resumen en el log al detenerse con `Ctrl+C`.
