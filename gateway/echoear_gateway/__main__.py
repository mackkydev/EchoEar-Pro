import asyncio
import logging

from .config import GatewayConfig
from .server import serve


def main() -> None:
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s %(levelname)s %(name)s: %(message)s",
    )
    config = GatewayConfig.from_environment()
    try:
        asyncio.run(serve(config))
    except KeyboardInterrupt:
        logging.getLogger("echoear.gateway").info("Gateway stopped")


if __name__ == "__main__":
    main()
