from __future__ import annotations

from contextlib import AbstractAsyncContextManager
from typing import Any, AsyncIterator, Callable

from ..audio import INPUT_AUDIO
from ..config import GatewayConfig
from .base import ProviderEvent


ClientFactory = Callable[[str], Any]


def build_live_config(config: GatewayConfig) -> dict[str, Any]:
    return {
        "response_modalities": ["AUDIO"],
        "system_instruction": config.system_instruction,
        "input_audio_transcription": {},
        "output_audio_transcription": {},
        "speech_config": {
            "voice_config": {
                "prebuilt_voice_config": {"voice_name": config.voice_name}
            }
        },
    }


def _default_client_factory(api_key: str) -> Any:
    try:
        from google import genai
    except ImportError as exc:
        raise RuntimeError(
            "google-genai is not installed; run pip install -r requirements.txt"
        ) from exc
    return genai.Client(api_key=api_key)


def _blob(data: bytes) -> Any:
    try:
        from google.genai import types
    except ImportError as exc:
        raise RuntimeError(
            "google-genai is not installed; run pip install -r requirements.txt"
        ) from exc
    return types.Blob(data=data, mime_type=INPUT_AUDIO.mime_type)


class GeminiLiveSession:
    """Small, testable wrapper around one Gemini Live SDK session."""

    def __init__(
        self,
        config: GatewayConfig,
        client_factory: ClientFactory = _default_client_factory,
        blob_factory: Callable[[bytes], Any] = _blob,
    ) -> None:
        self._config = config
        self._client_factory = client_factory
        self._blob_factory = blob_factory
        self._context: AbstractAsyncContextManager[Any] | None = None
        self._session: Any = None

    async def connect(self) -> None:
        if self._session is not None:
            return
        client = self._client_factory(self._config.api_key)
        self._context = client.aio.live.connect(
            model=self._config.model,
            config=build_live_config(self._config),
        )
        self._session = await self._context.__aenter__()

    async def close(self) -> None:
        if self._context is not None:
            await self._context.__aexit__(None, None, None)
        self._context = None
        self._session = None

    async def send_audio(self, data: bytes) -> None:
        INPUT_AUDIO.validate(data)
        self._require_session()
        await self._session.send_realtime_input(audio=self._blob_factory(data))

    async def end_audio(self) -> None:
        self._require_session()
        await self._session.send_realtime_input(audio_stream_end=True)

    async def send_text(self, text: str) -> None:
        if not text.strip():
            raise ValueError("text must not be empty")
        self._require_session()
        await self._session.send_realtime_input(text=text.strip())

    async def receive_forever(self) -> AsyncIterator[ProviderEvent]:
        self._require_session()
        while True:
            async for response in self._session.receive():
                for event in from_response(response):
                    yield event

    def _require_session(self) -> None:
        if self._session is None:
            raise RuntimeError("Gemini session is not connected")


def from_response(response: Any) -> list[ProviderEvent]:
    events: list[ProviderEvent] = []
    content = getattr(response, "server_content", None)
    if content is None:
        return events

    input_transcription = getattr(content, "input_transcription", None)
    input_text = getattr(input_transcription, "text", None)
    if input_text:
        events.append(ProviderEvent(kind="transcript.input", text=input_text))

    output_transcription = getattr(content, "output_transcription", None)
    output_text = getattr(output_transcription, "text", None)
    if output_text:
        events.append(ProviderEvent(kind="transcript.output", text=output_text))

    model_turn = getattr(content, "model_turn", None)
    for part in getattr(model_turn, "parts", None) or []:
        inline_data = getattr(part, "inline_data", None)
        audio = getattr(inline_data, "data", None)
        if audio:
            events.append(ProviderEvent.audio(audio))

    if getattr(content, "interrupted", False):
        events.append(ProviderEvent(kind="turn.interrupted"))
    if getattr(content, "turn_complete", False):
        events.append(ProviderEvent(kind="turn.complete"))
    return events
