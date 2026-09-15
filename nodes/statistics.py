"""Metricas y exportacion de ejecuciones de nodos."""

from __future__ import annotations

import csv
import threading
from dataclasses import asdict, dataclass
from datetime import datetime, timezone
from pathlib import Path


@dataclass
class NodeStatistics:
    node_id: str
    messages_attempted: int = 0
    messages_transmitted: int = 0
    send_failures: int = 0
    acknowledgements_received: int = 0
    sequence_gaps_observed: int = 0

    @property
    def network_loss_unknown(self) -> bool:
        return self.messages_attempted > 0 and self.acknowledgements_received == 0

    @property
    def local_loss_percentage(self) -> float:
        if self.messages_attempted == 0:
            return 0.0
        return self.send_failures * 100.0 / self.messages_attempted

    @property
    def network_loss_percentage(self) -> str:
        return "N/D" if self.network_loss_unknown else str(self.local_loss_percentage)

    def snapshot(self) -> dict[str, object]:
        values = asdict(self)
        values["estimated_local_losses"] = self.send_failures
        values["network_loss_unknown"] = self.network_loss_unknown
        values["local_loss_percentage"] = round(self.local_loss_percentage, 2)
        values["network_loss_percentage"] = self.network_loss_percentage
        return values


class StatisticsRegistry:
    """Almacena metricas de nodos y exporta un reporte CSV agregado."""

    def __init__(self) -> None:
        self._items: dict[str, NodeStatistics] = {}
        self._lock = threading.Lock()

    def for_node(self, node_id: str) -> NodeStatistics:
        with self._lock:
            if node_id not in self._items:
                self._items[node_id] = NodeStatistics(node_id)
            return self._items[node_id]

    def register(self, statistics: NodeStatistics) -> None:
        with self._lock:
            self._items[statistics.node_id] = statistics

    def snapshot(self) -> list[dict[str, object]]:
        with self._lock:
            return [item.snapshot() for item in self._items.values()]

    def export_csv(self, directory: Path) -> Path:
        directory.mkdir(parents=True, exist_ok=True)
        path = directory / "telemetry_stats.csv"
        rows = self.snapshot()
        fieldnames = [
            "node_id",
            "messages_attempted",
            "messages_transmitted",
            "send_failures",
            "acknowledgements_received",
            "sequence_gaps_observed",
            "estimated_local_losses",
            "network_loss_unknown",
            "local_loss_percentage",
            "network_loss_percentage",
        ]
        with path.open("w", newline="", encoding="utf-8") as report:
            writer = csv.DictWriter(report, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(rows)

        aggregate = self.aggregate()
        summary_path = directory / "telemetry_summary.txt"
        with summary_path.open("w", encoding="utf-8") as report:
            report.write(f"Generado: {datetime.now(timezone.utc).isoformat()}\n")
            for key, value in aggregate.items():
                report.write(f"{key}: {value}\n")
            report.write(
                "nota: sin ACK UDP, la perdida de red no puede determinarse "
                "desde los nodos; estimated_local_losses son fallos locales de envio.\n"
            )
        return path

    def aggregate(self) -> dict[str, int | bool]:
        rows = self.snapshot()
        return {
            "nodes": len(rows),
            "messages_attempted": sum(int(row["messages_attempted"]) for row in rows),
            "messages_transmitted": sum(int(row["messages_transmitted"]) for row in rows),
            "send_failures": sum(int(row["send_failures"]) for row in rows),
            "acknowledgements_received": sum(
                int(row["acknowledgements_received"]) for row in rows
            ),
            "sequence_gaps_observed": sum(
                int(row["sequence_gaps_observed"]) for row in rows
            ),
            "estimated_local_losses": sum(
                int(row["estimated_local_losses"]) for row in rows
            ),
            "network_loss_unknown": any(
                bool(row["network_loss_unknown"]) for row in rows
            ),
            "local_loss_percentage": round(
                sum(int(row["send_failures"]) for row in rows)
                * 100.0
                / max(sum(int(row["messages_attempted"]) for row in rows), 1),
                2,
            ),
            "network_loss_percentage": "N/D",
        }
