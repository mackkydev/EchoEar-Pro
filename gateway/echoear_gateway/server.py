from __future__ import annotations

import asyncio
from collections import defaultdict
from dataclasses import replace
import json
import logging
from types import SimpleNamespace
from typing import Any, Callable

from .audio import INPUT_AUDIO
from .audio_opus import OpusToPcmDecoder, PcmToOpusEncoder
from .config import GatewayConfig
from .protocol import (
    DeviceHello,
    ProtocolError,
    decode_control,
    encode_control,
    parse_hello,
    ready_message,
    xiaozhi_ready_message,
)
from .providers.base import ProviderEvent
from .providers.gemini_live import GeminiLiveSession


LOGGER = logging.getLogger("echoear.gateway")


class GatewayRuntime:
    def __init__(self, config: GatewayConfig) -> None:
        self.config = config
        self.active_sessions = 0
        self.active_by_device: dict[str, int] = defaultdict(int)
        self._lock = asyncio.Lock()

    async def acquire(self, device_id: str) -> str | None:
        async with self._lock:
            if self.active_sessions >= self.config.max_active_sessions:
                return "gateway capacity reached"
            if (
                self.active_by_device[device_id]
                >= self.config.max_sessions_per_device
            ):
                return "device session limit reached"
            self.active_sessions += 1
            self.active_by_device[device_id] += 1
        return None

    async def release(self, device_id: str) -> None:
        async with self._lock:
            if self.active_by_device.get(device_id, 0) > 0:
                self.active_by_device[device_id] -= 1
                self.active_sessions -= 1
                if self.active_by_device[device_id] == 0:
                    del self.active_by_device[device_id]

    def health_payload(self) -> bytes:
        return json.dumps(
            {
                "status": "ok",
                "service": "echoear-gateway",
                "environment": self.config.environment,
                "active_sessions": self.active_sessions,
            },
            separators=(",", ":"),
        ).encode("utf-8")


def _request_header(websocket: Any, name: str) -> str:
    request = getattr(websocket, "request", None)
    headers = getattr(request, "headers", None)
    if headers is None:
        return ""
    try:
        return str(headers.get(name, "")).strip()
    except Exception:
        return ""


def _presented_token(websocket: Any, hello_message: dict[str, Any]) -> str:
    authorization = _request_header(websocket, "Authorization")
    if authorization.lower().startswith("bearer "):
        return authorization[7:].strip()
    token = hello_message.get("token")
    return token.strip() if isinstance(token, str) else ""


class AiohttpWebSocketAdapter:
    def __init__(self, websocket: Any, headers: Any) -> None:
        self.websocket = websocket
        self.request = SimpleNamespace(headers=headers)

    async def recv(self) -> str | bytes:
        message = await self.websocket.receive()
        try:
            from aiohttp import WSMsgType
        except ImportError as exc:
            raise RuntimeError("aiohttp is not installed") from exc
        if message.type == WSMsgType.TEXT:
            return message.data
        if message.type == WSMsgType.BINARY:
            return message.data
        if message.type in {WSMsgType.CLOSE, WSMsgType.CLOSED, WSMsgType.CLOSING}:
            raise StopAsyncIteration
        if message.type == WSMsgType.ERROR:
            raise ProtocolError("WebSocket transport error")
        raise ProtocolError("unsupported WebSocket message")

    def __aiter__(self) -> "AiohttpWebSocketAdapter":
        return self

    async def __anext__(self) -> str | bytes:
        try:
            return await self.recv()
        except StopAsyncIteration:
            raise

    async def send(self, message: str | bytes) -> None:
        if isinstance(message, bytes):
            await self.websocket.send_bytes(message, compress=False)
        else:
            await self.websocket.send_str(message, compress=False)


