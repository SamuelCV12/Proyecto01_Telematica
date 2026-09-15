"""Simulacion gradual de las variables de un nodo IoT."""

from __future__ import annotations

import random
from typing import Union

try:
	from .config import ANOMALY_THRESHOLDS, SENSOR_RANGES
except ImportError:
	from config import ANOMALY_THRESHOLDS, SENSOR_RANGES


NumericReading = float
Reading = Union[NumericReading, str]


class SensorSimulator:
	"""Genera lecturas normales, graduales y anomalias bajo demanda."""

	def __init__(self, anomaly_probability: float = 0.05) -> None:
		if not 0.0 <= anomaly_probability <= 1.0:
			raise ValueError("anomaly_probability debe estar entre 0 y 1")
		self.anomaly_probability = anomaly_probability
		self._last_values: dict[str, float] = {}
		self._forced_anomalies: set[str] = set()

	def force_anomaly(self, variable: str) -> None:
		"""Hace que la siguiente lectura numerica supere el umbral indicado."""
		variable = variable.upper()
		if variable not in ANOMALY_THRESHOLDS:
			raise ValueError(f"Variable sin umbral de anomalia: {variable}")
		self._forced_anomalies.add(variable)

	def generate_reading(self, variable: str) -> Reading:
		"""Genera una lectura para una variable definida en la configuracion."""
		variable = variable.upper()
		if variable == "ESTADO":
			return random.choice(SENSOR_RANGES["ESTADO"])
		if variable not in ANOMALY_THRESHOLDS:
			raise ValueError(f"Variable desconocida: {variable}")

		if variable in self._forced_anomalies or random.random() < self.anomaly_probability:
			self._forced_anomalies.discard(variable)
			value = self._anomalous_value(variable)
		else:
			value = self._gradual_value(variable)

		self._last_values[variable] = value
		return value

	def generate_all(self) -> dict[str, Reading]:
		"""Genera las cinco variables simuladas."""
		return {
			variable: self.generate_reading(variable)
			for variable in ("TEMP", "HUM", "CONSUMO", "VIBRACION", "ESTADO")
		}

	def _gradual_value(self, variable: str) -> float:
		minimum, maximum = SENSOR_RANGES[variable]
		previous = self._last_values.get(variable, random.uniform(minimum, maximum))
		span = maximum - minimum
		value = previous + random.uniform(-span * 0.08, span * 0.08)
		return max(minimum, min(maximum, value))

	def _anomalous_value(self, variable: str) -> float:
		threshold = ANOMALY_THRESHOLDS[variable]
		minimum, maximum = SENSOR_RANGES[variable]
		protocol_maximum = 100.0 if variable in {"TEMP", "HUM"} else threshold * 1.5
		upper_bound = max(threshold + 0.1, min(protocol_maximum, threshold * 1.2))
		value = random.uniform(threshold, upper_bound)
		if variable == "TEMP":
			value = min(value, 100.0)
		elif variable == "HUM":
			value = min(value, 100.0)
		else:
			value = max(value, minimum)
		return value
