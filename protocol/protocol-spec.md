# Especificación del Protocolo de Telemetría

## Formatos de Mensaje

Los mensajes se estructuran en tramas de texto delimitadas por `|` y terminadas
en salto de línea cuando se envían por TCP:

### Telemetría (UDP)

`TELEMETRY|NODE03|TEMP|24.8`

### Consulta de estado (TCP)

`GET_STATUS|NODE03`

### Alerta (UDP)

`ALERT|NODE03|TEMP_HIGH|42.1`

El tercer campo de una alerta es el código de alerta y el cuarto es el valor
medido. Las alertas se almacenan en el historial del servidor.

## Códigos de Error

- `ERR_01`: Formato de mensaje inválido.
- `ERR_02`: Dispositivo no autorizado.
- `ERR_03`: Carga útil fuera de rango.
