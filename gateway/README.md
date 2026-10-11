# EchoEar Gateway

EchoEar Gateway keeps cloud credentials off the ESP32 and translates the
EchoEar device stream to an AI provider. The first provider is Gemini Live.

## Audio contract

- Device to gateway: mono PCM16 little-endian at 16 kHz
- Gateway to device: mono PCM16 little-endian at 24 kHz
- Recommended input packet: 100 ms (3,200 bytes)
- Control messages are JSON; audio packets are binary WebSocket messages

See `PROTOCOL.md` for the complete device-facing contract.

## Local setup

Use Python 3.11 or newer. Keep the API key on this computer or server; never
compile it into firmware.

```powershell
cd D:\2026-Projects\EchoEar-Pro\gateway
python -m venv .venv
.venv\Scripts\python -m pip install -r requirements.txt
$env:GEMINI_API_KEY = "your-key-kept-on-this-computer"
.venv\Scripts\python -m echoear_gateway
```

The gateway listens on `ws://0.0.0.0:8765` by default. Configuration is read
from `gateway/.env` and may be overridden by environment variables listed in
`.env.example`. The `.env` file is ignored by Git and must never be shared.

Local health checks are available on the same port at `/`, `/healthz`, and
`/readyz`. On Cloud Run, use `/` because the Google frontend reserves the
Kubernetes-style health paths. WebSocket clients should connect to `/ws`.

## Production mode

Production mode fails closed unless a per-device credential registry is
configured. Every device sends its immutable ID in `Device-Id` and its unique
token in `Authorization: Bearer ...`. The Gemini key remains only on the server.

```text
ECHOEAR_ENVIRONMENT=production
ECHOEAR_DEVICE_CREDENTIALS_FILE=devices.json
ECHOEAR_MAX_ACTIVE_SESSIONS=100
ECHOEAR_MAX_SESSIONS_PER_DEVICE=1
ECHOEAR_SESSION_MAX_SECONDS=3600
```

Create or rotate a pilot-device token with:

```powershell
.venv\Scripts\python scripts\provision_device.py 30:ed:a0:1e:86:bc
```

`devices.json` is private and ignored by Git. See `deploy/cloud-run/README.md`
for the TLS/WSS Cloud Run deployment. The JSON registry is intentionally a
pilot-stage store; production fleet activation and revocation should use a
database rather than rebuilding a secret for every change.

## Tests

The unit tests do not contact Gemini and do not require an API key.

```powershell
python -m unittest discover -s tests -v
```

After setting `GEMINI_API_KEY`, a direct cloud smoke test can save Gemini's
24 kHz response as a WAV file before any firmware is changed:

```powershell
.venv\Scripts\python scripts\gemini_smoke_test.py --output output\gemini-thai.wav
```

On Windows, the safer interactive wrapper hides the key while it is typed and
removes it from the process immediately after the test:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\gemini_smoke_test.ps1
```

To exercise the same XiaoZhi-compatible Opus/WebSocket path used by the ESP32:

```powershell
.venv\Scripts\python scripts\xiaozhi_gateway_smoke_test.py
```
