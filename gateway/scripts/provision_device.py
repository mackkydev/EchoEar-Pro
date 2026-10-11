from __future__ import annotations

import argparse
import json
from pathlib import Path
import secrets


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Create or rotate one EchoEar device credential."
    )
    parser.add_argument("device_id", help="Device MAC address or immutable device ID")
    parser.add_argument(
        "--registry",
        type=Path,
        default=Path("devices.json"),
        help="Private JSON registry (default: devices.json)",
    )
    parser.add_argument(
        "--quiet",
        action="store_true",
        help="Do not print the generated token (recommended for automation)",
    )
    args = parser.parse_args()
    device_id = args.device_id.strip().lower()
    if not device_id:
        parser.error("device_id cannot be empty")

    registry: dict[str, str] = {}
    if args.registry.exists():
        payload = json.loads(args.registry.read_text(encoding="utf-8"))
        if not isinstance(payload, dict) or not all(
            isinstance(key, str) and isinstance(value, str)
            for key, value in payload.items()
        ):
            raise SystemExit("registry must contain a JSON object of device tokens")
        registry.update(payload)

    token = secrets.token_urlsafe(32)
    registry[device_id] = token
    args.registry.write_text(
        json.dumps(registry, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(f"Device: {device_id}")
    if not args.quiet:
        print(f"Token:  {token}")
    print(f"Saved private registry: {args.registry.resolve()}")
    print("Store this token in the device's echoear_ai/gateway_token setting.")


if __name__ == "__main__":
    main()
