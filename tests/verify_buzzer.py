#!/usr/bin/env python3
"""Exercise production buzzer/alarm automations with no physical outputs."""

from pathlib import Path
import subprocess


def main():
    tests = Path(__file__).resolve().parent
    subprocess.run(["esphome", "compile", str(tests / "buzzer-smoke-host.yaml")], check=True)
    binary = (
        tests / ".esphome/build/ceilsense-pro-buzzer-test"
        / ".pioenvs/ceilsense-pro-buzzer-test/program"
    )
    result = subprocess.run(
        [str(binary)], timeout=30, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, text=True,
    )
    print(result.stdout, end="", flush=True)
    result.check_returncode()
    if "PRO_BUZZER_TESTS_PASSED:" not in result.stdout:
        raise RuntimeError("Missing buzzer integration test completion marker")


if __name__ == "__main__":
    main()
