# Fingerprint Host-Firmware Protocol (v1)

This file defines the line-based protocol between:
- Host application: `src/main.cpp`
- Arduino firmware: `src/FingerPrint_Enroll.ino`

The fingerprint sensor's native binary UART protocol is handled internally by the firmware via the Adafruit library.

## Transport

- Encoding: ASCII text
- Framing: one message per line (`\n` terminated)
- Direction:
  - Host -> Arduino: command line
  - Arduino -> Host: events, responses, template packets

## Message Types

### 1) Host Command

Format:

CMD|<command_code>|<arg>

- `command_code`: integer command ID
- `arg`: integer argument (use `0` if command does not need arguments)

### 2) Firmware Event

Format:

EVT|<command_code>|<event_code>|<message>

- Asynchronous progress updates during a command flow
- `message` is human-readable and optional for logging/UI

### 3) Firmware Response

Format:

RSP|<command_code>|<status_code>|<payload>

- Terminal status for one command request
- `payload` can contain key-value data (`k=v;k2=v2`) or an error tag

### 4) Template Packet

Format:

TPL|<index>|<hex_bytes>

- Used by command `30` to stream fingerprint template bytes
- `hex_bytes` is a space-separated uppercase hex byte list
- Example: `TPL|0|3A FF 00 9C ...`

### 5) Image Packet

Format:

IMG|<index>|<hex_bytes>

- Used by command `40` to stream grayscale fingerprint image bytes
- `hex_bytes` is a space-separated uppercase hex byte list

Host-side file export for command `40`:

- Firmware streams only raw `gray8` pixel bytes.
- Host assembles these bytes into an uncompressed 8-bit Windows BMP file:
  - 14-byte file header
  - 40-byte BITMAPINFOHEADER
  - 1024-byte grayscale palette (256 entries)
  - pixel data (`width * height` bytes, row-padded per BMP rules)

## Command Codes

- `10`: Read board status
- `20`: Read fingerprint module status
- `30`: Read/store fingerprint and export template
- `40`: Capture fingerprint image and export grayscale data

## Status Codes

- `0`: OK
- `1`: Generic error
- `2`: Invalid command
- `10`: Sensor not found / offline
- `11`: Timeout
- `12`: Enroll/read flow failed

## Typical Flows

### Board Status

Host:
CMD|10|0

Firmware:
RSP|10|0|fw=fingerprint-host-proto-v1;serial=9600;sensor_uart=57600;board=arduino-uno

### Fingerprint Module Status

Host:
CMD|20|0

Firmware (online):
RSP|20|0|sensor=online;capacity=162;security=3;packet_len=64;baud=57600;stored=5

Firmware (offline):
RSP|20|10|sensor=offline

### Read/Store/Export Fingerprint

Host (`id=7`):
CMD|30|7

Firmware example sequence:
EVT|30|100|place finger
EVT|30|110|remove finger
EVT|30|120|place same finger again
EVT|30|200|streaming template
TPL|0|...
TPL|1|...
TPL|2|...
TPL|3|...
RSP|30|0|id=7;bytes=256;stored=1

### Capture Fingerprint Image

Host:
CMD|40|0

Firmware example sequence:
EVT|40|100|place finger
EVT|40|200|streaming image
IMG|0|...
IMG|1|...
...
RSP|40|0|bytes=73728;width=256;height=288;format=gray8

## Error Handling Rules

- Host should ignore lines that do not match a known message type.
- Host should correlate final `RSP` by `command_code`.
- For command `30`, host should accumulate all `TPL` lines until the final `RSP|30|...`.
- On timeout without `RSP`, host should treat command as failed.

## Compatibility Notes

- This protocol is versioned as `v1`.
- Future changes should keep backward compatibility where possible, or add a version handshake command.
