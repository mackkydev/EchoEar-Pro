import unittest
from types import SimpleNamespace

from echoear_gateway.config import GatewayConfig
from echoear_gateway.audio_opus import PcmToOpusEncoder
from echoear_gateway.providers.base import ProviderEvent
from echoear_gateway.server import handle_device


class FakeWebSocket:
    def __init__(self, incoming, headers=None):
        self.incoming = list(incoming)
        self.sent = []
        self.request = SimpleNamespace(headers=headers or {})

    async def recv(self):
        return self.incoming.pop(0)

    def __aiter__(self):
        return self

    async def __anext__(self):
        if not self.incoming:
            raise StopAsyncIteration
        return self.incoming.pop(0)

    async def send(self, message):
        self.sent.append(message)


class FakeProvider:
    def __init__(self, config):
        self.config = config
        self.connected = False
        self.closed = False
        self.audio = []
        self.text = []
        self.ended = 0

    async def connect(self):
        self.connected = True

    async def close(self):
        self.closed = True

    async def send_audio(self, data):
        self.audio.append(data)

    async def send_text(self, text):
        self.text.append(text)

    async def end_audio(self):
        self.ended += 1

    async def receive_forever(self):
        yield ProviderEvent(kind="transcript.output", text="สวัสดีค่ะ")
        yield ProviderEvent.audio(b"reply-audio")
        yield ProviderEvent(kind="turn.complete")


class FakeXiaozhiProvider(FakeProvider):
    async def receive_forever(self):
        yield ProviderEvent(kind="transcript.input", text="สวัสดี")
        yield ProviderEvent(kind="transcript.output", text="สวัสดีค่ะ")
        yield ProviderEvent.audio(bytes(24_000 * 2 * 60 // 1000))
        yield ProviderEvent(kind="turn.complete")


class ServerTests(unittest.IsolatedAsyncioTestCase):
    async def test_complete_device_provider_round_trip(self):
        websocket = FakeWebSocket(
            [
                '{"type":"hello","protocol":1,"device_id":"echoear-test"}',
                b"\x00\x00" * 10,
                '{"type":"audio.end"}',
                '{"type":"text","text":"ทดสอบ"}',
            ]
        )
        created = []

        def provider_factory(config):
            provider = FakeProvider(config)
            created.append(provider)
            return provider

        await handle_device(
            websocket,
            GatewayConfig(api_key="secret"),
            provider_factory=provider_factory,
        )

        provider = created[0]
        self.assertTrue(provider.connected)
        self.assertTrue(provider.closed)
        self.assertEqual(provider.audio, [b"\x00\x00" * 10])
        self.assertEqual(provider.ended, 1)
        self.assertEqual(provider.text, ["ทดสอบ"])
        self.assertIn('"type":"ready"', websocket.sent[0])
        self.assertIn('"type":"transcript.output"', websocket.sent[1])
        self.assertEqual(websocket.sent[2], b"reply-audio")
        self.assertIn('"type":"turn.complete"', websocket.sent[3])

    async def test_bad_gateway_token_never_connects_provider(self):
        websocket = FakeWebSocket(
            ['{"type":"hello","protocol":1,"device_id":"x","token":"wrong"}']
        )
        created = []

        await handle_device(
            websocket,
            GatewayConfig(api_key="secret", gateway_token="right"),
            provider_factory=lambda config: created.append(config),
        )

        self.assertEqual(created, [])
        self.assertIn('"type":"error"', websocket.sent[0])
        self.assertIn('"fatal":true', websocket.sent[0])

    async def test_xiaozhi_opus_device_round_trip(self):
        input_encoder = PcmToOpusEncoder(sample_rate=16_000, frame_duration_ms=60)
        input_packet = input_encoder.feed(bytes(16_000 * 2 * 60 // 1000))[0]
        websocket = FakeWebSocket(
            [
                '{"type":"hello","version":1,"transport":"websocket",'
                '"audio_params":{"format":"opus","sample_rate":16000}}',
                input_packet,
                '{"type":"listen","state":"stop"}',
            ]
        )
        created = []

        def provider_factory(config):
            provider = FakeXiaozhiProvider(config)
            created.append(provider)
            return provider

        await handle_device(
            websocket,
            GatewayConfig(api_key="secret"),
            provider_factory=provider_factory,
        )

        provider = created[0]
        self.assertTrue(provider.audio)
        self.assertTrue(all(len(chunk) % 2 == 0 for chunk in provider.audio))
        self.assertEqual(provider.ended, 1)
        self.assertIn('"type":"hello"', websocket.sent[0])
        self.assertIn('"type":"stt"', websocket.sent[1])
        self.assertIn('"state":"start"', websocket.sent[2])
        self.assertIn('"state":"sentence_start"', websocket.sent[3])
        self.assertIsInstance(websocket.sent[4], bytes)
        self.assertIn('"state":"stop"', websocket.sent[5])

    async def test_device_voice_overrides_gateway_default_for_session(self):
        websocket = FakeWebSocket(
            [
                '{"type":"hello","version":1,"transport":"websocket",'
                '"voice":"Leda",'
                '"audio_params":{"format":"opus","sample_rate":16000}}',
            ]
        )
        created = []

        def provider_factory(config):
            provider = FakeXiaozhiProvider(config)
            created.append(provider)
            return provider

        await handle_device(
            websocket,
            GatewayConfig(api_key="secret", voice_name="Kore"),
            provider_factory=provider_factory,
        )

        self.assertEqual(created[0].config.voice_name, "Leda")

    async def test_production_accepts_device_id_and_bearer_token_headers(self):
        token = "a" * 32
        websocket = FakeWebSocket(
            [
                '{"type":"hello","version":1,"transport":"websocket",'
                '"audio_params":{"format":"opus","sample_rate":16000}}',
            ],
            headers={
                "Device-Id": "30:ED:A0:1E:86:BC",
                "Authorization": f"Bearer {token}",
            },
        )
        created = []

        def provider_factory(config):
            provider = FakeXiaozhiProvider(config)
            created.append(provider)
            return provider

        config = GatewayConfig.from_environment(
            {
                "GEMINI_API_KEY": "secret",
                "ECHOEAR_ENVIRONMENT": "production",
                "ECHOEAR_DEVICE_CREDENTIALS_JSON":
                    '{"30:ed:a0:1e:86:bc":"' + token + '"}',
            }
        )
        await handle_device(websocket, config, provider_factory=provider_factory)

        self.assertEqual(len(created), 1)
        self.assertIn('"type":"hello"', websocket.sent[0])


if __name__ == "__main__":
    unittest.main()
