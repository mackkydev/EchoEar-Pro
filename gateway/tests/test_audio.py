import unittest

from echoear_gateway.audio import INPUT_AUDIO, RECOMMENDED_INPUT_CHUNK_BYTES


class AudioTests(unittest.TestCase):
    def test_recommended_packet_is_100ms_pcm16_mono(self):
        self.assertEqual(RECOMMENDED_INPUT_CHUNK_BYTES, 3200)

    def test_rejects_partial_sample(self):
        with self.assertRaisesRegex(ValueError, "PCM frame"):
            INPUT_AUDIO.validate(b"123")

    def test_accepts_complete_samples(self):
        INPUT_AUDIO.validate(b"\x00\x00" * 1600)


if __name__ == "__main__":
    unittest.main()
