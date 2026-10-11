import unittest
from types import SimpleNamespace

from echoear_gateway.config import GatewayConfig
from echoear_gateway.providers.gemini_live import GeminiLiveSession, build_live_config, from_response


class FakeSession:
    def __init__(self, responses=None):
        self.calls = []
        self.responses = list(responses or [])

    async def send_realtime_input(self, **kwargs):
        self.calls.append(kwargs)

    async def receive(self):
        while self.responses:
            yield self.responses.pop(0)


class FakeContext:
    def __init__(self, session):
        self.session = session
        self.closed = False

    async def __aenter__(self):
        return self.session

    async def __aexit__(self, exc_type, exc, tb):
        self.closed = True


class FakeLive:
    def __init__(self, context):
        self.context = context
        self.connect_args = None

    def connect(self, **kwargs):
        self.connect_args = kwargs
        return self.context


class GeminiTests(unittest.IsolatedAsyncioTestCase):
    async def test_connect_send_audio_end_text_and_close(self):
        sdk_session = FakeSession()
        context = FakeContext(sdk_session)
        live = FakeLive(context)
        client = SimpleNamespace(aio=SimpleNamespace(live=live))
        config = GatewayConfig(api_key="secret", voice_name="Kore")
        wrapper = GeminiLiveSession(
            config,
            client_factory=lambda key: client,
            blob_factory=lambda data: (data, "audio/pcm;rate=16000"),
        )

        await wrapper.connect()
        await wrapper.send_audio(b"\x00\x00" * 10)
        await wrapper.end_audio()
        await wrapper.send_text(" สวัสดี ")
        await wrapper.close()

        self.assertEqual(live.connect_args["model"], config.model)
        self.assertEqual(sdk_session.calls[0]["audio"][1], "audio/pcm;rate=16000")
        self.assertEqual(sdk_session.calls[1], {"audio_stream_end": True})
        self.assertEqual(sdk_session.calls[2], {"text": "สวัสดี"})
        self.assertTrue(context.closed)

    def test_live_config_enables_audio_transcripts_and_voice(self):
        config = build_live_config(GatewayConfig(api_key="secret", voice_name="Kore"))
        self.assertEqual(config["response_modalities"], ["AUDIO"])
        self.assertIn("input_audio_transcription", config)
        self.assertIn("output_audio_transcription", config)
        self.assertEqual(
            config["speech_config"]["voice_config"]["prebuilt_voice_config"]["voice_name"],
            "Kore",
        )

    def test_response_is_translated_to_device_events(self):
        content = SimpleNamespace(
            input_transcription=SimpleNamespace(text="สวัสดี"),
            output_transcription=SimpleNamespace(text="สวัสดีค่ะ"),
            model_turn=SimpleNamespace(
                parts=[SimpleNamespace(inline_data=SimpleNamespace(data=b"audio"))]
            ),
            interrupted=False,
            turn_complete=True,
        )
        events = from_response(SimpleNamespace(server_content=content))
        self.assertEqual(
            [event.kind for event in events],
            ["transcript.input", "transcript.output", "audio", "turn.complete"],
        )
        self.assertEqual(events[2].data, b"audio")

    async def test_receive_forever_yields_individual_events(self):
        content = SimpleNamespace(
            input_transcription=None,
            output_transcription=SimpleNamespace(text="ตอบแล้ว"),
            model_turn=SimpleNamespace(parts=[]),
            interrupted=False,
            turn_complete=True,
        )
        sdk_session = FakeSession([SimpleNamespace(server_content=content)])
        context = FakeContext(sdk_session)
        live = FakeLive(context)
        client = SimpleNamespace(aio=SimpleNamespace(live=live))
        wrapper = GeminiLiveSession(
            GatewayConfig(api_key="secret"),
            client_factory=lambda key: client,
        )
        await wrapper.connect()

        received = []
        async for event in wrapper.receive_forever():
            received.append(event)
            if event.kind == "turn.complete":
                break

        self.assertEqual(
            [event.kind for event in received],
            ["transcript.output", "turn.complete"],
        )
        await wrapper.close()


if __name__ == "__main__":
    unittest.main()
