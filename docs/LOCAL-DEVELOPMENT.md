# Local development

## ESPHome Dashboard

Copy the repository into the ESPHome configuration directory or mount it into
the ESPHome Docker container. Create a local secrets file:

```sh
cp secrets.example.yaml secrets.yaml
cp ceilsense-pro-v2/secrets.example.yaml ceilsense-pro-v2/secrets.yaml
```

Fill in `wifi_ssid` and `wifi_password` in the generation you are building.
Use `ceilsense-pro-wifi-local.yaml` for V1 C6 or
`ceilsense-pro-v2/ceilsense-pro-v2-wifi-local.yaml` for the current V2 S3 board.
Both local files use a stable hostname.

## Command-line build with Docker

```sh
docker run --rm -v "$PWD":/config -w /config \
  esphome/esphome:2026.6.1 config ceilsense-pro-wifi-local.yaml

docker run --rm -v "$PWD":/config -w /config \
  esphome/esphome:2026.6.1 compile ceilsense-pro-wifi-local.yaml

docker run --rm -v "$PWD":/config -w /config \
  esphome/esphome:2026.6.1 compile \
  ceilsense-pro-v2/ceilsense-pro-v2-wifi-local.yaml
```

## Flashing

Connect the board over USB-C and install the local WiFi variant from ESPHome
Device Builder. After the first USB installation, subsequent builds can be sent
over OTA while the device remains reachable.

Production WiFi variants use captive portal and serial Improv onboarding and
add a MAC suffix. Local variants read fixed credentials from `secrets.yaml` and
do not add the suffix.

## Logs

ESPHome API logs are the preferred method after the device joins the network.
For V1 USB logs, select the board's USB-to-UART serial port. V2 uses ESP32-S3
native USB on GPIO19/GPIO20 and exposes BOOT and RESET buttons for recovery.

## Validation commands

See [CONTRIBUTING.md](../CONTRIBUTING.md) for the complete local command set and
[VALIDATION.md](VALIDATION.md) for hardware acceptance checks.
