"""Configuracion del modulo de nodos IoT.

La configuracion se obtiene del entorno del proceso y, como respaldo, de
nodes/.env. No se requieren dependencias externas para cargar ese archivo.
"""

from __future__ import annotations

import os
from dataclasses import dataclass
from pathlib import Path


MODULE_DIR = Path(__file__).resolve().parent
ENV_FILE = MODULE_DIR / ".env"


SENSOR_RANGES = {
    "TEMP": (-10.0, 35.0),
    "HUM": (20.0, 75.0),
    "CONSUMO": (10.0, 70.0),
    "VIBRACION": (0.1, 2.4),
    "ESTADO": ("OK", "WARN", "FAIL"),
}

# Deben mantenerse alineados con UMBRAL_* en server/include/config.h.
ANOMALY_THRESHOLDS = {
    "TEMP": 40.0,
    "HUM": 80.0,
    "CONSUMO": 80.0,
    "VIBRACION": 2.8,
}


@dataclass(frozen=True)
class NodeConfig:
    """Valores de ejecucion usados por los nodos."""

    server_hostname: str = "geonodos.duckdns.org"
    server_udp_port: int = 9001
    server_tcp_port: int = 9002
    telemetry_interval: float = 5.0
    socket_timeout: float = 3.0
    anomaly_probability: float = 0.05
    retry_attempts: int = 3
    retry_backoff: float = 2.0


def _read_dotenv(path: Path) -> dict[str, str]:
    """Lee pares KEY=VALUE simples sin sustituir al entorno del proceso."""
    values: dict[str, str] = {}
    if not path.is_file():
        return values

    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        key = key.strip()
        value = value.strip().strip("\"'")
        if key:
            values[key] = value
    return values


def _value(name: str, dotenv: dict[str, str], default: str) -> str:
    return os.environ.get(name, dotenv.get(name, default))


def load_config(path: Path = ENV_FILE) -> NodeConfig:
    """Carga configuracion con prioridad entorno > nodes/.env > defaults."""
    dotenv = _read_dotenv(path)
    return NodeConfig(
        server_hostname=_value(
            "SERVER_HOSTNAME", dotenv, "geonodos.duckdns.org"
        ),
        server_udp_port=int(_value("SERVER_UDP_PORT", dotenv, "9001")),
        server_tcp_port=int(_value("SERVER_TCP_PORT", dotenv, "9002")),
        telemetry_interval=float(_value("TELEMETRY_INTERVAL", dotenv, "5")),
        socket_timeout=float(_value("SOCKET_TIMEOUT", dotenv, "3")),
        anomaly_probability=float(
            _value("ANOMALY_PROBABILITY", dotenv, "0.05")
        ),
        retry_attempts=int(_value("RETRY_ATTEMPTS", dotenv, "3")),
        retry_backoff=float(_value("RETRY_BACKOFF", dotenv, "2")),
    )


CONFIG = load_config()
