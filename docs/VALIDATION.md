# Validation status and acceptance checks

## Automated checks

The following checks run in GitHub Actions and can also be run locally:

- repository structure and safety tests;
- ESPHome configuration validation for every V1 network/radar variant;
- complete compilation of all CeilSense Pro V1 firmware variants;
- host compilation and execution of buzzer priority, mute, hush, self-test and
  alarm-loop automation.

## Hardware acceptance checklist

Run this checklist on every PCB revision before declaring the firmware ready.

- [ ] USB-C powers the board and the serial interface remains stable.
- [ ] First flash and OTA update both succeed.
- [ ] WiFi captive portal onboarding succeeds.
- [ ] W5500 obtains an address over Ethernet and survives cable reconnects.
- [ ] WiFi firmware keeps the W5500 rail disabled.
- [ ] SCD41 reports plausible CO2, temperature and humidity.
- [ ] SGP41 reports VOC and NOx indices after warm-up.
- [ ] VEML7700 and BMP388 return plausible readings.
- [ ] LD2412 reports moving and stationary presence.
- [ ] LD2450 reports targets and all configured zones.
- [ ] LED ring displays boot, presence and alarm effects correctly.
- [ ] Buzzer never exceeds the configured 40% output cap.
- [ ] MAX98357A produces clean mono audio without resetting the board.
- [ ] Microphone channel and SEL wiring are confirmed with live RMS/voice input.
- [ ] Local wake word restarts after a Voice Assistant response and disconnect.
- [ ] Each fitted radar option is tested only with its matching firmware.
- [ ] LD6002B beta firmware is bench-tested before any field installation.
- [ ] ADPD188BI device ID and eFuse calibration are valid.
- [ ] ADPD188BI clean-air baseline becomes ready without false candidates.
- [ ] Controlled aerosol creates repeatable dual-channel response.
- [ ] Steam, dust, lighting and airflow nuisance tests are recorded.
- [ ] Network loss does not reboot or stop local sensor operation.

## Current status

Firmware configuration and host alarm automation have been validated with
ESPHome `2026.6.1`. Physical smoke sensitivity and false-alarm immunity are not
certified and remain development work. This repository does not claim EN 14604,
UL 217 or UL 268 compliance.
