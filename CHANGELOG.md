# Changelog

All notable user-facing changes to CeilSense Pro firmware are documented here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and releases use semantic versioning.

## [Unreleased]

### Added

- Initial CeilSense Pro repository structure.
- CeilSense Pro V1 ESP32-S3-N16R8 hardware profile with native USB and PSRAM.
- WiFi, local WiFi and W5500 PoE/Ethernet firmware variants.
- Firmware choices for LD2450 + LD2412, LD2460 or LD6002B radar hardware.
- SCD41, SGP41, VEML7700 and BMP388 environmental measurements.
- WS2812B LED ring, MAX98357A speaker and transistor-driven buzzer support.
- Experimental ADPD188BI optical aerosol sensing component with eFuse
  calibration, adaptive baseline, diagnostics and standalone test automation.
- GitHub Actions validation for all firmware variants and the host alarm test.
- I2S microphone, MAX98357A mixer/resampler audio and local wake-word Voice
  Assistant support based on the UltimateSensor V2 architecture.
- Shared SmartHomeShop buzzer, LD2450, LD2460, LD6002B and radar-mounting
  packages pinned to released versions.

## [1.0.0] - 2026-11-04

### Added

- First development release for CeilSense Pro V1 ESP32-S3 hardware bring-up.

[Unreleased]: https://github.com/smarthomeshop/ceilsense-pro/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/smarthomeshop/ceilsense-pro/releases/tag/v1.0.0
