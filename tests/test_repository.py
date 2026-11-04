"""Repository-level safeguards for the CeilSense Pro firmware."""

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[1]
ENTRYPOINTS = {
    "ceilsense-pro-wifi.yaml": "packages/network_wifi.yaml",
    "ceilsense-pro-wifi-local.yaml": "packages/network_wifi_local.yaml",
    "ceilsense-pro-poe.yaml": "packages/network_poe.yaml",
}
V2_ENTRYPOINTS = {
    "ceilsense-pro-v2-wifi-ld2450-ld2412.yaml": (
        "network-wifi.yaml",
        "radar-ld2450-ld2412.yaml",
    ),
    "ceilsense-pro-v2-ethernet-ld2450-ld2412.yaml": (
        "network-ethernet.yaml",
        "radar-ld2450-ld2412.yaml",
    ),
    "ceilsense-pro-v2-wifi-ld2460.yaml": ("network-wifi.yaml", "radar-ld2460.yaml"),
    "ceilsense-pro-v2-ethernet-ld2460.yaml": (
        "network-ethernet.yaml",
        "radar-ld2460.yaml",
    ),
    "ceilsense-pro-v2-wifi-ld6002b.yaml": ("network-wifi.yaml", "radar-ld6002b.yaml"),
    "ceilsense-pro-v2-ethernet-ld6002b.yaml": (
        "network-ethernet.yaml",
        "radar-ld6002b.yaml",
    ),
    "ceilsense-pro-v2-wifi-local.yaml": (
        "network-wifi-local.yaml",
        "radar-ld2450-ld2412.yaml",
    ),
}
REQUIRED_PACKAGES = {
    "packages/base.yaml",
    "packages/audio_max98357.yaml",
    "packages/buzzer.yaml",
    "packages/scd41.yaml",
    "packages/sgp41.yaml",
    "packages/adpd188bi.yaml",
    "packages/smoke_alarm.yaml",
    "packages/ld2412.yaml",
    "packages/ld2450.yaml",
    "packages/presence.yaml",
}


class RepositoryTest(unittest.TestCase):
    def test_entrypoints_include_complete_hardware_stack(self):
        for entrypoint, network_package in ENTRYPOINTS.items():
            text = (ROOT / entrypoint).read_text()
            with self.subTest(entrypoint=entrypoint):
                for package in REQUIRED_PACKAGES | {network_package}:
                    self.assertIn(package, text)

        v2 = ROOT / "ceilsense-pro-v2"
        for entrypoint, packages in V2_ENTRYPOINTS.items():
            text = (v2 / entrypoint).read_text()
            with self.subTest(entrypoint=entrypoint):
                self.assertIn("device-common.yaml", text)
                for package in packages:
                    self.assertIn(package, text)

    def test_v2_common_stack_has_voice_speaker_buzzer_and_smoke(self):
        text = (ROOT / "ceilsense-pro-v2/device-common.yaml").read_text()
        for package in (
            "audio-voice-hardware.yaml",
            "voice-assistant.yaml",
            "../packages/buzzer.yaml",
            "../packages/adpd188bi.yaml",
            "../packages/smoke_alarm.yaml",
            "environment.yaml",
            "presence.yaml",
        ):
            self.assertIn(package, text)

    def test_v2_radar_variants_use_schematic_uart_pins(self):
        v2 = ROOT / "ceilsense-pro-v2"
        standard = (v2 / "radar-ld2450-ld2412.yaml").read_text()
        ld2460 = (v2 / "radar-ld2460.yaml").read_text()
        ld6002b = (v2 / "radar-ld6002b.yaml").read_text()
        self.assertIn("ld2450_uart_tx_pin: GPIO6", standard)
        self.assertIn("ld2450_uart_rx_pin: GPIO7", standard)
        self.assertIn("tx_pin: GPIO39", standard)
        self.assertIn("rx_pin: GPIO38", standard)
        self.assertIn("ld2460_tx_pin: GPIO39", ld2460)
        self.assertIn("ld2460_rx_pin: GPIO38", ld2460)
        self.assertIn("tx_pin: GPIO39", ld6002b)
        self.assertIn("rx_pin: GPIO38", ld6002b)

    def test_project_version_is_semver(self):
        for path in (ROOT / "packages/base.yaml", ROOT / "ceilsense-pro-v2/base.yaml"):
            text = path.read_text()
            match = re.search(r'project_version:\s*"([^"]+)"', text)
            with self.subTest(path=path):
                self.assertIsNotNone(match)
                self.assertRegex(match.group(1), r"^\d+\.\d+\.\d+$")

    def test_external_packages_are_immutably_pinned(self):
        paths = list((ROOT / "packages").glob("*.yaml"))
        paths += list((ROOT / "ceilsense-pro-v2").glob("*.yaml"))
        for path in paths:
            text = path.read_text()
            if "url: https://github.com/" not in text:
                continue
            with self.subTest(path=path.name):
                refs = re.findall(r"^\s*ref:\s*(\S+)", text, re.MULTILINE)
                self.assertTrue(refs)
                for ref in refs:
                    self.assertRegex(
                        ref,
                        r"^(v\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?|[0-9a-f]{40})$",
                    )

    def test_smoke_feature_remains_explicitly_experimental(self):
        readme = (ROOT / "README.md").read_text().lower()
        smoke_docs = (ROOT / "docs/SMOKE-DEVELOPMENT.md").read_text().lower()
        for text in (readme, smoke_docs):
            self.assertIn("not a certified smoke alarm", text)
            self.assertIn("adpd188bi", text)

    def test_sensitive_local_files_are_ignored(self):
        ignore = (ROOT / ".gitignore").read_text().splitlines()
        self.assertIn("secrets.yaml", ignore)
        self.assertIn("build/", ignore)
        self.assertIn("**/.esphome/", ignore)

    def test_no_plaintext_wifi_values_in_firmware(self):
        paths = list(ROOT.glob("*.yaml")) + list((ROOT / "packages").glob("*.yaml"))
        paths += list((ROOT / "ceilsense-pro-v2").glob("*.yaml"))
        for path in paths:
            text = path.read_text()
            with self.subTest(path=path.name):
                self.assertNotRegex(text, r"(?mi)^\s*(ssid|password):\s*['\"](?!\$\{|!secret)[^'\"]+['\"]")


if __name__ == "__main__":
    unittest.main()
