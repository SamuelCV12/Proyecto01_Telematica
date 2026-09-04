# Guía de Despliegue (AWS EC2 / Cloud)

## 1. Configuración de Instancia EC2
- SO recomendado: Ubuntu Server 22.04 LTS o Amazon Linux 2023.
- Tipo de instancia: `t2.micro` o `t3.micro`.

## 2. Configuración de Security Groups (Puertos)
- `SSH`: Puerto 22 (restringido a tu IP).
- `Servidor Telemetría`: UDP `9001` y TCP `9002`.
- `Servicio Web / Operador`: Puerto TCP `3000`.

## 3. Configuración DNS
- Asociar registro `A` o `CNAME` apuntando a la IP pública elástica (Elastic IP) de la instancia.

## 4. Despliegue con Docker
```bash
git clone <URL_REPOSITORIO>
cd <DIRECTORIO_REPOSITORIO>
cp server/config/server.env.example server/config/server.env
docker-compose up -d --build
```

El servicio web queda disponible en `http://IP_PUBLICA:3000`. Para acceder a él
desde Internet, agrega TCP `3000` al Security Group. El contenedor web se
conecta internamente al servidor mediante `telemetry-server:9002`.
