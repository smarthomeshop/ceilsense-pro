# CeilSense Pro

[![ESPHome CI](https://github.com/smarthomeshop/ceilsense-pro/actions/workflows/ci.yml/badge.svg)](https://github.com/smarthomeshop/ceilsense-pro/actions/workflows/ci.yml)

Local-first ESPHome firmware for CeilSense Pro: an all-in-one ceiling sensor
with WiFi or PoE/Ethernet, mmWave tracking, environmental sensors, voice,
speaker, buzzer and a 16-pixel LED ring.

> ⚠️ **This device is NOT a certified smoke alarm.** The ADPD188BI
> smoke-sensing features are experimental and provide no life-safety
> function. See [Smoke Detection Disclaimer — Certification &
> Legal](#smoke-detection-disclaimer--certification--legal) before using,
> distributing or selling this product.

The repository supports two hardware generations:

| Hardware | Controller | Status | Main differences |
| --- | --- | --- | --- |
| V1 | ESP32-C6-WROOM-1-N8 | Development/legacy | USB-UART, LD2450 + LD2412, speaker without microphone |
| V2 | ESP32-S3-WROOM-1-N16R8 | Current | Native USB, BOOT/RESET, PSRAM, microphone, Voice Assistant and three radar options |

## V2 Firmware Variants

| Radar hardware | WiFi | PoE/Ethernet |
| --- | --- | --- |
| LD2450 + LD2412 | `ceilsense-pro-v2-wifi-ld2450-ld2412.yaml` | `ceilsense-pro-v2-ethernet-ld2450-ld2412.yaml` |
| LD2460 | `ceilsense-pro-v2-wifi-ld2460.yaml` | `ceilsense-pro-v2-ethernet-ld2460.yaml` |
| LD6002B | `ceilsense-pro-v2-wifi-ld6002b.yaml` | `ceilsense-pro-v2-ethernet-ld6002b.yaml` |

These files live in `ceilsense-pro-v2/`. The local development file
`ceilsense-pro-v2-wifi-local.yaml` uses the standard LD2450 + LD2412 hardware.
Only install firmware matching the radar module or modules physically fitted.
The LD6002B integration currently uses the pinned `v0.1.0-beta.1` component
and must complete the hardware acceptance tests before production use.

The root `ceilsense-pro-wifi.yaml`, `ceilsense-pro-wifi-local.yaml` and
`ceilsense-pro-poe.yaml` files remain available for V1 ESP32-C6 test boards.

## Main Features

- ESP32-S3-WROOM-1-N16R8 with 16 MB flash and 8 MB PSRAM on V2
- LD2450 + LD2412, LD2460 or LD6002B mmWave hardware on V2
- SCD41 CO2, temperature and humidity
- SGP41 VOC and NOx indices
- VEML7700 illuminance and BMP388 temperature/pressure on V2
- WiFi or W5500 Ethernet with PoE power
- 16-pixel WS2812B LED ring
- I2S microphone, MAX98357A speaker, local wake word and Home Assistant Voice Assistant on V2
- Dedicated passive buzzer through the pinned `smarthomeshop/buzzer` package
- Experimental ADPD188BI dual-wavelength optical aerosol measurement

The complete V1 and V2 pin maps are in [`docs/HARDWARE.md`](docs/HARDWARE.md).

## Build

```bash
cp secrets.example.yaml secrets.yaml
# Edit secrets.yaml and set wifi_ssid / wifi_password.

esphome run ceilsense-pro-wifi-local.yaml

esphome config ceilsense-pro-wifi.yaml
esphome compile ceilsense-pro-wifi.yaml

esphome config ceilsense-pro-poe.yaml
esphome compile ceilsense-pro-poe.yaml

# V2 local development
cp ceilsense-pro-v2/secrets.example.yaml ceilsense-pro-v2/secrets.yaml
esphome run ceilsense-pro-v2/ceilsense-pro-v2-wifi-local.yaml
```

Docker instructions, USB logging details and the difference between production
and local firmware are documented in
[`docs/LOCAL-DEVELOPMENT.md`](docs/LOCAL-DEVELOPMENT.md).

## Shared buzzer package

All V1 and V2 entry points use `packages/buzzer.yaml`, which imports
[`smarthomeshop/buzzer`](https://github.com/smarthomeshop/buzzer) at **v1.0.0**.
GPIO2 drives the on-board transistor; maximum PWM duty remains 40%. First-use volume is 100% of
that limit, matching the previous 40% RTTTL gain. Later volume, mute and melody
choices are restored by the shared package.

The package adds sound selection, play/test/stop buttons, volume, mute and
playback diagnostics. It does not play a boot sound. The Pro overrides `warning`
with its existing smoke-test pattern and `error` with its existing failure cue.
Ordinary sounds use priority 0, optical self-test cues use 50 and the repeating
local/remote alert uses 100. A normal test cannot interrupt an active alert.

Global **Buzzer mute** and volume zero also silence the experimental alert.
Its device-owned repeat loop yields between attempts and resumes while the
alert remains active when sound is enabled again. **Smoke Alarm Silence** is
still the ten-minute hush control. **Buzzer stop** stops the current sound;
use the hush control to suspend an active repeating alert.

The first build needs access to GitHub to fetch the pinned package. The
package is compiled into the firmware; playback does not need a network.
No device is updated by editing these local YAML files alone.

The host integration test uses the real device adapter and alert scripts with
silent outputs and stub optical input. It checks priorities, mute/zero-volume
loop responsiveness, hush, both self-test outcomes and finite cue completion:

```sh
python3 tests/verify_buzzer.py
```

Run this in an environment with ESPHome and a native C++ compiler installed.
It verifies automation behavior, not the physical optical sensor or sound level.

## ADPD188BI Optical Smoke Sensing (Experimental)

ESPHome has no native ADPD188BI component, so this repo ships a local external
component in `components/adpd188bi/` (loaded via `packages/adpd188bi.yaml`).
It configures the ADPD188BI with the smoke-detection register set from the ADI
no-OS driver, reads Slot A (blue 470 nm LED) and Slot B (infrared 850 nm LED),
tracks an adaptive baseline and derives a relative "smoke index" with a
configurable alarm threshold. The standard I2C scan remains enabled so
hardware bring-up can verify the sensor is visible on the bus.

Analog Devices positions the ADPD188BI as a smoke-detection sensor; their
CN0583 reference design and app notes AN-1567/AN-2033 describe
production-grade calibration (PTR-based, blue/IR ratio discrimination against
UL 217/UL 268 test data). The logic in this firmware is a much simpler
baseline-delta heuristic intended for bring-up and calibration experiments
only — see the disclaimer below.

### Standalone alarm behaviour

All smoke alarm behaviour runs on-device (`packages/smoke_alarm.yaml`) and
keeps working without WiFi, Ethernet, Home Assistant or any cloud connection:

- Each five-second evaluation uses the median of 15 held optical samples.
  A window containing zero, out-of-range, or highly scattered samples is
  marked invalid and cannot start an alert. `ADPD188BI Measurement Stable`
  exposes that validity state for bench diagnostics.
- On startup, 36 stable windows are required before the measurement is armed
  (at least three minutes). The warm-up restarts when either channel drifts by
  more than 20%, avoiding an alarm based on an unstable desk-level baseline.
  `ADPD188BI Baseline Ready` exposes that armed state.
- The `ADPD188BI Smoke Alert` binary sensor requires four consecutive,
  dual-channel candidates above the configured threshold before it starts the
  buzzer siren and red LED strobe. It takes three stable clear windows to
  release an active alert. `ADPD188BI Alarm Candidate` shows candidates before
  they become an alert.
- The siren repeats the ISO 8201 temporal-three pattern (3 beeps, pause) at
  roughly 3.1 kHz until the alert clears.
- `Smoke Alarm Silence` (hush) mutes an active alarm for 10 minutes, then
  re-arms automatically.
- `Smoke Test` button works like the test button on a real smoke alarm: it
  runs a live sensor self-test (I2C communication, device ID, and a fresh
  optical sample on both channels). On pass it sounds the alarm pattern and
  strobes the ring; on fail it plays an error tone, lights the ring solid red
  and publishes `FAIL` to the `Smoke Self Test Result` text sensor.
- `Smoke Siren Enabled` switch controls the local siren. It defaults to off
  after a flash so a bench unit is armed deliberately only after a controlled
  smoke test.
- `Remote Smoke Alarm` switch lets Home Assistant sound this unit's siren and
  strobe, e.g. to interconnect multiple units (see below). It is never
  restored after a reboot, so a unit cannot come back latched in alarm.
- `ADPD188BI Sensor Problem` reports sensor setup failures and failed I2C
  reads, which would otherwise silently look like "no smoke".
- `api.reboot_timeout` and `wifi.reboot_timeout` are set to `0s` so the
  device never reboots itself just because the network or Home Assistant is
  unreachable. This matters because every reboot restarts the ADPD188BI
  baseline warm-up, during which smoke detection is intentionally unarmed
  (`baseline_samples` × `update_interval`, at least three minutes by default).

### Interconnecting multiple units via Home Assistant

With several CeilSense Pro units, a Home Assistant automation can sound all
units when any one of them detects smoke, using each unit's
`Remote Smoke Alarm` switch:

```yaml
automation:
  - alias: "CeilSense smoke interconnect"
    mode: single
    triggers:
      - trigger: state
        entity_id:
          - binary_sensor.ceilsense_livingroom_adpd188bi_smoke_alert
          - binary_sensor.ceilsense_kitchen_adpd188bi_smoke_alert
          - binary_sensor.ceilsense_hallway_adpd188bi_smoke_alert
        to: "on"
    actions:
      - action: switch.turn_on
        target:
          entity_id:
            - switch.ceilsense_livingroom_remote_smoke_alarm
            - switch.ceilsense_kitchen_remote_smoke_alarm
            - switch.ceilsense_hallway_remote_smoke_alarm
  - alias: "CeilSense smoke interconnect clear"
    mode: single
    triggers:
      - trigger: state
        entity_id:
          - binary_sensor.ceilsense_livingroom_adpd188bi_smoke_alert
          - binary_sensor.ceilsense_kitchen_adpd188bi_smoke_alert
          - binary_sensor.ceilsense_hallway_adpd188bi_smoke_alert
        to: "off"
    conditions:
      - condition: state
        entity_id:
          - binary_sensor.ceilsense_livingroom_adpd188bi_smoke_alert
          - binary_sensor.ceilsense_kitchen_adpd188bi_smoke_alert
          - binary_sensor.ceilsense_hallway_adpd188bi_smoke_alert
        state: "off"
    actions:
      - action: switch.turn_off
        target:
          entity_id:
            - switch.ceilsense_livingroom_remote_smoke_alarm
            - switch.ceilsense_kitchen_remote_smoke_alarm
            - switch.ceilsense_hallway_remote_smoke_alarm
```

Note the asymmetry with the local alarm: the unit that detects smoke always
alarms on its own, with no network involved, but the interconnect depends on
WiFi/Ethernet and Home Assistant being up. Do not rely on it — certified
interconnected smoke alarms use their own dedicated RF or wired link.

## Smoke Detection Disclaimer — Certification & Legal

**This product is NOT a smoke alarm (rookmelder) and must never be sold,
marketed, installed or relied upon as one.**

- The smoke-sensing functionality in this firmware and hardware is an
  **experimental optical particle/aerosol measurement for research,
  development and calibration purposes only**. It is uncertified, uncalibrated
  against any smoke-alarm standard, and provides **no life-safety function**.
- In the EU, a device may only be placed on the market as a smoke alarm if it
  complies with **EN 14604** (harmonised under the Construction Products
  Regulation, EU 305/2011). EN 14604 requires AVCP System 1 conformity
  assessment: type testing by a notified body (sensitivity, ≥85 dB(A) at 3 m
  sound output, endurance, and more), initial factory inspection, continuous
  surveillance of factory production control, a Declaration of Performance and
  CE marking. **CeilSense Pro has none of these.**
- In the Netherlands, smoke alarms are legally required on every floor of
  every home (Besluit bouwwerken leefomgeving, previously Bouwbesluit 2012
  art. 6.21, mandatory for existing homes since 1 July 2022) and those alarms
  must comply with EN 14604. **CeilSense Pro does not satisfy this legal
  obligation in any way.** Install certified EN 14604 smoke alarms regardless
  of this device.
- The smoke-related entities (`Smoke Alert`, `Smoke Index`, siren, LED strobe)
  are best-effort experimental indicators. They may fail to detect real smoke
  or fire, may trigger falsely, and may stop working at any time (sensor
  fault, power loss, firmware defect, misconfiguration). No warranty of any
  kind is given for this functionality, express or implied, including fitness
  for the purpose of fire or smoke detection.
- Marketing an uncertified device as a smoke detector may constitute a
  misleading commercial practice and exposes the seller to product liability
  (Directive 85/374/EEC, replaced by Directive (EU) 2024/2853) and
  market-surveillance enforcement. Any commercial listing of CeilSense Pro
  must describe the smoke functionality as an experimental air-quality/aerosol
  sensor and must not use the terms "smoke alarm", "smoke detector" or
  "rookmelder" for the product, nor imply any protective or life-safety
  capability.
- Independent of the smoke functionality, placing this device on the EU
  market still requires the usual product compliance for a WiFi IoT device:
  Radio Equipment Directive 2014/53/EU (including the cybersecurity
  requirements of Delegated Regulation (EU) 2022/30 / EN 18031, applicable
  since 1 August 2025, transitioning to the Cyber Resilience Act), RoHS, WEEE
  and the General Product Safety Regulation (EU) 2023/988.

By using this firmware you acknowledge that the smoke-sensing features are
experimental developer tooling and accept full responsibility for providing
certified smoke detection where required by law.

## Home Assistant API encryption

The firmware supports a unique API encryption key provisioned at runtime. No shared key is compiled into manufacturer firmware. Until a key is provisioned, unencrypted clients can connect; compatible Home Assistant versions can automatically provision a key and then use encrypted connections. Other native API clients must use the same key once encryption is active.

Runtime-provisioned keys survive reboots and normal manufacturer OTA updates that preserve preferences, provided the target firmware also supports runtime API encryption. Factory resets, erased preferences, or firmware without encryption support do not provide this guarantee.

A key previously compiled into a custom YAML configuration is not automatically copied into runtime storage. Moving from a YAML key to manufacturer firmware requires a one-time provisioning/reconfiguration step; entering a key only in a Home Assistant connection form does not write it to the device. Keep using ESPHome Device Builder updates if that migration has not been completed.

This setting controls the local native ESPHome API. SmartHomeShop Cloud uses its separate HTTP token authentication. See the [ESPHome API documentation](https://esphome.io/components/api/#configuration-variables).
