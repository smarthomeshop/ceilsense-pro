# CeilSense Pro hardware

## V1 ESP32-C6 development hardware

CeilSense Pro V1 consists of two connected PCBs.

### PCB 1: power and connectivity

- AC/DC or PoE input, depending on the assembled variant
- W5500 Ethernet controller
- USB-C connector and USB-to-UART interface for flashing and logs
- Switched 3.3 V Ethernet rail

### PCB 2: controller and sensors

- ESP32-C6-WROOM-1-N8
- 16 WS2812B LEDs
- MAX98357A I2S speaker amplifier
- transistor-driven buzzer
- SCD41, SGP41, BH1750, BMP388 and ADPD188BI I2C devices
- LD2412 and LD2450 mmWave modules on separate UART buses

### V1 GPIO map

| Function | GPIO | Notes |
| --- | ---: | --- |
| I2C SCL | 8 | Shared sensor bus; ESP32-C6 strapping pin |
| I2C SDA | 10 | Shared sensor bus |
| LED ring data | 13 | Conflicts with native USB-Serial-JTAG use |
| MAX98357A BCLK | 0 | I2S audio |
| MAX98357A LRCLK | 1 | I2S audio |
| MAX98357A DIN | 11 | I2S audio |
| Buzzer | 2 | Transistor drive, firmware-limited duty cycle |
| W5500 CS | 3 | SPI chip select |
| W5500 SCLK | 20 | SPI clock |
| W5500 MOSI | 21 | SPI controller to W5500 |
| W5500 MISO | 22 | W5500 to controller |
| W5500 interrupt | 23 | Ethernet interrupt |
| W5500 reset | 15 | ESP32-C6 strapping pin |
| W5500 rail enable | 12 | Active low; conflicts with native USB D- |
| LD2412 TX/RX | 19 / 18 | 115200 baud |
| LD2450 TX/RX | 6 / 5 | 256000 baud; GPIO5 is a strapping pin |

### V1 USB design note

The current V1 PCB uses its USB-to-UART interface for flashing and logging.
ESP32-C6 native USB uses GPIO12 and GPIO13, but those pins are already assigned
to Ethernet power control and the LED ring on this board. Native USB therefore
cannot be enabled on this hardware without a PCB revision and pin reassignment.

For a future native-USB PCB revision, add accessible BOOT and RESET controls or
test pads. Normal USB flashing can automatically reset a healthy application,
but BOOT access is still required to recover a blank, damaged or incorrectly
configured device reliably.

### V1 strapping pins

GPIO5, GPIO8 and GPIO15 are ESP32-C6 strapping pins. The V1 schematic determines
their external loading. Firmware validation warns about these assignments; do
not add external pull-ups or pull-downs without rechecking the ESP32-C6 boot
requirements.

## V2 ESP32-S3 hardware

V2 is based on the newest local schematics: the ESP32-S3 PCB with the smaller
VEML7700-family illuminance sensor and the matching ESP32-S3 power/Ethernet PCB.

### V2 hardware changes

- ESP32-S3-WROOM-1-N16R8 with 16 MB flash and 8 MB octal PSRAM
- native USB-C connected to GPIO19/GPIO20
- physical BOOT and RESET buttons
- I2S microphone module header
- MAX98357A speaker path on a separate I2S bus
- VEML7700-compatible illuminance sensor at I2C address `0x10`
- dedicated connectors for LD2450, LD2412, LD2460 and LD6002B options

### V2 fixed GPIO map

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

### V2 radar assemblies

The firmware intentionally provides separate images. Never enable multiple
components on the same UART.

| Assembly | Connections used | Firmware behaviour |
| --- | --- | --- |
| LD2450 + LD2412 | LD2450 GPIO6/7; LD2412 GPIO39/38 | Tracking plus independent presence fallback |
| LD2460 | GPIO39/38 | Five-target tracking in ceiling/top mode |
| LD6002B | GPIO39/38 plus GPIO4/5 diagnostics | 3D tracking; P19 held low for normal boot |

The LD6002B connector also exposes AIO1 on GPIO6 and SCL0 on GPIO7. The current
firmware does not use those two lines. GPIO5 reads the module's raw TX2 presence
signal as a disabled-by-default diagnostic entity.

### V2 microphone

The microphone header exposes 3.3 V, ground, BCLK, DOUT, LRCLK and SEL. Firmware
defaults to the left I2S channel, which matches common microphone modules with
SEL low. If the assembled module drives the right channel, override the
`microphone_channel` substitution with `right` and repeat the audio acceptance
test before production.
