from __future__ import annotations

from dataclasses import dataclass
from typing import Literal


EventKind = Literal[
    "audio",
    "transcript.input",
    "transcript.output",
    "turn.complete",
    "turn.interrupted",
]


@dataclass(frozen=True)
class ProviderEvent:
    kind: EventKind
    data: bytes | None = None
    text: str | None = None

    @classmethod
    def audio(cls, data: bytes) -> "ProviderEvent":
        return cls(kind="audio", data=data)
