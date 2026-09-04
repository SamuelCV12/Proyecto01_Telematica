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

## Validaciones

- El identificador debe tener el formato `NODE` seguido de uno o más dígitos.
- Las variables de telemetría permitidas son `TEMP`, `HUM`, `CONSUMO` y
  `VIBRACION`.
- `TEMP` acepta valores entre `-50` y `100`; `HUM`, entre `0` y `100`.
- `CONSUMO` y `VIBRACION` deben ser valores no negativos.
- Los códigos de alerta permitidos son `TEMP_HIGH`, `HUM_HIGH`,
  `CONSUMO_HIGH` y `VIBRACION_HIGH`.
- Los valores deben ser numéricos, finitos y no negativos para alertas.

## Códigos de Error

- `ERR_01`: Formato de mensaje inválido.
- `ERR_02`: Dispositivo no autorizado.
- `ERR_03`: Carga útil fuera de rango.
