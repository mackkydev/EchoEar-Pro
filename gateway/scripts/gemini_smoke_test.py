from __future__ import annotations

import argparse
import asyncio
from pathlib import Path
import sys
import wave


GATEWAY_ROOT = Path(__file__).resolve().parents[1]
if str(GATEWAY_ROOT) not in sys.path:
    sys.path.insert(0, str(GATEWAY_ROOT))

from echoear_gateway.audio import OUTPUT_AUDIO
from echoear_gateway.config import GatewayConfig
from echoear_gateway.providers.gemini_live import GeminiLiveSession


async def run(prompt: str, output: Path, timeout_seconds: float) -> None:
    config = GatewayConfig.from_environment()
    session = GeminiLiveSession(config)
    audio = bytearray()
    transcript: list[str] = []
    try:
        await session.connect()
        await session.send_text(prompt)
        async with asyncio.timeout(timeout_seconds):
            async for event in session.receive_forever():
                if event.kind == "audio" and event.data:
                    audio.extend(event.data)
                elif event.kind == "transcript.output" and event.text:
                    transcript.append(event.text)
                    print(event.text, end="", flush=True)
                elif event.kind == "turn.complete":
                    break
        print()
    finally:
        await session.close()

    if not audio:
        raise RuntimeError("Gemini returned no audio")
    output.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(output), "wb") as wav:
        wav.setnchannels(OUTPUT_AUDIO.channels)
        wav.setsampwidth(OUTPUT_AUDIO.sample_width_bytes)
        wav.setframerate(OUTPUT_AUDIO.sample_rate)
        wav.writeframes(audio)
    print(f"Saved {len(audio)} PCM bytes to {output.resolve()}")


def main() -> None:
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    parser = argparse.ArgumentParser(description="Test EchoEar Gemini voice without firmware")
    parser.add_argument(
        "--prompt",
        default="สวัสดี แนะนำตัวสั้น ๆ เป็นภาษาไทยในชื่อ Robo",
    )
    parser.add_argument("--output", type=Path, default=Path("output/gemini-thai.wav"))
    parser.add_argument("--timeout", type=float, default=45.0)
    args = parser.parse_args()
    asyncio.run(run(args.prompt, args.output, args.timeout))


if __name__ == "__main__":
    main()
