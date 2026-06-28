# Recent Major Changes Reference

This document provides a quick reference for major updates in the codebase as of v1.0.0. For fully detailed API documentation with examples, see [docs/README.md](docs/README.md) and the generated [docs/html/index.html](docs/html/index.html).

## 📝 main.cpp - Host Application

### New Modes

#### Flash Mode (`--flash`)
Automates Arduino sketch compilation and upload using arduino-cli:

```bash
./fingerprintsensor --flash --board arduino:avr:uno --port /dev/ttyACM0
```

**Features:**
- Path resolution for arduino-cli (system PATH, ~/.local/bin)
- Automatic sketch staging to Arduino-compatible directory structure
- Optional auto-installation of board cores and libraries with `--prepare`
- Shell-escaped file paths for safety
- Error reporting with helpful diagnostics

#### Interactive Service Mode (default)
Menu-driven fingerprint operations:

```
========== Fingerprint Service Menu ==========
1) Read board status
2) Check fingerprint module status
3) Read/store fingerprint and export template
4) Capture fingerprint image
5) Quit
```

### Serial Communication Protocol

**New Command Handling:**

| Command                  | Function        | Input          | Output                      |
| ------------------------ | --------------- | -------------- | --------------------------- |
| `requestSimpleStatus()`  | Single RSP read | CMD code       | RSP with status/payload     |
| `sendCommand()`          | Send CMD line   | code, arg      | Formatted: CMD\|code\|arg\n |
| `readSerialLine()`       | Read until \n   | timeout        | String line                 |
| `readSerialBytesExact()` | Read N bytes    | count, timeout | vector<uint8_t>             |

**New Parsers:**

```cpp
parseResponseLine()        // RSP|cmd|status|payload
parseTemplateLine()        // TPL|idx|hex-bytes
parseImageBinaryHeader()   // IMB|idx|payload_len
parseHexBytes()            // "3A FF 00 9C" → {0x3A, 0xFF, 0x00, 0x9C}
```

### Template Export Flow

**Workflow:**
1. User enters fingerprint ID (1-127)
2. Host sends CMD|30|ID command
3. Firmware streams TPL packets with hex-encoded template bytes
4. Host reconstructs binary template
5. Persists to disk:
   - `.txt` file - hex dump (human-readable)
   - `.bin` file - raw binary (machine-readable)

**File Persistence:**
```cpp
persistFingerprintTemplate(bytes, fingerId)
// Creates: 2026-06-28.235959_id42.txt
//          2026-06-28.235959_id42.bin
```

### Image Capture Flow

**Workflow:**
1. User selects "Capture image" from menu
2. Host sends CMD|40|0 command
3. Firmware streams IMB packets with raw grayscale bytes
4. Host reconstructs 4-bit grayscale image
5. Expands to 8-bit grayscale
6. Exports as Windows BMP file with:
   - 14-byte file header
   - 40-byte BITMAPINFOHEADER
   - 1024-byte grayscale palette (256 entries)
   - Raw pixel data (row-padded per BMP spec)

**File Export:**
```cpp
persistFingerprintImageBmp(bytes, width, height)
// Creates: fingerprint_2026-06-28.235959.bmp
// Format: Uncompressed 8-bit Windows BMP
// Dimensions: 256x288 (or detected height)
```

**Image Packet Format (IMB):**
```
IMB|0|64           // Packet 0, 64 bytes payload
[64 raw bytes]     // 64 bytes of packed 4-bit grayscale data
                   // (2 pixels per byte: high nibble = pixel0, low = pixel1)
```

### Async Progress Display

**Features:**
- Real-time progress bar with percentage
- Byte count and packet tracking
- Elapsed time formatting (MMmSSs)
- Runs in background thread
- Non-blocking main loop

**Usage:**
```cpp
AsyncProgressDisplay progress("Template Export", 640, start_time);
progress.start();
// ... transfer data ...
progress.update(bytesReceived, targetBytes, packetCount);
progress.stop();
```

### Enhanced Error Recovery

**Sensor Baud Fallback (Image Capture):**
- Starts at 57600 baud (SoftwareSerial default)
- If >12 bad frames detected, attempts 19200 baud fallback
- Restarts image capture at new baud rate
- Recovers from timeout/corruption issues

