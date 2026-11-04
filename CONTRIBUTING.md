# Contributing

## Development requirements

- Docker, or ESPHome `2026.6.1`
- Python 3.11 or newer for repository tests
- A CeilSense Pro V1 ESP32-S3 board for hardware validation

## Local checks

Run the repository tests:

```sh
python3 -m unittest discover -s tests -p 'test_*.py' -v
```

Validate all ESPHome entry points with Docker:

```sh
for file in ceilsense-pro-wifi.yaml ceilsense-pro-ethernet.yaml; do
  docker run --rm -v "$PWD":/config -w /config \
    esphome/esphome:2026.6.1 config "$file"
done

for file in ceilsense-pro-v1/ceilsense-pro-v1-*.yaml; do
  docker run --rm -v "$PWD":/config -w /config \
    esphome/esphome:2026.6.1 config "$file"
done
```

Compile the host alarm integration test and run it inside the container:

```sh
docker run --rm -v "$PWD":/config -w /config \
  --entrypoint /bin/bash esphome/esphome:2026.6.1 -lc \
  'esphome compile tests/buzzer-smoke-host.yaml && \
   tests/.esphome/build/ceilsense-pro-buzzer-test/.pioenvs/ceilsense-pro-buzzer-test/program'
```

## Pull requests

- Keep hardware packages modular.
- Do not change GPIO assignments without updating `README.md` and
  `docs/HARDWARE.md`.
- Add a customer-facing entry under `Unreleased` in `CHANGELOG.md` when
  firmware behaviour changes.
- Never commit WiFi credentials, API keys, compiled firmware or ESPHome build
  directories.
- Treat the ADPD188BI implementation as experimental measurement firmware,
  not as a certified life-safety device.
