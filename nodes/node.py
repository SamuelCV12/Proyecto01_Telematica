"""Ciclo de ejecucion de un nodo individual de telemetria."""

from __future__ import annotations

import argparse
import logging
import threading
import time

try:
	from .config import CONFIG, NodeConfig
	from .network_client import NetworkClient
	from .protocol_builder import build_telemetry
	from .sensor_simulator import SensorSimulator
	from .statistics import NodeStatistics
except ImportError:
	from config import CONFIG, NodeConfig
	from network_client import NetworkClient
	from protocol_builder import build_telemetry
	from sensor_simulator import SensorSimulator
	from statistics import NodeStatistics


LOGGER = logging.getLogger(__name__)
MEASUREMENT_VARIABLES = ("TEMP", "HUM", "CONSUMO", "VIBRACION")


class Node:
	"""Genera y transmite mediciones hasta que se solicita su detencion."""

	def __init__(self, node_id: str, config: NodeConfig = CONFIG) -> None:
		if not node_id.startswith("NODE") or not node_id[4:].isdigit():
			raise ValueError("node_id debe tener formato NODE seguido de digitos")
		self.node_id = node_id
		self.config = config
		self.stop_event = threading.Event()
		self.sensor = SensorSimulator(config.anomaly_probability)
		self.network = NetworkClient(
			config.server_hostname, config.server_udp_port, config.socket_timeout
		)
		self.sequence = 0
		self.statistics = NodeStatistics(node_id)

	def stop(self) -> None:
		"""Solicita una parada; el ciclo no queda bloqueado durante el intervalo."""
		self.stop_event.set()

	def force_anomaly(self, variable: str) -> None:
		self.sensor.force_anomaly(variable)

	def run(self) -> None:
		"""Ejecuta el ciclo principal del nodo y captura errores de red."""
		LOGGER.info("[%s] iniciado", self.node_id)
		try:
			while not self.stop_event.is_set():
				self._send_measurements()
				self.stop_event.wait(self.config.telemetry_interval)
		finally:
			self._log_final_statistics()

	def _log_final_statistics(self) -> None:
		stats = self.statistics.snapshot()
		LOGGER.info(
			"[%s] detenido en SEQ=%d: intentados=%s transmitidos=%s "
			"fallos_locales=%s perdida_local=%.2f%% perdida_red=%s",
			self.node_id,
			self.sequence,
			stats["messages_attempted"],
			stats["messages_transmitted"],
			stats["send_failures"],
			stats["local_loss_percentage"],
			stats["network_loss_percentage"],
		)

	def _send_measurements(self) -> None:
		for variable in MEASUREMENT_VARIABLES:
			if self.stop_event.is_set():
				return
			value = self.sensor.generate_reading(variable)
			self.sequence += 1
			message = build_telemetry(self.node_id, variable, value, self.sequence)
			self.statistics.messages_attempted += 1
			delivered = self._send_with_retries(message)
			if delivered:
				self.statistics.messages_transmitted += 1
			else:
				self.statistics.send_failures += 1
			status = "enviado" if delivered else "no enviado"
			LOGGER.info(
				"[%s] %s %s=%.2f %s",
				self.node_id,
				status,
				variable,
				value,
				message,
			)

	def _send_with_retries(self, message: str) -> bool:
		for attempt in range(1, self.config.retry_attempts + 1):
			if self.network.send_datagram(message):
				return True
			LOGGER.warning(
				"[%s] intento UDP %d/%d fallido",
				self.node_id,
				attempt,
				self.config.retry_attempts,
			)
			if attempt < self.config.retry_attempts:
				self.stop_event.wait(self.config.retry_backoff)
		return False


def _arguments() -> argparse.Namespace:
	parser = argparse.ArgumentParser(description="Ejecuta un nodo IoT")
	parser.add_argument("node_id", help="Identificador, por ejemplo NODE01")
	parser.add_argument(
		"--force-anomaly",
		choices=("TEMP", "HUM", "CONSUMO", "VIBRACION"),
		help="Fuerza la proxima medicion de la variable indicada",
	)
	return parser.parse_args()


def main() -> None:
	logging.basicConfig(
		level=logging.INFO,
		format="%(asctime)s %(levelname)s %(message)s",
	)
	arguments = _arguments()
	node = Node(arguments.node_id)
	if arguments.force_anomaly:
		node.force_anomaly(arguments.force_anomaly)
	try:
		node.run()
	except KeyboardInterrupt:
		node.stop()
		LOGGER.info("[%s] apagado por Ctrl+C", node.node_id)


if __name__ == "__main__":
	main()
