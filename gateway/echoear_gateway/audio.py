from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class PcmFormat:
    sample_rate: int
    channels: int = 1
    sample_width_bytes: int = 2

    @property
    def mime_type(self) -> str:
        return f"audio/pcm;rate={self.sample_rate}"

    def bytes_for_ms(self, duration_ms: int) -> int:
        if duration_ms <= 0:
            raise ValueError("duration_ms must be positive")
        numerator = self.sample_rate * self.channels * self.sample_width_bytes * duration_ms
        if numerator % 1000:
            raise ValueError("duration does not produce a whole PCM frame")
        return numerator // 1000

    def validate(self, data: bytes) -> None:
        frame_size = self.channels * self.sample_width_bytes
        if not data:
            raise ValueError("audio chunk is empty")
        if len(data) % frame_size:
            raise ValueError("audio chunk ends inside a PCM frame")


INPUT_AUDIO = PcmFormat(sample_rate=16_000)
OUTPUT_AUDIO = PcmFormat(sample_rate=24_000)
RECOMMENDED_INPUT_CHUNK_BYTES = INPUT_AUDIO.bytes_for_ms(100)
