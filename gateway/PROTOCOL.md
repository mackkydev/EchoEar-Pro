# EchoEar Gateway protocol v1

The device opens a WebSocket and sends this JSON message first:

```json
{"type":"hello","protocol":1,"device_id":"echoear-30eda01e86bc"}
```

The gateway answers with:

```json
{
  "type":"ready",
  "protocol":1,
  "provider":"gemini",
  "input_audio":{"encoding":"pcm_s16le","sample_rate":16000,"channels":1},
  "output_audio":{"encoding":"pcm_s16le","sample_rate":24000,"channels":1}
}
```

After `ready`, each binary message is one input-audio chunk. The recommended
size is 3,200 bytes (100 ms). The device sends `{"type":"audio.end"}` when
local voice detection sees the end of speech. It may send
`{"type":"text","text":"..."}` for simulator tests.

Gateway output uses binary messages for output-audio chunks and JSON events:

- `transcript.input` — recognized user speech
- `transcript.output` — assistant speech text for the UI ticker
- `turn.complete` — the response turn ended
- `turn.interrupted` — the current response was interrupted
- `error` — recoverable or fatal session error

Optional authentication uses `ECHOEAR_GATEWAY_TOKEN`. When configured, the
device includes the same value as `token` in `hello`. This is separate from the
Gemini API key, which always remains on the gateway.

For production, shared-token authentication is replaced by a unique credential
per device. XiaoZhi-compatible firmware sends:

```text
Device-Id: 30:ed:a0:1e:86:bc
Authorization: Bearer <unique-device-token>
```

The Gateway matches the normalized device ID against its private credential
registry before opening a Gemini session. Unknown devices, missing tokens, and
duplicate sessions are rejected before any provider cost is incurred.
