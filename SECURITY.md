# Security Policy

## Supported versions

| Version | Supported |
| --- | --- |
| Latest release and `main` | Yes |
| Older development builds | No |

## Reporting a vulnerability

Do not open a public issue for a security vulnerability. Use GitHub private
vulnerability reporting at:

https://github.com/smarthomeshop/ceilsense-pro/security

We aim to acknowledge a report within seven days.

## Security considerations

- WiFi credentials and locally provisioned secrets belong in `secrets.yaml`,
  which is excluded from Git.
- Home Assistant API encryption is provisioned per device at runtime.
- OTA access depends on the security of the local network and Home Assistant.
- LD2412 and LD2450 Bluetooth should be disabled after radar configuration when
  it is not needed.
- The web server exposes device state to the local network and must not be
  forwarded to the public internet.

The experimental optical aerosol measurement is not a security or life-safety
system and must not replace a certified smoke alarm.
