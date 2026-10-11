from __future__ import annotations

import argparse
import asyncio
from contextlib import suppress
from dataclasses import replace
import json
import logging
from pathlib import Path
import sys
import wave


GATEWAY_ROOT = Path(__file__).resolve().parents[1]
if str(GATEWAY_ROOT) not in sys.path:
    sys.path.insert(0, str(GATEWAY_ROOT))

from websockets.asyncio.client import connect

from echoear_gateway.audio_opus import OpusToPcmDecoder
from echoear_gateway.config import GatewayConfig
from echoear_gateway.server import serve


async def run(prompt: str, output: Path, timeout_seconds: float) -> None:
    config = replace(GatewayConfig.from_environment(), host="127.0.0.1", port=8766)
    server_task = asyncio.create_task(serve(config))
    await asyncio.sleep(0.2)

    decoder = OpusToPcmDecoder(output_rate=24_000)
    output_pcm = bytearray()
    saw_start = False
    saw_stop = False
    transcript = []
    try:
        async with asyncio.timeout(timeout_seconds):
            async with connect(f"ws://{config.host}:{config.port}") as websocket:
                await websocket.send(
                    json.dumps(
                        {
                            "type": "hello",
                            "version": 1,
                            "transport": "websocket",
                            "audio_params": {
                                "format": "opus",
                                "sample_rate": 16_000,
                                "channels": 1,
                                "frame_duration": 60,
                            },
                        }
                    )
                )
                ready = json.loads(await websocket.recv())
                if ready.get("type") != "hello":
                    raise RuntimeError("gateway did not return XiaoZhi hello")

                await websocket.send(json.dumps({"type": "text", "text": prompt}))
                async for message in websocket:
                    if isinstance(message, bytes):
                        for pcm in decoder.decode(message):
                            output_pcm.extend(pcm)
                        continue

                    event = json.loads(message)
                    if event.get("type") == "tts" and event.get("state") == "start":
                        saw_start = True
                    elif event.get("type") == "tts" and event.get("state") == "sentence_start":
                        text = event.get("text", "")
                        transcript.append(text)
                        print(text, end="", flush=True)
                    elif event.get("type") == "tts" and event.get("state") == "stop":
                        saw_stop = True
                        break
                    elif event.get("type") == "error":
                        raise RuntimeError(f"gateway error: {event.get('message', 'unknown')}")
    finally:
        server_task.cancel()
        with suppress(asyncio.CancelledError, TimeoutError):
            await asyncio.wait_for(server_task, timeout=5)

    print()
    if not saw_start or not saw_stop or not output_pcm:
        raise RuntimeError("XiaoZhi-compatible Gemini turn was incomplete")
    output.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(output), "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(24_000)
        wav.writeframes(output_pcm)
    print(f"Saved {len(output_pcm)} decoded PCM bytes to {output.resolve()}")


def main() -> None:
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s %(levelname)s %(name)s: %(message)s",
    )
    parser = argparse.ArgumentParser(
        description="Test the full XiaoZhi Opus/WebSocket to Gemini Gateway path"
    )
    parser.add_argument(
        "--prompt",
        default="ตอบสั้น ๆ เป็นภาษาไทยว่า เส้นทางเสียง EchoEar พร้อมแล้ว",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("output/xiaozhi-gemini-roundtrip.wav"),
    )
    parser.add_argument("--timeout", type=float, default=45.0)
    args = parser.parse_args()
    asyncio.run(run(args.prompt, args.output, args.timeout))


if __name__ == "__main__":
    main()