async def serve(config: GatewayConfig) -> None:
    try:
        from aiohttp import web
    except ImportError as exc:
        raise RuntimeError(
            "production server dependencies are missing; run pip install -r requirements.txt"
        ) from exc

    runtime = GatewayRuntime(config)

    async def health(request: Any) -> Any:
        return web.Response(
            body=runtime.health_payload(),
            content_type="application/json",
            headers={"Cache-Control": "no-store"},
        )

    async def websocket_endpoint(request: Any) -> Any:
        # Cloud Run and other reverse proxies may probe the root path with a
        # normal HTTP GET. Keep that probe healthy while still accepting the
        # XiaoZhi WebSocket upgrade on the same route.
        if request.headers.get("Upgrade", "").lower() != "websocket":
            return await health(request)
        websocket = web.WebSocketResponse(
            max_msg_size=64 * 1024,
            compress=False,
            heartbeat=20,
            receive_timeout=config.session_max_seconds,
        )
        await websocket.prepare(request)
        adapter = AiohttpWebSocketAdapter(websocket, request.headers)
        try:
            await handle_device(adapter, config, runtime=runtime)
        finally:
            try:
                await websocket.close()
            except Exception:
                pass
        return websocket

    app = web.Application(client_max_size=64 * 1024)
    app.router.add_get("/healthz", health)
    app.router.add_get("/readyz", health)
    app.router.add_get("/", websocket_endpoint)
    app.router.add_get("/ws", websocket_endpoint)
    LOGGER.info(
        "EchoEar Gateway listening on %s:%d (%s)",
        config.host,
        config.port,
        config.environment,
    )
    runner = web.AppRunner(app, access_log=LOGGER)
    await runner.setup()
    site = web.TCPSite(runner, config.host, config.port, shutdown_timeout=10)
    await site.start()
    try:
        await asyncio.Future()
    finally:
        await runner.cleanup()


async def handle_device(
    websocket: Any,
    config: GatewayConfig,
    provider_factory: Callable[[GatewayConfig], GeminiLiveSession] = GeminiLiveSession,
    runtime: GatewayRuntime | None = None,
) -> None:
    send_lock = asyncio.Lock()
    provider: GeminiLiveSession | None = None
    device_id = "unknown"
    acquired = False
    runtime = runtime or GatewayRuntime(config)
    try:
        raw_hello = await asyncio.wait_for(websocket.recv(), timeout=10)
        if not isinstance(raw_hello, str):
            raise ProtocolError("the first message must be JSON hello")
        hello_message = decode_control(raw_hello)
        hello = parse_hello(
            hello_message,
            device_id_override=_request_header(websocket, "Device-Id"),
        )
        device_id = hello.device_id
        if not config.authenticate_device(
            device_id, _presented_token(websocket, hello_message)
        ):
            raise ProtocolError("device authentication failed")
        capacity_error = await runtime.acquire(device_id)
        if capacity_error is not None:
            raise ProtocolError(capacity_error)
        acquired = True
        session_config = (
            replace(config, voice_name=hello.voice_name)
            if hello.voice_name is not None
            else config
        )
        provider = provider_factory(session_config)
        await provider.connect()
        if hello.variant == "xiaozhi-opus":
            await websocket.send(xiaozhi_ready_message())
        else:
            await websocket.send(ready_message("gemini", config.model))
        LOGGER.info(
            "device connected: %s (%s, voice=%s)",
            device_id,
            hello.variant,
            session_config.voice_name,
        )

        async with asyncio.timeout(config.session_max_seconds):
            async with asyncio.TaskGroup() as tasks:
                tasks.create_task(_device_to_provider(websocket, provider, send_lock, hello))
                tasks.create_task(_provider_to_device(websocket, provider, send_lock, hello))
    except TimeoutError:
        await _safe_send(
            websocket,
            encode_control("error", message="session expired", fatal=True),
            send_lock,
        )
    except (ProtocolError, ValueError) as exc:
        await _safe_send(websocket, encode_control("error", message=str(exc), fatal=True), send_lock)
    except Exception:
        LOGGER.exception("session failed for device %s", device_id)
        await _safe_send(
            websocket,
            encode_control("error", message="gateway session failed", fatal=True),
            send_lock,
        )
    finally:
        if provider is not None:
            await provider.close()
        if acquired:
            await runtime.release(device_id)
        LOGGER.info("device disconnected: %s", device_id)


