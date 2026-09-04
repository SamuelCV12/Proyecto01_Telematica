# Ejemplos de Flujo de Mensajes

### 1. Telemetría normal (UDP)

```text
Node -> Server:   TELEMETRY|NODE03|TEMP|24.8
```

### 2. Consulta de estado (TCP)

```text
Operator -> Server: GET_STATUS|NODE03
Server -> Operator: STATUS|NODE03|TEMP|24.80|HUM|0.00|CONSUMO|0.00|VIBRACION|0.00|ESTADO|OK|CONEXION|ACTIVO
```

### 3. Alerta (UDP)

```text
Node -> Server:   ALERT|NODE03|TEMP_HIGH|42.1
```