**Frame Recovery:**
```cpp
// For templates: up to 6 bad frames before failure
// For images: up to 40 bad frames before fallback
// Then up to 40 more with new baud rate
```

## 🤖 FingerPrint_Enroll.ino - Arduino Firmware

### Command Dispatcher

Four main commands:

| Command       | Code | Arg   | Response                               |
| ------------- | ---- | ----- | -------------------------------------- |
| Board Status  | 10   | 0     | Board info: fw, UART baud, board type  |
| FP Status     | 20   | 0     | Sensor info: capacity, security, count |
| Read/Store    | 30   | 1-127 | Template bytes (TPL packets)           |
| Capture Image | 40   | 0     | Grayscale image (IMB packets)          |

### Structured Packet Reading

**Adafruit Fingerprint Sensor Protocol:**
```
Frame Format:
┌──────────────────────────────────────────────────┐
│ Sync (0xEF01) │ Address (4) │ Type │ Length │ Data+CRC │
└──────────────────────────────────────────────────┘
     2 bytes       4 bytes    1 byte  2 bytes   Variable
```

**Error Codes:**
- `stageError = 1` - Sync code timeout/not found
- `stageError = 2` - Second sync byte read failed
- `stageError = 3-7` - Address/type/length/data read failure
- `stageError = 8` - Payload exceeds buffer

**Retry Logic:**
- Sync to 0xEF01 with 1.5s timeout
- Allow up to 6 bad frames before failure
- Each stage has independent timeout (200-3000ms)

### Template Export (CMD 30)

**Process:**
1. Load stored model from sensor (ID 1-127)
2. Call getModel() to prepare upload
3. Read structured packets from sensor UART
4. Identify DATAPACKET vs ENDDATAPACKET types
5. Stream each packet as TPL line with hex bytes
6. Verify total bytes > 0

**Error Codes (CMD 30):**
- `90+code` - Model load error
- `100+code` - getModel() error
- `400+stage` - Packet read error at stage
- `420+type` - Invalid packet type received

**Packet Example:**
```
TPL|0|3A FF 00 9C 12 34 56 78 AB CD EF 01 23 45 67 89
TPL|1|AB CD EF 01 23 45 67 89 ...
...
TPL|19|[final packet]
```

**Expected Size:** ~640 bytes (20 packets × 32 bytes typical)

### Image Export (CMD 40)

**Process:**
1. Send upload-image command (0x0A) to sensor
2. Receive acknowledgment (ACK packet)
3. Read 2-40+ binary image data packets from sensor
4. Compute image height from total bytes
5. Stream each packet as IMB line with byte count
6. Stream raw packet bytes immediately after
7. Verify end packet received and height valid

**Image Format:**
- Input: 4-bit grayscale packed (2 pixels/byte)
- Output: Raw binary, then unpacked by host to 8-bit
- Dimensions: 256×288 typical, but computed from data
- Total bytes: ~(256/2 × height) = ~36KB for full image

**Error Codes (CMD 40):**
- `500+code` - Image upload command error
- `600+stage` - Packet read stage error
- `601` - Initial sync timeout (>3 retries)
- `620+type` - Invalid packet type
- `630-634` - Data validation errors (height, format, totals)

**Packet Example:**
```
IMB|0|64
[64 raw bytes of image data]
IMB|1|64
[64 raw bytes of image data]
...
IMB|550|32
[final 32 bytes]
```

### Sensor UART Baud Detection

**On Boot:**
```cpp
// Try in order:
initializeSensorLink()
  ├─ 57600 baud (common, SoftwareSerial stable)
  ├─ 19200 baud (fallback)
  └─ 9600 baud (last resort)
```

**Dynamic Baud Switching (Image Capture):**
```cpp
// If image stream has >12 bad frames at 57600:
setSensorBaudRuntime(19200)
  ├─ Send command packet to sensor (baud config)
  ├─ Wait 120ms for sensor to switch
  ├─ Verify new baud with password check
  └─ Restart image upload
```

### Fingerprint Enrollment Workflow (CMD 30)

