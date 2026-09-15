"""Construccion y validacion del protocolo de texto de telemetria."""

from __future__ import annotations

import math
import re


MAX_SERVER_PAYLOAD = 511
NODE_ID_PATTERN = re.compile(r"^NODE[0-9]+$")
TELEMETRY_VARIABLES = {"TEMP", "HUM", "CONSUMO", "VIBRACION"}
ALERT_CODES = {
	"TEMP_HIGH",
	"HUM_HIGH",
	"CONSUMO_HIGH",
	"VIBRACION_HIGH",
}


def _validate_node_id(node_id: str) -> None:
	if not NODE_ID_PATTERN.fullmatch(node_id):
		raise ValueError("El identificador debe tener formato NODE seguido de digitos")


def _validate_value(value: float) -> float:
	value = float(value)
	if not math.isfinite(value):
		raise ValueError("El valor debe ser numerico y finito")
	return value


def _fit_datagram(message: str) -> str:
	encoded = message.encode("utf-8")
	if len(encoded) > MAX_SERVER_PAYLOAD:
		raise ValueError("El mensaje supera el buffer UDP del servidor")
	return message


def build_telemetry(node_id: str, variable: str, value: float, sequence: int) -> str:
	"""Construye TELEMETRY|NODE|SEQ|VARIABLE|VALOR."""
	_validate_node_id(node_id)
	if not isinstance(sequence, int) or isinstance(sequence, bool) or sequence < 0:
		raise ValueError("La secuencia debe ser un entero no negativo")
	variable = variable.upper()
	if variable not in TELEMETRY_VARIABLES:
		raise ValueError(f"Variable TELEMETRY no permitida: {variable}")
	value = _validate_value(value)
	if variable == "TEMP" and not -50.0 <= value <= 100.0:
		raise ValueError("TEMP fuera del rango del protocolo")
	if variable == "HUM" and not 0.0 <= value <= 100.0:
		raise ValueError("HUM fuera del rango del protocolo")
	if variable in {"CONSUMO", "VIBRACION"} and value < 0.0:
		raise ValueError(f"{variable} no puede ser negativo")
	return _fit_datagram(
		f"TELEMETRY|{node_id}|SEQ:{sequence}|{variable}|{value:.2f}"
	)


def build_alert(node_id: str, code: str, value: float) -> str:
	"""Construye una alerta UDP valida para el servidor."""
	_validate_node_id(node_id)
	code = code.upper()
	if code not in ALERT_CODES:
		raise ValueError(f"Codigo ALERT no permitido: {code}")
	value = _validate_value(value)
	if value < 0.0:
		raise ValueError("El valor de ALERT no puede ser negativo")
	return _fit_datagram(f"ALERT|{node_id}|{code}|{value:.2f}")


def parse_message(message: str) -> tuple[str, list[str]]:
	"""Valida una trama recibida y devuelve su tipo y campos restantes."""
	if "\n" in message or "\r" in message:
		raise ValueError("El datagrama no debe contener saltos de linea")
	parts = message.split("|")
	if parts[0] == "TELEMETRY":
		if len(parts) != 5 or not parts[2].startswith("SEQ:"):
			raise ValueError("TELEMETRY debe incluir SEQ y cuatro campos utiles")
		_validate_node_id(parts[1])
		sequence = parts[2][4:]
		if not sequence.isdigit():
			raise ValueError("SEQ debe ser un entero no negativo")
		variable = parts[3]
		value = _validate_value(float(parts[4]))
		if variable not in TELEMETRY_VARIABLES:
			raise ValueError(f"Variable TELEMETRY no permitida: {variable}")
		if variable == "TEMP" and not -50.0 <= value <= 100.0:
			raise ValueError("TEMP fuera del rango del protocolo")
		if variable == "HUM" and not 0.0 <= value <= 100.0:
			raise ValueError("HUM fuera del rango del protocolo")
		if variable in {"CONSUMO", "VIBRACION"} and value < 0.0:
			raise ValueError(f"{variable} no puede ser negativo")
		return parts[0], parts[1:]
	if parts[0] == "ALERT" and len(parts) == 4:
		_validate_node_id(parts[1])
		if parts[2] not in ALERT_CODES:
			raise ValueError(f"Codigo ALERT no permitido: {parts[2]}")
		if _validate_value(float(parts[3])) < 0.0:
			raise ValueError("El valor de ALERT no puede ser negativo")
		return parts[0], parts[1:]
	if parts[0] not in {"TELEMETRY", "ALERT"}:
		raise ValueError("Mensaje con formato de protocolo invalido")
	raise ValueError("Mensaje con cantidad de campos invalida")
