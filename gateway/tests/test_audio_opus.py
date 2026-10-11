import unittest

from echoear_gateway.audio_opus import OpusToPcmDecoder, PcmToOpusEncoder


class OpusAudioTests(unittest.TestCase):
    def test_encoder_buffers_arbitrary_chunks_into_60ms_packets(self):
        encoder = PcmToOpusEncoder(sample_rate=24_000, frame_duration_ms=60)
        pcm = bytes(24_000 * 2 * 120 // 1000)

        packets = encoder.feed(pcm[:1000])
        packets += encoder.feed(pcm[1000:])

        self.assertEqual(len(packets), 2)
        self.assertTrue(all(packet for packet in packets))

    def test_finish_turn_pads_only_the_last_partial_frame(self):
        encoder = PcmToOpusEncoder(sample_rate=24_000, frame_duration_ms=60)
        self.assertEqual(encoder.feed(bytes(1000)), [])
        self.assertEqual(len(encoder.finish_turn()), 1)
        self.assertEqual(encoder.finish_turn(), [])

    def test_xiaozhi_opus_round_trip_produces_pcm16_at_16khz(self):
        encoder = PcmToOpusEncoder(sample_rate=16_000, frame_duration_ms=60)
        packets = encoder.feed(bytes(16_000 * 2 * 120 // 1000))
        decoder = OpusToPcmDecoder(output_rate=16_000)

        pcm_chunks = [chunk for packet in packets for chunk in decoder.decode(packet)]

        self.assertEqual(len(packets), 2)
        self.assertTrue(pcm_chunks)
        self.assertTrue(all(len(chunk) % 2 == 0 for chunk in pcm_chunks))
        total_samples = sum(len(chunk) // 2 for chunk in pcm_chunks)
        self.assertGreaterEqual(total_samples, 1_800)
        self.assertLessEqual(total_samples, 1_920)


if __name__ == "__main__":
    unittest.main()