**Two-Finger Verification:**
```
1. User places finger
   ├─ captureImage → image2Tz(1)
   └─ Send EVT|200 "remove finger"

2. User removes finger
   ├─ waitUntilNoFinger (8s timeout)
   └─ Send EVT|110 "remove_timeout" if fails

3. User places same finger again
   ├─ captureImage → image2Tz(2)
   └─ createModel() (generates template)

4. Store template
   ├─ storeModel(fingerId) - save to sensor
   └─ uploadModel() - stream to host

5. Return response
   └─ RSP|30|0|id=42;bytes=640;stored=1
```

**Status Codes:**
- `0` - OK (template stored and exported)
- `1` - Generic error
- `12` - Enroll/read failed (image quality, matching)
- `11` - Timeout (finger removal, capture)

### Board Status Response

**Example Response:**
```
RSP|10|0|fw=fingerprint-host-proto-v1;serial=230400;sensor_uart=57600;board=arduino-uno
```

**Fields:**
- `fw` - Firmware version/protocol ID
- `serial` - Host UART baud (230400 typical)
- `sensor_uart` - Current sensor UART baud (57600/19200/9600)
- `board` - Arduino board type (arduino-uno, arduino-nano, etc.)

### Fingerprint Module Status Response

**Example Response:**
```
RSP|20|0|sensor=online;capacity=150;security=2;packet_len=128;baud=57600;stored=42
```

**Fields:**
- `sensor` - "online" or "offline"
- `capacity` - Max templates (usually 127-200)
- `security` - Security level (1-5, higher = stricter matching)
- `packet_len` - Payload packet size bytes
- `baud` - Sensor UART baud rate
- `stored` - Number of templates currently stored

---

## 🔄 Integration Points

### Host ↔ Firmware Communication Flow

```
HOST                           FIRMWARE
  │                                 │
  ├─ CMD|30|42 ─────────────────→ [Process]
  │                                 │
  │ ← EVT|30|100 "place finger" ─ [Waiting]
  │ ← EVT|30|110 "remove finger"    │
  │ ← EVT|30|200 "streaming..."    [Export]
  │                                 │
  │ ← TPL|0|[hex bytes] ────────── [Stream]
  │ ← TPL|1|[hex bytes] ────────→ [Collect]
  │ ← TPL|...|...                   │
  │                                 │
  │ ← RSP|30|0|id=42;bytes=640 ← [Done]
  │                                 │
  └─ Save to disk                   │
```

### Error Reporting

**Protocol Layers:**

| Layer     | Codes  | Example                                                          |
| --------- | ------ | ---------------------------------------------------------------- |
| Protocol  | 0-2    | Invalid command format                                           |
| Board     | 10-12  | Sensor offline, timeout, enroll fail                             |
| Operation | 90-634 | Model load fail (90X), packet error (40X), data validation (63X) |

**Event Messages (EVT):**
- Informational: "place finger", "remove finger", "streaming..."
- Errors: "capture_1_failed:timeout", "template_stream_failed"

**Response Payloads (RSP):**
- Success: `id=42;bytes=640;stored=1`
- Error: `capture_image_failed:code=timeout;code=11`

---

## 🚀 Testing Checklist

When validating the updated code:

- [ ] Board status command responds with correct firmware version
- [ ] Sensor status shows correct capacity and stored count
- [ ] Fingerprint enrollment completes successfully
  - [ ] Two-finger capture works
  - [ ] Template exports (>0 bytes)
  - [ ] Files saved locally (.txt and .bin)
- [ ] Image capture completes successfully
  - [ ] Grayscale data exported
  - [ ] BMP file created with correct dimensions
  - [ ] Image viewable in standard image viewer
- [ ] Error recovery works
  - [ ] Timeout on stuck finger
  - [ ] Sensor offline detection
  - [ ] Baud fallback on image stream errors
- [ ] Serial communication stable
  - [ ] No data corruption
  - [ ] Packets reassembled correctly
  - [ ] Checksums validated

---

## 📞 Getting Help

For detailed implementation info, see:
- **Protocol Details:** [docs/protocol.md](docs/protocol.md)
- **API Documentation:** [docs/html/index.html](docs/html/index.html)
- **Source Code Comments:** Full Doxygen comments in src/ and include/

To regenerate docs after these changes:
```bash
./scripts/regenerate-docs.sh --clean
```

---

**Last Updated:** 2026-06-28  
**Version:** 1.0.0
