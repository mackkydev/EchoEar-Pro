from __future__ import annotations

from collections.abc import Iterable

import av


class OpusToPcmDecoder:
    """Decode headerless Opus packets and continuously resample to PCM16 mono."""

    def __init__(self, output_rate: int) -> None:
        if output_rate <= 0:
            raise ValueError("output_rate must be positive")
        self.output_rate = output_rate
        self._decoder = av.CodecContext.create("opus", "r")
        self._decoder.open()
        self._resampler = av.AudioResampler(
            format="s16",
            layout="mono",
            rate=output_rate,
        )

    def decode(self, packet: bytes) -> list[bytes]:
        if not packet:
            raise ValueError("Opus packet is empty")
        output: list[bytes] = []
        for frame in self._decoder.decode(av.Packet(packet)):
            output.extend(self._pcm_bytes(self._resampler.resample(frame)))
        return output

    @staticmethod
    def _pcm_bytes(frames: Iterable[av.AudioFrame]) -> list[bytes]:
        result: list[bytes] = []
        for frame in frames:
            byte_count = frame.samples * 2
            if byte_count:
                result.append(bytes(frame.planes[0])[:byte_count])
        return result


class PcmToOpusEncoder:
    """Buffer arbitrary PCM16 chunks into headerless 60 ms Opus packets."""

    def __init__(
        self,
        sample_rate: int,
        frame_duration_ms: int = 60,
        bitrate: int = 32_000,
    ) -> None:
        if sample_rate <= 0 or frame_duration_ms <= 0:
            raise ValueError("sample rate and frame duration must be positive")
        samples_times_ms = sample_rate * frame_duration_ms
        if samples_times_ms % 1000:
            raise ValueError("frame duration must produce a whole sample count")

        self.sample_rate = sample_rate
        self.frame_duration_ms = frame_duration_ms
        self.frame_samples = samples_times_ms // 1000
        self.frame_bytes = self.frame_samples * 2
        self._pending = bytearray()
        self._encoder = av.CodecContext.create("libopus", "w")
        self._encoder.sample_rate = sample_rate
        self._encoder.layout = "mono"
        self._encoder.format = "s16"
        self._encoder.bit_rate = bitrate
        self._encoder.options = {
            "application": "voip",
            "frame_duration": str(frame_duration_ms),
        }
        self._encoder.open()

    def feed(self, pcm: bytes) -> list[bytes]:
        if len(pcm) % 2:
            raise ValueError("PCM chunk ends inside a 16-bit sample")
        self._pending.extend(pcm)
        packets: list[bytes] = []
        while len(self._pending) >= self.frame_bytes:
            frame_pcm = bytes(self._pending[: self.frame_bytes])
            del self._pending[: self.frame_bytes]
            packets.extend(self._encode_frame(frame_pcm))
        return packets

    def finish_turn(self) -> list[bytes]:
        if not self._pending:
            return []
        padding = self.frame_bytes - len(self._pending)
        frame_pcm = bytes(self._pending) + bytes(padding)
        self._pending.clear()
        return self._encode_frame(frame_pcm)

    def discard(self) -> None:
        self._pending.clear()

    def _encode_frame(self, pcm: bytes) -> list[bytes]:
        frame = av.AudioFrame(format="s16", layout="mono", samples=self.frame_samples)
        frame.sample_rate = self.sample_rate
        frame.planes[0].update(pcm)
        return [bytes(packet) for packet in self._encoder.encode(frame)]
