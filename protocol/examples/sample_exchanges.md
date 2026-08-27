# Ejemplos de Flujo de Mensajes

### 1. Intercambio de Telemetría Normal

```text
Node -> Server:   TELEMETRY|NODE_01|1710000000|TEMP:24.5,HUM:60
Server -> Node:   ACK|SERVER|1710000001|STATUS:OK
```

### 2. Flujo de Alerta Crítica

```text
Node -> Server:   ALERT|NODE_02|1710000010|CRITICAL:PRESSURE_HIGH
Server -> Node:   ACK|SERVER|1710000011|STATUS:RECEIVED
Server -> Client: ALERT_FORWARD|NODE_02|1710000011|CRITICAL:PRESSURE_HIGH
```
