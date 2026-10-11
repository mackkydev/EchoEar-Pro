from __future__ import annotations

import json
from dataclasses import dataclass
import secrets
from typing import Any

from .audio import INPUT_AUDIO, OUTPUT_AUDIO


PROTOCOL_VERSION = 1
SUPPORTED_VOICES = frozenset({"Leda", "Sulafat", "Aoede", "Achird", "Kore"})


class ProtocolError(ValueError):
    pass


@dataclass(frozen=True)
class DeviceHello:
    variant: str
    device_id: str
    voice_name: str | None = None


def _parse_voice(message: dict[str, Any]) -> str | None:
    voice = message.get("voice")
    if voice is None:
        return None
    if not isinstance(voice, str) or voice not in SUPPORTED_VOICES:
        raise ProtocolError("voice is not supported")
    return voice


def parse_hello(
    message: dict[str, Any],
    expected_token: str = "",
    device_id_override: str = "",
) -> DeviceHello:
    if message.get("type") != "hello":
        raise ProtocolError("the first message must be hello")

    if message.get("protocol") == PROTOCOL_VERSION:
        if device_id_override and not message.get("device_id"):
            message = {**message, "device_id": device_id_override}
        return DeviceHello(
            variant="echoear-pcm-v1",
            device_id=validate_hello(message, expected_token),
            voice_name=_parse_voice(message),
        )

    audio = message.get("audio_params")
    if (
        message.get("transport") == "websocket"
        and isinstance(audio, dict)
        and audio.get("format") == "opus"
    ):
        if expected_token and message.get("token") != expected_token:
            raise ProtocolError("gateway token is invalid")
        return DeviceHello(
            variant="xiaozhi-opus",
            device_id=str(
                message.get("device_id")
                or device_id_override
                or "xiaozhi-device"
            ).strip().lower(),
            voice_name=_parse_voice(message),
        )

    raise ProtocolError("unsupported hello message")


def decode_control(raw: str) -> dict[str, Any]:
    try:
        message = json.loads(raw)
    except json.JSONDecodeError as exc:
        raise ProtocolError("control message is not valid JSON") from exc
    if not isinstance(message, dict) or not isinstance(message.get("type"), str):
        raise ProtocolError("control message must be an object with a string type")
    return message


def encode_control(event_type: str, **payload: Any) -> str:
    return json.dumps({"type": event_type, **payload}, ensure_ascii=False, separators=(",", ":"))


def validate_hello(message: dict[str, Any], expected_token: str = "") -> str:
    if message.get("type") != "hello":
        raise ProtocolError("the first message must be hello")
    if message.get("protocol") != PROTOCOL_VERSION:
        raise ProtocolError(f"unsupported protocol version; expected {PROTOCOL_VERSION}")
    device_id = message.get("device_id")
    if not isinstance(device_id, str) or not device_id.strip():
        raise ProtocolError("device_id is required")
    if expected_token and message.get("token") != expected_token:
        raise ProtocolError("gateway token is invalid")
    return device_id.strip()


def ready_message(provider: str, model: str) -> str:
    return encode_control(
        "ready",
        protocol=PROTOCOL_VERSION,
        provider=provider,
        model=model,
        input_audio={"encoding": "pcm_s16le", "sample_rate": INPUT_AUDIO.sample_rate, "channels": 1},
        output_audio={"encoding": "pcm_s16le", "sample_rate": OUTPUT_AUDIO.sample_rate, "channels": 1},
    )


def xiaozhi_ready_message() -> str:
    return encode_control(
        "hello",
        transport="websocket",
        session_id=f"echoear-{secrets.token_hex(8)}",
        audio_params={
            "format": "opus",
            "sample_rate": OUTPUT_AUDIO.sample_rate,
            "channels": 1,
            "frame_duration": 60,
        },
    )
