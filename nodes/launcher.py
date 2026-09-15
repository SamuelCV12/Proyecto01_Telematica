"""Lanzador concurrente y control basico de multiples nodos."""

from __future__ import annotations

import argparse
import logging
import signal
import threading
from pathlib import Path

try:
	from .node import Node
	from .statistics import StatisticsRegistry
except ImportError:
	from node import Node
	from statistics import StatisticsRegistry


LOGGER = logging.getLogger(__name__)


class NodeLauncher:
	"""Administra un hilo independiente por nodo para aislar sus fallos."""

	def __init__(self, count: int) -> None:
		if count < 1:
			raise ValueError("La cantidad de nodos debe ser positiva")
		self.nodes = {f"NODE{index:02d}": Node(f"NODE{index:02d}") for index in range(1, count + 1)}
		self.threads: dict[str, threading.Thread] = {}
		self.lock = threading.Lock()
		self.stop_event = threading.Event()
		self.statistics = StatisticsRegistry()
		for node in self.nodes.values():
			self.statistics.register(node.statistics)

	def start_node(self, node_id: str) -> bool:
		with self.lock:
			node = self.nodes.get(node_id)
			if node is None or node_id in self.threads and self.threads[node_id].is_alive():
				return False
			node.stop_event.clear()
			thread = threading.Thread(target=node.run, name=node_id)
			self.threads[node_id] = thread
			thread.start()
			return True

	def stop_node(self, node_id: str) -> bool:
		with self.lock:
			node = self.nodes.get(node_id)
			thread = self.threads.get(node_id)
			if node is None or thread is None or not thread.is_alive():
				return False
			node.stop()
		thread.join(timeout=5)
		return not thread.is_alive()

	def start_all(self) -> None:
		for node_id in self.nodes:
			self.start_node(node_id)

	def stop_all(self) -> None:
		self.stop_event.set()
		for node_id in self.nodes:
			self.stop_node(node_id)
		path = self.statistics.export_csv(Path(__file__).resolve().parent / "stats")
		LOGGER.info("Reporte de estadisticas exportado en %s", path)

	def command_loop(self) -> None:
		"""Acepta start/stop NODE## y permite controlar nodos individualmente."""
		while not self.stop_event.is_set():
			try:
				command = input("nodes> ").strip().split()
			except (EOFError, KeyboardInterrupt):
				self.stop_all()
				return
			if not command:
				continue
			action = command[0].lower()
			if action in {"quit", "exit", "stopall"}:
				self.stop_all()
				return
			if len(command) != 2:
				LOGGER.info("Uso: start NODE01 | stop NODE01 | stopall")
				continue
			node_id = command[1].upper()
			changed = self.start_node(node_id) if action == "start" else self.stop_node(node_id)
			LOGGER.info("[%s] %s", node_id, "operacion aplicada" if changed else "operacion no aplicada")


def main() -> None:
	parser = argparse.ArgumentParser(description="Ejecuta multiples nodos IoT")
	parser.add_argument("--count", type=int, default=5, help="Cantidad de nodos")
	parser.add_argument(
		"--non-interactive",
		action="store_true",
		help="Mantiene los nodos activos sin leer comandos de stdin",
	)
	arguments = parser.parse_args()
	logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")

	launcher = NodeLauncher(arguments.count)
	launcher.start_all()
	if arguments.non_interactive:
		def stop_on_signal(signum, _frame):
			LOGGER.info("Senal %s recibida, deteniendo nodos", signum)
			launcher.stop_all()

		signal.signal(signal.SIGTERM, stop_on_signal)
		signal.signal(signal.SIGINT, stop_on_signal)
		try:
			while not launcher.stop_event.wait(1):
				pass
		finally:
			launcher.stop_all()
		return
	try:
		launcher.command_loop()
	finally:
		launcher.stop_all()


if __name__ == "__main__":
	main()