async def _device_to_provider(
    websocket: Any,
    provider: GeminiLiveSession,
    lock: asyncio.Lock,
    hello: DeviceHello,
) -> None:
    decoder = OpusToPcmDecoder(INPUT_AUDIO.sample_rate) if hello.variant == "xiaozhi-opus" else None
    async for message in websocket:
        if isinstance(message, bytes):
            if decoder is None:
                INPUT_AUDIO.validate(message)
                await provider.send_audio(message)
            else:
                for pcm in decoder.decode(message):
                    if pcm:
                        await provider.send_audio(pcm)
            continue

        control = decode_control(message)
        event_type = control["type"]
        if event_type == "audio.end":
            await provider.end_audio()
        elif event_type == "listen" and control.get("state") == "stop":
            await provider.end_audio()
        elif event_type in {"listen", "abort", "mcp"}:
            # XiaoZhi control events are understood by the gateway. Gemini's
            # server VAD handles listen start/detect; tool bridging follows later.
            continue
        elif event_type == "text":
            text = control.get("text")
            if not isinstance(text, str):
                raise ProtocolError("text event requires a string text field")
            await provider.send_text(text)
        elif event_type == "ping":
            await _safe_send(websocket, encode_control("pong"), lock)
        else:
            raise ProtocolError(f"unsupported control message: {event_type}")


async def _provider_to_device(
    websocket: Any,
    provider: GeminiLiveSession,
    lock: asyncio.Lock,
    hello: DeviceHello,
) -> None:
    if hello.variant == "xiaozhi-opus":
        await _provider_to_xiaozhi(websocket, provider, lock)
        return
    async for event in provider.receive_forever():
        await _send_provider_event(websocket, event, lock)


async def _provider_to_xiaozhi(
    websocket: Any,
    provider: GeminiLiveSession,
    lock: asyncio.Lock,
) -> None:
    encoder = PcmToOpusEncoder(sample_rate=24_000, frame_duration_ms=60)
    tts_started = False
    async for event in provider.receive_forever():
        if event.kind in {"audio", "transcript.output"} and not tts_started:
            await _safe_send(websocket, encode_control("tts", state="start"), lock)
            tts_started = True

        if event.kind == "audio" and event.data:
            for packet in encoder.feed(event.data):
                await _safe_send(websocket, packet, lock)
        elif event.kind == "transcript.input" and event.text:
            await _safe_send(websocket, encode_control("stt", text=event.text), lock)
        elif event.kind == "transcript.output" and event.text:
            await _safe_send(
                websocket,
                encode_control("tts", state="sentence_start", text=event.text),
                lock,
            )
        elif event.kind == "turn.interrupted":
            encoder.discard()
            if tts_started:
                await _safe_send(websocket, encode_control("tts", state="stop"), lock)
                tts_started = False
        elif event.kind == "turn.complete":
            for packet in encoder.finish_turn():
                await _safe_send(websocket, packet, lock)
            if tts_started:
                await _safe_send(websocket, encode_control("tts", state="stop"), lock)
                tts_started = False


async def _send_provider_event(websocket: Any, event: ProviderEvent, lock: asyncio.Lock) -> None:
    if event.kind == "audio":
        if event.data:
            await _safe_send(websocket, event.data, lock)
        return
    payload: dict[str, Any] = {}
    if event.text is not None:
        payload["text"] = event.text
    await _safe_send(websocket, encode_control(event.kind, **payload), lock)


async def _safe_send(websocket: Any, message: str | bytes, lock: asyncio.Lock) -> None:
    try:
        async with lock:
            await websocket.send(message)
    except Exception:
        LOGGER.debug("could not send final gateway message", exc_info=True)
