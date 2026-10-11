import json
import unittest

from echoear_gateway.protocol import (
    ProtocolError,
    decode_control,
    parse_hello,
    ready_message,
    validate_hello,
    xiaozhi_ready_message,
)


class ProtocolTests(unittest.TestCase):
    def test_hello_and_token(self):
        device_id = validate_hello(
            {"type": "hello", "protocol": 1, "device_id": "echoear-test", "token": "abc"},
            "abc",
        )
        self.assertEqual(device_id, "echoear-test")

    def test_wrong_token_is_rejected(self):
        with self.assertRaisesRegex(ProtocolError, "token"):
            validate_hello({"type": "hello", "protocol": 1, "device_id": "x"}, "abc")

    def test_ready_declares_both_audio_rates(self):
        message = json.loads(ready_message("gemini", "test-model"))
        self.assertEqual(message["input_audio"]["sample_rate"], 16000)
        self.assertEqual(message["output_audio"]["sample_rate"], 24000)

    def test_invalid_json_is_rejected(self):
        with self.assertRaises(ProtocolError):
            decode_control("not-json")

    def test_accepts_xiaozhi_opus_hello(self):
        hello = parse_hello(
            {
                "type": "hello",
                "version": 1,
                "transport": "websocket",
                "audio_params": {"format": "opus", "sample_rate": 16000},
            }
        )
        self.assertEqual(hello.variant, "xiaozhi-opus")

        response = json.loads(xiaozhi_ready_message())
        self.assertEqual(response["type"], "hello")
        self.assertEqual(response["audio_params"]["format"], "opus")
        self.assertEqual(response["audio_params"]["sample_rate"], 24000)

    def test_xiaozhi_uses_device_id_from_transport_header(self):
        hello = parse_hello(
            {
                "type": "hello",
                "version": 1,
                "transport": "websocket",
                "audio_params": {"format": "opus", "sample_rate": 16000},
            },
            device_id_override="30:ED:A0:1E:86:BC",
        )
        self.assertEqual(hello.device_id, "30:ed:a0:1e:86:bc")

    def test_accepts_supported_voice_and_rejects_unknown_voice(self):
        hello = parse_hello(
            {
                "type": "hello",
                "version": 1,
                "transport": "websocket",
                "voice": "Leda",
                "audio_params": {"format": "opus", "sample_rate": 16000},
            }
        )
        self.assertEqual(hello.voice_name, "Leda")

        with self.assertRaisesRegex(ProtocolError, "voice"):
            parse_hello(
                {
                    "type": "hello",
                    "version": 1,
                    "transport": "websocket",
                    "voice": "unknown",
                    "audio_params": {"format": "opus", "sample_rate": 16000},
                }
            )


if __name__ == "__main__":
    unittest.main()
