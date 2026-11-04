# CeilSense Pro V1 hardware

CeilSense Pro V1 is the first product hardware release. It combines an
ESP32-S3 sensor PCB with the matching power and Ethernet/PoE PCB.

## Main hardware

- ESP32-S3-WROOM-1-N16R8 with 16 MB flash and 8 MB octal PSRAM
- native USB-C connected to GPIO19/GPIO20
- physical BOOT and RESET buttons
- I2S microphone and MAX98357A speaker amplifier
- transistor-driven passive buzzer
- 16 WS2812B LEDs
- SCD41, SGP41, VEML7700, BMP388 and ADPD188BI I2C devices
- W5500 Ethernet controller with switched 3.3 V rail
- connectors for LD2450 + LD2412, LD2460 or LD6002B radar assemblies

## GPIO map

| Function | GPIO | Notes |
| --- | ---: | --- |
| Buzzer | 2 | Transistor drive, 40% firmware cap |
| W5500 reset | 3 | Strapping pin |
| W5500 CS | 8 | SPI chip select |
| W5500 MOSI | 9 | SPI controller to W5500 |
| W5500 MISO | 10 | W5500 to controller |
| W5500 SCLK | 11 | SPI clock |
| MAX98357A BCLK | 12 | Speaker I2S bus |
| MAX98357A LRCLK | 13 | Speaker I2S bus |
| MAX98357A DIN | 14 | Speaker I2S data |
| I2C SCL | 15 | Shared environmental/optical bus |
| I2C SDA | 16 | Shared environmental/optical bus |
| LED ring data | 17 | 16 WS2812B pixels |
| Native USB D- / D+ | 19 / 20 | USB Serial/JTAG and flashing |
| LD2412/LD2460/LD6002B TX/RX | 39 / 38 | Mutually exclusive primary radar UART |
| Microphone BCLK | 40 | Microphone I2S bus |
| Microphone LRCLK | 41 | Microphone I2S bus |
| Microphone DOUT | 42 | Microphone I2S data |
| W5500 interrupt | 46 | Strapping pin |
| W5500 rail control | 48 | High disables, low enables Ethernet 3.3 V |

## Radar assemblies

The firmware provides a separate image for every radar assembly. Never enable
multiple components on the same UART.

| Assembly | Connections used | Firmware behaviour |
| --- | --- | --- |
| LD2450 + LD2412 | LD2450 GPIO6/7; LD2412 GPIO39/38 | Tracking plus independent presence fallback |
| LD2460 | GPIO39/38 | Five-target tracking in ceiling/top mode |
| LD6002B | GPIO39/38 plus GPIO4/5 diagnostics | 3D tracking; P19 held low for normal boot |

The LD6002B connector also exposes AIO1 on GPIO6 and SCL0 on GPIO7. The current
firmware does not use those lines. GPIO5 reads the module's raw TX2 presence
signal as a disabled-by-default diagnostic entity.

## Microphone

The microphone header exposes 3.3 V, ground, BCLK, DOUT, LRCLK and SEL. Firmware
defaults to the left I2S channel, matching common microphone modules with SEL
low. If the assembled module drives the right channel, override the
`microphone_channel` substitution with `right` and repeat the audio acceptance
test before production.

## USB recovery

Normal flashing and logs use the ESP32-S3 native USB Serial/JTAG interface.
The BOOT and RESET buttons allow download-mode recovery if an invalid image or
USB configuration prevents automatic flashing.
