from __future__ import annotations

from dataclasses import dataclass, field
import json
import os
from pathlib import Path
import secrets
from types import MappingProxyType
from typing import Mapping


DEFAULT_MODEL = "gemini-3.1-flash-live-preview"
DEFAULT_ENV_FILE = Path(__file__).resolve().parents[1] / ".env"
FEMALE_VOICES = frozenset({"Leda", "Sulafat", "Aoede", "Kore"})
PRODUCTION_ENVIRONMENTS = frozenset({"production", "prod"})


def _read_env_file(path: Path) -> dict[str, str]:
    if not path.is_file():
        return {}
    values: dict[str, str] = {}
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        key = key.strip()
        if key:
            values[key] = value.strip()
    return values


def _read_positive_int(values: Mapping[str, str], name: str, default: int) -> int:
    raw_value = values.get(name, str(default))
    try:
        value = int(raw_value)
    except ValueError as exc:
        raise ValueError(f"{name} must be a number") from exc
    if value < 1:
        raise ValueError(f"{name} must be at least 1")
    return value


def _load_device_tokens(values: Mapping[str, str], env_file: Path) -> Mapping[str, str]:
    raw_json = values.get("ECHOEAR_DEVICE_CREDENTIALS_JSON", "").strip()
    raw_path = values.get("ECHOEAR_DEVICE_CREDENTIALS_FILE", "").strip()
    if raw_json and raw_path:
        raise ValueError(
            "set only one of ECHOEAR_DEVICE_CREDENTIALS_JSON or "
            "ECHOEAR_DEVICE_CREDENTIALS_FILE"
        )
    if raw_path:
        path = Path(raw_path)
        if not path.is_absolute():
            path = env_file.parent / path
        try:
            raw_json = path.read_text(encoding="utf-8")
        except OSError as exc:
            raise ValueError(f"could not read device credentials file: {path}") from exc
    if not raw_json:
        return MappingProxyType({})
    try:
        payload = json.loads(raw_json)
    except json.JSONDecodeError as exc:
        raise ValueError("device credentials must be valid JSON") from exc
    if not isinstance(payload, dict):
        raise ValueError("device credentials must be a JSON object")
    device_tokens: dict[str, str] = {}
    for raw_device_id, raw_token in payload.items():
        if not isinstance(raw_device_id, str) or not isinstance(raw_token, str):
            raise ValueError("device credential keys and tokens must be strings")
        device_id = raw_device_id.strip().lower()
        token = raw_token.strip()
        if not device_id or not token:
            raise ValueError("device credential keys and tokens cannot be empty")
        device_tokens[device_id] = token
    return MappingProxyType(device_tokens)


@dataclass(frozen=True)
class GatewayConfig:
    api_key: str = field(repr=False)
    host: str = "0.0.0.0"
    port: int = 8765
    model: str = DEFAULT_MODEL
    gateway_token: str = field(default="", repr=False)
    voice_name: str = "Leda"
    assistant_name: str = "Robo"
    environment: str = "development"
    device_tokens: Mapping[str, str] = field(
        default_factory=lambda: MappingProxyType({}), repr=False
    )
    max_active_sessions: int = 100
    max_sessions_per_device: int = 1
    session_max_seconds: int = 3600

    @classmethod
    def from_environment(
        cls,
        environ: dict[str, str] | None = None,
        env_file: Path = DEFAULT_ENV_FILE,
    ) -> "GatewayConfig":
        if environ is None:
            values = _read_env_file(env_file)
            values.update(os.environ)
        else:
            values = environ
        key = values.get("GEMINI_API_KEY", "").strip()
        if not key:
            raise ValueError(
                "GEMINI_API_KEY is not set. Keep it on the gateway; do not put it in firmware."
            )

        raw_port = values.get("ECHOEAR_GATEWAY_PORT", values.get("PORT", "8765"))
        try:
            port = int(raw_port)
        except ValueError as exc:
            raise ValueError("ECHOEAR_GATEWAY_PORT must be a number") from exc
        if not 1 <= port <= 65535:
            raise ValueError("ECHOEAR_GATEWAY_PORT must be between 1 and 65535")

        environment = values.get("ECHOEAR_ENVIRONMENT", "development").strip().lower()
        if environment not in {"development", "test", "production", "prod"}:
            raise ValueError(
                "ECHOEAR_ENVIRONMENT must be development, test, or production"
            )
        device_tokens = _load_device_tokens(values, env_file)
        gateway_token = values.get("ECHOEAR_GATEWAY_TOKEN", "").strip()
        if environment in PRODUCTION_ENVIRONMENTS:
            if not device_tokens:
                raise ValueError(
                    "production requires ECHOEAR_DEVICE_CREDENTIALS_FILE or "
                    "ECHOEAR_DEVICE_CREDENTIALS_JSON"
                )
            weak_device = next(
                (device_id for device_id, token in device_tokens.items() if len(token) < 32),
                None,
            )
            if weak_device is not None:
                raise ValueError(
                    f"production device token for {weak_device} must be at least 32 characters"
                )

        return cls(
            api_key=key,
            host=values.get("ECHOEAR_GATEWAY_HOST", "0.0.0.0").strip() or "0.0.0.0",
            port=port,
            model=values.get("ECHOEAR_GATEWAY_MODEL", DEFAULT_MODEL).strip() or DEFAULT_MODEL,
            gateway_token=gateway_token,
            voice_name=values.get("ECHOEAR_GEMINI_VOICE", "Leda").strip() or "Leda",
            assistant_name=values.get("ECHOEAR_ASSISTANT_NAME", "Robo").strip() or "Robo",
            environment=environment,
            device_tokens=device_tokens,
            max_active_sessions=_read_positive_int(
                values, "ECHOEAR_MAX_ACTIVE_SESSIONS", 100
            ),
            max_sessions_per_device=_read_positive_int(
                values, "ECHOEAR_MAX_SESSIONS_PER_DEVICE", 1
            ),
            session_max_seconds=_read_positive_int(
                values, "ECHOEAR_SESSION_MAX_SECONDS", 3600
            ),
        )

    @property
    def is_production(self) -> bool:
        return self.environment in PRODUCTION_ENVIRONMENTS

    def authenticate_device(self, device_id: str, presented_token: str) -> bool:
        normalized_device_id = device_id.strip().lower()
        if self.device_tokens:
            expected = self.device_tokens.get(normalized_device_id, "")
            # Compare even for an unknown device to reduce device-id enumeration clues.
            comparison_target = expected or ("0" * max(32, len(presented_token)))
            return bool(expected and presented_token) and secrets.compare_digest(
                comparison_target, presented_token
            )
        if self.gateway_token:
            return bool(presented_token) and secrets.compare_digest(
                self.gateway_token, presented_token
            )
        return not self.is_production

    @property
    def system_instruction(self) -> str:
        if self.voice_name in FEMALE_VOICES:
            thai_voice_identity = (
                "Use a feminine speaking persona. When speaking Thai, use female "
                "sentence-ending particles such as ค่ะ and คะ naturally; never use "
                "ครับ or masculine self-reference. "
            )
        else:
            thai_voice_identity = (
                "Use a masculine speaking persona. When speaking Thai, use male "
                "sentence-ending particles such as ครับ naturally. "
            )
        return (
            f"You are {self.assistant_name}, the voice assistant in EchoEar Pro. "
            "Reply naturally and concisely in the language used by the user. "
            "When the user speaks Thai, use fluent conversational Thai. "
            f"{thai_voice_identity}"
            "Prefer short spoken answers unless the user asks for detail."
        )
