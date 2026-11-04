# Local development

## ESPHome Dashboard

Copy the repository into the ESPHome configuration directory or mount it into
the ESPHome Docker container. For the local WiFi firmware, create a secrets
file and fill in `wifi_ssid` and `wifi_password`:

```sh
cp ceilsense-pro-v1/secrets.example.yaml ceilsense-pro-v1/secrets.yaml
```

Use `ceilsense-pro-v1/ceilsense-pro-v1-wifi-local.yaml` for development on a
fixed WiFi network. Production WiFi firmware uses captive-portal and serial
Improv onboarding instead of credentials stored in the configuration.

## Command-line build with Docker

```sh
docker run --rm -v "$PWD":/config -w /config \
  esphome/esphome:2026.6.1 config ceilsense-pro-wifi.yaml

docker run --rm -v "$PWD":/config -w /config \
  esphome/esphome:2026.6.1 compile ceilsense-pro-ethernet.yaml

docker run --rm -v "$PWD":/config -w /config \
  esphome/esphome:2026.6.1 compile \
  ceilsense-pro-v1/ceilsense-pro-v1-wifi-local.yaml
```

## Flashing and logs

Connect the board over USB-C and install the matching firmware from ESPHome
Device Builder. After the first USB installation, subsequent builds can be
sent over OTA while the device remains reachable.

ESPHome API logs are preferred after the device joins the network. USB logs
use the ESP32-S3 native USB Serial/JTAG interface on GPIO19/GPIO20. BOOT and
RESET are available for recovery when automatic USB flashing cannot start the
download mode.

## Validation commands

See [CONTRIBUTING.md](../CONTRIBUTING.md) for the complete local command set and
[VALIDATION.md](VALIDATION.md) for hardware acceptance checks.
