"""Cliente UDP explicito para enviar telemetria al servidor."""

from __future__ import annotations

import logging
import socket


LOGGER = logging.getLogger(__name__)


class NetworkClient:
	"""Resuelve el servidor y envia datagramas sin terminar el proceso."""

	def __init__(self, hostname: str, udp_port: int, timeout: float = 3.0) -> None:
		self.hostname = hostname
		self.udp_port = udp_port
		self.timeout = timeout
		self._server_address: tuple[str, int] | None = None

	def resolve_server(self) -> tuple[str, int] | None:
		"""Resuelve explicitamente el hostname, sin aceptar una IP fija."""
		try:
			results = socket.getaddrinfo(
				self.hostname,
				self.udp_port,
				socket.AF_INET,
				socket.SOCK_DGRAM,
			)
			if not results:
				LOGGER.error("DNS no devolvio direcciones para %s", self.hostname)
				return None
			address = results[0][4]
			self._server_address = (address[0], address[1])
			LOGGER.info("DNS resuelto: %s -> %s:%s", self.hostname, *self._server_address)
			return self._server_address
		except (socket.gaierror, OSError) as error:
			LOGGER.error("No se pudo resolver %s: %s", self.hostname, error)
			self._server_address = None
			return None

	def send_datagram(self, message: str) -> bool:
		"""Envia un datagrama UDP; no espera ACK porque el servidor no responde."""
		address = self._server_address or self.resolve_server()
		if address is None:
			return False

		sock: socket.socket | None = None
		try:
			# UDP es apropiado aqui porque la telemetria es un datagrama independiente
			# y el servidor la procesa como fire-and-forget, sin ACK ni handshake.
			sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
			sock.settimeout(self.timeout)
			payload = message.encode("utf-8")
			sent = sock.sendto(payload, address)
			if sent != len(payload):
				LOGGER.error("Datagrama incompleto: %s de %s bytes", sent, len(payload))
				return False
			LOGGER.debug("Datagrama UDP enviado a %s:%s", *address)
			return True
		except (socket.timeout, OSError, UnicodeError) as error:
			LOGGER.error("Error enviando datagrama UDP: %s", error)
			return False
		finally:
			if sock is not None:
				try:
					sock.close()
				except OSError as error:
					LOGGER.error("Error cerrando socket UDP: %s", error)
