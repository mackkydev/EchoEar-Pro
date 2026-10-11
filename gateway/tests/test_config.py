import unittest
from pathlib import Path
import tempfile

from echoear_gateway.config import GatewayConfig


class ConfigTests(unittest.TestCase):
    def test_requires_api_key(self):
        with self.assertRaisesRegex(ValueError, "GEMINI_API_KEY"):
            GatewayConfig.from_environment({})

    def test_secret_is_not_in_repr(self):
        config = GatewayConfig.from_environment({"GEMINI_API_KEY": "top-secret"})
        self.assertNotIn("top-secret", repr(config))

    def test_thai_persona_and_custom_name(self):
        config = GatewayConfig.from_environment(
            {"GEMINI_API_KEY": "key", "ECHOEAR_ASSISTANT_NAME": "Robo"}
        )
        self.assertIn("Robo", config.system_instruction)
        self.assertIn("Thai", config.system_instruction)

    def test_female_voice_uses_feminine_thai_persona(self):
        config = GatewayConfig(api_key="secret", voice_name="Leda")

        self.assertIn("ค่ะ", config.system_instruction)
        self.assertIn("never use ครับ", config.system_instruction)

    def test_male_voice_uses_masculine_thai_persona(self):
        config = GatewayConfig(api_key="secret", voice_name="Achird")

        self.assertIn("ครับ", config.system_instruction)
        self.assertNotIn("never use ครับ", config.system_instruction)

    def test_loads_local_env_file_without_exposing_key(self):
        with tempfile.TemporaryDirectory() as directory:
            env_file = Path(directory) / ".env"
            env_file.write_text(
                "# gateway secret\nGEMINI_API_KEY=file-secret\nECHOEAR_GATEWAY_PORT=9876\n",
                encoding="utf-8",
            )
            config = GatewayConfig.from_environment(env_file=env_file)

        self.assertEqual(config.api_key, "file-secret")
        self.assertEqual(config.port, 9876)
        self.assertNotIn("file-secret", repr(config))

    def test_production_requires_per_device_credentials(self):
        with self.assertRaisesRegex(ValueError, "production requires"):
            GatewayConfig.from_environment(
                {"GEMINI_API_KEY": "secret", "ECHOEAR_ENVIRONMENT": "production"}
            )

    def test_production_authenticates_each_device_with_its_own_token(self):
        token = "a" * 32
        config = GatewayConfig.from_environment(
            {
                "GEMINI_API_KEY": "secret",
                "ECHOEAR_ENVIRONMENT": "production",
                "ECHOEAR_DEVICE_CREDENTIALS_JSON": '{"AA:BB:CC":"' + token + '"}',
            }
        )

        self.assertTrue(config.authenticate_device("aa:bb:cc", token))
        self.assertFalse(config.authenticate_device("aa:bb:cc", "wrong"))
        self.assertFalse(config.authenticate_device("unknown", token))
        self.assertNotIn(token, repr(config))

    def test_production_rejects_short_device_tokens(self):
        with self.assertRaisesRegex(ValueError, "at least 32"):
            GatewayConfig.from_environment(
                {
                    "GEMINI_API_KEY": "secret",
                    "ECHOEAR_ENVIRONMENT": "production",
                    "ECHOEAR_DEVICE_CREDENTIALS_JSON": '{"device":"short"}',
                }
            )


if __name__ == "__main__":
    unittest.main()
