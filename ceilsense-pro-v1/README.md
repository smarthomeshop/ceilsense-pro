# CeilSense Pro V1

CeilSense Pro V1 is the first CeilSense Pro hardware release. It uses an
ESP32-S3-WROOM-1-N16R8 with native USB, PSRAM, microphone, speaker, buzzer,
16-pixel LED ring, environmental sensors and the experimental ADPD188BI
optical aerosol sensor.

## Firmware variants

| Radar assembly | WiFi | PoE/Ethernet |
| --- | --- | --- |
| LD2450 + LD2412 (standard) | `ceilsense-pro-v1-wifi-ld2450-ld2412.yaml` | `ceilsense-pro-v1-ethernet-ld2450-ld2412.yaml` |
| LD2460 | `ceilsense-pro-v1-wifi-ld2460.yaml` | `ceilsense-pro-v1-ethernet-ld2460.yaml` |
| LD6002B (beta) | `ceilsense-pro-v1-wifi-ld6002b.yaml` | `ceilsense-pro-v1-ethernet-ld6002b.yaml` |

Only install firmware matching the radar hardware fitted to the PCB.

For local development, copy `secrets.example.yaml` to `secrets.yaml` and use
`ceilsense-pro-v1-wifi-local.yaml`. The LD6002B integration is pinned to a beta
component and requires full hardware acceptance testing before deployment.

The ADPD188BI implementation is intentionally kept in this product repository
under `../components/adpd188bi/`. CeilSense Pro is currently the only product that
uses it, so its firmware and component versions remain synchronized. It can be
split into a separate repository later without changing its ESPHome API.

> The ADPD188BI feature is experimental. CeilSense Pro is not a certified
> smoke alarm and must not be used as a life-safety device.
