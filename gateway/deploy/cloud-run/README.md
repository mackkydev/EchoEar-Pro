# EchoEar Gateway on Google Cloud Run

Cloud Run terminates HTTPS/WSS at Google's edge. The container listens on the
injected `PORT`; the ESP32 connects to the resulting `wss://...run.app` URL.

## One-time Google Cloud setup

Use a dedicated production project and enable Cloud Run, Artifact Registry,
Cloud Build, and Secret Manager. Create two Secret Manager secrets:

- `echoear-gemini-api-key`: the server-side Gemini API key
- `echoear-device-credentials`: a JSON object mapping device IDs to unique tokens

The credential JSON format is shown in `gateway/devices.json.example`. Give the
Cloud Run service account Secret Manager access only to these two secrets.

## Build and deploy

Run from the `gateway` directory after choosing the project and region:

```powershell
gcloud builds submit --tag REGION-docker.pkg.dev/PROJECT/echoear/gateway:VERSION
gcloud run deploy echoear-gateway `
  --image REGION-docker.pkg.dev/PROJECT/echoear/gateway:VERSION `
  --region REGION `
  --allow-unauthenticated `
  --port 8080 `
  --min-instances 0 `
  --max-instances 1 `
  --concurrency 20 `
  --timeout 3600 `
  --set-env-vars ECHOEAR_ENVIRONMENT=production,ECHOEAR_MAX_ACTIVE_SESSIONS=20,ECHOEAR_MAX_SESSIONS_PER_DEVICE=1,ECHOEAR_SESSION_MAX_SECONDS=3300 `
  --set-secrets GEMINI_API_KEY=echoear-gemini-api-key:1,ECHOEAR_DEVICE_CREDENTIALS_JSON=echoear-device-credentials:1
```

`--allow-unauthenticated` permits the ESP32 WebSocket handshake to reach the
container; the Gateway still requires and verifies the per-device bearer token.
Do not use a shared token across customer devices.

The pilot starts at zero instances to avoid idle cost and is capped at one
instance so in-memory per-device session limits remain authoritative. Before
scaling beyond one instance, move quotas and session ownership to a shared
database or cache. Pin secret versions and deploy a new revision when rotating
credentials.

## Verify before provisioning firmware

```powershell
curl.exe https://YOUR-SERVICE-URL/
```

Expected status is HTTP 200 with `{"status":"ok",...}`. Then store these values
in each device during manufacturing or activation:

- `echoear_ai/gateway_url`: the Cloud Run URL with `https://` changed to `wss://`
  and `/ws` appended
- `echoear_ai/gateway_token`: that device's unique token

Do not use `/healthz` or `/readyz` for the public Cloud Run probe. Those paths
can be handled by the Google frontend before the request reaches the container.

The current JSON registry is suitable for a pilot. Before a large public launch,
move device identities, revocation, subscriptions, and quotas to a transactional
database-backed activation service.
