from __future__ import annotations

import argparse
import asyncio
import json
from pathlib import Path

import websockets


HELLO = json.dumps(
    {
        "type": "hello",
        "version": 1,
        "transport": "websocket",
        "audio_params": {"format": "opus", "sample_rate": 16000},
    },
    separators=(",", ":"),
)


def _load_single_device(path: Path) -> tuple[str, str]:
    payload = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(payload, dict) or len(payload) != 1:
        raise ValueError("credentials file must contain exactly one device")
    device_id, token = next(iter(payload.items()))
    if not isinstance(device_id, str) or not isinstance(token, str):
        raise ValueError("device ID and token must be strings")
    return device_id, token


async def _exchange(url: str, device_id: str, token: str) -> dict[str, object]:
    headers = {
        "Device-Id": device_id,
        "Authorization": f"Bearer {token}",
    }
    async with websockets.connect(
        url,
        additional_headers=headers,
        open_timeout=30,
        close_timeout=5,
        max_size=64 * 1024,
    ) as websocket:
        await websocket.send(HELLO)
        message = await asyncio.wait_for(websocket.recv(), timeout=30)
    if not isinstance(message, str):
        raise RuntimeError("gateway returned binary data before its hello response")
    payload = json.loads(message)
    if not isinstance(payload, dict):
        raise RuntimeError("gateway response was not a JSON object")
    return payload


async def _main(args: argparse.Namespace) -> None:
    device_id, token = _load_single_device(args.credentials_file)

    rejected = await _exchange(args.url, device_id, "invalid-smoke-test-token")
    if rejected.get("type") != "error" or "authentication" not in str(
        rejected.get("message", "")
    ):
        raise RuntimeError("invalid device credential was not rejected")
    print("invalid credential: rejected")

    if args.verify_valid:
        accepted = await _exchange(args.url, device_id, token)
        if accepted.get("type") != "hello":
            raise RuntimeError("valid device credential did not receive gateway hello")
        print("valid credential: accepted")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Check EchoEar Cloud Run WebSocket authentication without printing secrets."
    )
    parser.add_argument("url")
    parser.add_argument("credentials_file", type=Path)
    parser.add_argument("--verify-valid", action="store_true")
    asyncio.run(_main(parser.parse_args()))


if __name__ == "__main__":
    main()
