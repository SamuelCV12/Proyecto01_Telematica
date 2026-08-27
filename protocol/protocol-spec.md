# Especificación del Protocolo de Telemetría

## Formato General de Mensaje

Los mensajes se estructuran en tramas de texto delimitadas:

`TIPO_MENSAJE|ORIGEN_ID|TIMESTAMP|CARGA_UTIL\n`

## Tipos de Mensajes

1. `TELEMETRY`: Envío periódico de métricas/sensores.
   - Ejemplo: `TELEMETRY|NODE_01|1710000000|TEMP:24.5,HUM:60`
2. `ALERT`: Señales de emergencia o umbrales superados.
   - Ejemplo: `ALERT|NODE_01|1710000005|CRITICAL:OVERHEAT`
3. `ACK`: Confirmación de recepción.
   - Ejemplo: `ACK|SERVER|1710000006|STATUS:OK`

## Códigos de Error

- `ERR_01`: Formato de mensaje inválido.
- `ERR_02`: Dispositivo no autorizado.
- `ERR_03`: Carga útil fuera de rango.
