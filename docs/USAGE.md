# Usage Guide

This guide explains how to build, flash, and run the FingerPrint Lock System host application.

## Table of Contents

1. [Building the Project](#building-the-project)
2. [Flashing the Arduino](#flashing-the-arduino)
3. [Running the Interactive Service](#running-the-interactive-service)
4. [Command Reference](#command-reference)
5. [Examples](#examples)
6. [Troubleshooting](#troubleshooting)

## Building the Project

### Prerequisites

- CMake 3.10+
- C++11 compatible compiler (g++, clang++)
- Arduino CLI (for flashing mode)
- Doxygen (optional, for documentation)

### Build Steps

```bash
# Navigate to project root
cd /path/to/fingerprint-arduino

# Create and enter build directory
mkdir -p build && cd build

# Configure with CMake
cmake ..

# Build the project
make

# Verify build succeeded
ls -l ../bin/fingerprintsensor
```

**Output:** Executable at `bin/fingerprintsensor`

### Optional: Generate API Documentation

```bash
cd build
make docs
# Open in browser: ../docs/html/index.html
```

## Flashing the Arduino

The host application can automatically compile and upload the Arduino sketch using `arduino-cli`.

### Prerequisites

1. **Arduino CLI installed:**
   ```bash
   # Check if installed
   which arduino-cli
   
   # If not found, install:
   curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
   ```

2. **Arduino connected via USB** (typically appears as `/dev/ttyACM0` or `/dev/ttyUSB0`)

3. **Arduino board core installed** (the script can do this with `--prepare`)

### Flash Modes

#### Mode 1: Flash with Automatic Board Setup (First Time)

```bash
./bin/fingerprintsensor --flash --prepare
```

This will:
1. Install the Arduino AVR board core (if needed)
2. Install Adafruit Fingerprint library (if needed)
3. Compile the sketch
4. Upload to `/dev/ttyACM0`

#### Mode 2: Flash with Custom Port

```bash
./bin/fingerprintsensor --flash --port /dev/ttyUSB0
```

For non-standard serial ports or multiple Arduino boards.

#### Mode 3: Flash with Custom Board Type

```bash
./bin/fingerprintsensor --flash --board arduino:avr:mega2560
```

For different Arduino models. Common examples:
- `arduino:avr:uno` — Arduino Uno (default)
- `arduino:avr:mega2560` — Arduino Mega
- `arduino:samd:arduino_zero_edbg` — Arduino Zero

#### Mode 4: Flash with Custom Sketch Location

```bash
./bin/fingerprintsensor --flash --sketch /path/to/custom_sketch.ino
```

#### Mode 5: Full Custom Configuration

```bash
./bin/fingerprintsensor --flash \
  --port /dev/ttyUSB0 \
  --board arduino:avr:mega2560 \
  --sketch custom/MySketch.ino \
  --prepare
```

### Flash Help

```bash
./bin/fingerprintsensor --flash --help
```

Output:
```
Usage:
  fingerprintsensor                           # Start service menu mode
  fingerprintsensor --flash [options]         # Compile and upload Arduino sketch

Flash options:
  --port <serial-port>    Default: /dev/ttyACM0
  --board <fqbn>          Default: arduino:avr:uno
  --sketch <path>         Default: src/FingerPrint_Enroll.ino
  --cli <path>            Path to arduino-cli binary
  --prepare               Install required board core and library first
  --help                  Show this help
```

## Running the Interactive Service

After flashing the Arduino, start the interactive service mode to communicate with the firmware and execute fingerprint operations.

### Basic Usage

```bash
./bin/fingerprintsensor
```

This starts an interactive CLI menu. The menu displays:

```
=== Fingerprint Lock System ===
1. Board Status     - Read firmware version and UART info
2. Sensor Status    - Check fingerprint module online/offline
3. Enroll & Export  - Dual-capture fingerprint enrollment with template export
4. Capture Image    - Capture fingerprint image and export as BMP
5. Flash Arduino    - Upload new firmware from this menu
0. Exit

Enter command (0-5):
```

### Menu Options

#### 1. Board Status

```
Enter command (0-5): 1
```

Displays:
- Firmware version
- Host serial UART baud rate (230400)
- Sensor UART baud rate
- Board type

Example output:
```
Board status: fw=fingerprint-host-proto-v1; serial=230400; sensor_uart=57600; board=arduino-uno
```

#### 2. Sensor Status

```
Enter command (0-5): 2
```

Displays:
- Sensor online/offline status
- Fingerprint template capacity
- Security level
- Stored template count

Example output:
```
Fingerprint status: sensor=online; capacity=127; security=5; stored=3
```

#### 3. Enroll & Export

```
Enter command (0-5): 3
```

Interactive fingerprint enrollment workflow:

1. **Prompts:** "Place finger on sensor"
2. **Waits:** For first fingerprint capture (max 20s)
3. **Prompts:** "Remove finger"
4. **Prompts:** "Place same finger again"
5. **Waits:** For second capture (verifies match)
6. **Exports:** Template to files:
   - `fingerprint_<timestamp>.txt` — hex format
   - `fingerprint_<timestamp>.bin` — binary format

Output shows real-time progress:
```
Initiating read/store/export...
[████████████████────────────────] 45% 1250B / 2780B [00m15s packets=28
Template exported to: fingerprint_2026-06-28.13-45-22.txt (2780 bytes)
Template exported to: fingerprint_2026-06-28.13-45-22.bin (2780 bytes)
```

#### 4. Capture Image

```
Enter command (0-5): 4
```

Captures a fingerprint image and exports as Windows BMP:

1. **Prompts:** "Place finger on sensor"
2. **Waits:** For image capture (max 20s)
3. **Exports:** 8-bit grayscale BMP file

Output:
```
Capturing fingerprint image...
[████████████████████████████████] 100% 18432B / 18432B [00m08s packets=72
Image exported to: fingerprint_image_2026-06-28.13-47-11.bmp (256x288)
```

#### 5. Flash Arduino (from Menu)

```
Enter command (0-5): 5
```

Re-uploads Arduino firmware without exiting the menu. Equivalent to running `./bin/fingerprintsensor --flash`.

#### 0. Exit

```
Enter command (0-5): 0
Exiting...
```

Cleanly closes the serial connection and exits.

## Command Reference

### Executable

```
./bin/fingerprintsensor [MODE] [OPTIONS]
```

### Modes

| Mode      | Description                              |
| --------- | ---------------------------------------- |
| (none)    | Start interactive service menu (default) |
| `--flash` | Flash Arduino sketch and exit            |
| `--help`  | Show help text                           |

### Flash Options

| Option          | Default                      | Description                                      |
| --------------- | ---------------------------- | ------------------------------------------------ |
| `--port PORT`   | `/dev/ttyACM0`               | Serial port where Arduino is connected           |
| `--board FQBN`  | `arduino:avr:uno`            | Board type (fully qualified board name)          |
| `--sketch PATH` | `src/FingerPrint_Enroll.ino` | Path to Arduino sketch file                      |
| `--cli PATH`    | (auto-detect)                | Explicit path to `arduino-cli` binary            |
| `--prepare`     | (disabled)                   | Install board core and libraries before flashing |
| `--help`        |                              | Show flash-specific help                         |

### Common FQBN Values

- `arduino:avr:uno` — Arduino Uno
- `arduino:avr:nano` — Arduino Nano
- `arduino:avr:mega2560` — Arduino Mega 2560
- `arduino:avr:leonardo` — Arduino Leonardo
- `arduino:samd:arduino_zero_edbg` — Arduino Zero

## Examples

### Example 1: First-Time Setup

```bash
# Build from scratch
mkdir -p build && cd build
cmake ..
make

# Flash Arduino for the first time (install board/libraries)
cd ..
./bin/fingerprintsensor --flash --prepare

# Enter interactive mode
./bin/fingerprintsensor
```

### Example 2: Enroll a New Fingerprint

```bash
./bin/fingerprintsensor
# Menu appears...
# Enter: 3
# Place finger when prompted
# Remove finger when prompted
# Place same finger again
# Wait for export...
# Result: fingerprint_YYYY-MM-DD.HH-MM-SS.txt and .bin files
```

### Example 3: Capture Fingerprint Image

```bash
./bin/fingerprintsensor
# Menu appears...
# Enter: 4
# Place finger when prompted
# Wait for capture and export...
# Result: fingerprint_image_YYYY-MM-DD.HH-MM-SS.bmp file (256x288 pixels)
```

### Example 4: Multiple Arduino Boards

```bash
# Flash Arduino Mega 2560 on /dev/ttyUSB0
./bin/fingerprintsensor --flash \
  --board arduino:avr:mega2560 \
  --port /dev/ttyUSB0 \
  --prepare
```

### Example 5: Flash Without Menu

```bash
# Just flash, then exit (no interactive mode)
./bin/fingerprintsensor --flash --port /dev/ttyACM0
```

### Example 6: Custom arduino-cli Path

```bash
# If arduino-cli is in a non-standard location
./bin/fingerprintsensor --flash --cli ~/tools/arduino-cli/bin/arduino-cli
```

## Troubleshooting

### "Cannot find /dev/ttyACM0"

**Problem:** Arduino not connected or wrong serial port.

**Solution:**
1. Connect Arduino via USB
2. Wait 2 seconds for device enumeration
3. Find the correct port:
   ```bash
   ls /dev/tty* | grep -E "ACM|USB"
   ```
4. Use `--port` flag with correct port

### "arduino-cli: command not found"

**Problem:** Arduino CLI not installed or not in PATH.

**Solution:**
```bash
# Install arduino-cli
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

# Verify installation
~/.local/bin/arduino-cli version

# Add to PATH (optional, for convenience)
export PATH="$HOME/.local/bin:$PATH"
```

### "Board not found" during flash

**Problem:** Arduino AVR board core not installed.

**Solution:**
```bash
# Use --prepare flag to auto-install
./bin/fingerprintsensor --flash --prepare
```

Or manually:
```bash
arduino-cli core install arduino:avr
arduino-cli lib install "Adafruit Fingerprint Sensor Library"
```

### "Upload failed: timeout"

**Problem:** Serial communication timeout during upload.

**Solutions:**
1. Reset Arduino manually: Press reset button on board
2. Disconnect and reconnect USB cable
3. Try different USB port on computer
4. Check Arduino is actually Uno (use `--board` flag)

### "Sensor not found" in service menu

**Problem:** Fingerprint sensor not responding.

**Solutions:**
1. Check sensor wiring (SoftwareSerial on pins 2-3)
2. Verify sensor power (3.3V or 5V depending on module)
3. Check serial baud rate matches (default 57600)
4. Try running "Board Status" first to verify firmware

### "Timeout reading line" in service menu

**Problem:** Serial communication timeout waiting for firmware response.

**Solutions:**
1. Re-flash the Arduino
2. Check USB cable connection
3. Try different USB port
4. Verify no other program is using the serial port (close Arduino IDE, etc.)

### "Permission denied" accessing /dev/ttyACM0

**Problem:** User doesn't have permission to access serial port.

**Solution:**
```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER

# Log out and back in, then try again
# Or use sudo
sudo ./bin/fingerprintsensor
```

## File Output

When running the interactive service, fingerprint data is automatically organized into the `exports/` directory structure:

### Directory Organization

```
exports/
├── templates/        # Fingerprint template data (hex and binary)
│   ├── YYYY-MM-DD.HH-MM-SS_idN.txt    # Hex-encoded template
│   └── YYYY-MM-DD.HH-MM-SS_idN.bin    # Binary template
└── images/          # Fingerprint images (BMP format)
    └── fingerprint_YYYY-MM-DD.HH-MM-SS.bmp    # Grayscale image
```

### Files Created

| File Type         | Location             | Naming                                | Size    | Description                   |
| ----------------- | -------------------- | ------------------------------------- | ------- | ----------------------------- |
| Template (hex)    | `exports/templates/` | `YYYY-MM-DD.HH-MM-SS_id1.txt`         | ~3 KB   | Space-separated hex bytes     |
| Template (binary) | `exports/templates/` | `YYYY-MM-DD.HH-MM-SS_id1.bin`         | ~2.7 KB | Raw binary format             |
| Fingerprint image | `exports/images/`    | `fingerprint_YYYY-MM-DD.HH-MM-SS.bmp` | ~75 KB  | 8-bit grayscale BMP (256×288) |

**Timestamp format:** `YYYY-MM-DD.HH-MM-SS` (e.g., `2026-06-28.13-45-22`)

### Examples

After enrolling fingerprint ID 1:
```bash
ls -lh exports/templates/
# Output:
# -rw-r--r-- 1 user group 2.8K Jun 28 13:45 2026-06-28.13-45-22_id1.txt
# -rw-r--r-- 1 user group 2.7K Jun 28 13:45 2026-06-28.13-45-22_id1.bin
```

After capturing an image:
```bash
ls -lh exports/images/
# Output:
# -rw-r--r-- 1 user group  75K Jun 28 13:47 fingerprint_2026-06-28.13-47-11.bmp
```

### Viewing Exports

View hex template:
```bash
cat exports/templates/2026-06-28.13-45-22_id1.txt
# Output: 6F 00 E7 FF E7 FF BF 7F DF 7F EF 7F F7 7F ...
```

Open fingerprint image:
```bash
feh exports/images/fingerprint_2026-06-28.13-47-11.bmp
# Or use any BMP viewer (GIMP, Photoshop, Paint, Preview, etc.)
```

### Backing Up Exports

```bash
# Archive all exports with timestamp
tar -czf fingerprint_exports_$(date +%Y%m%d_%H%M%S).tar.gz exports/

# Copy to external storage
cp -r exports/ /mnt/usb/backup/
```

For more details on working with exports, see [exports/README.md](../exports/README.md).

## Next Steps

- Read [Protocol Specification](protocol.md) for low-level communication details
- See [Documentation](README.md) for API documentation
- Check [DEVELOPMENT.md](../DEVELOPMENT.md) for contributing changes
- Learn more about exports organization in [exports/README.md](../exports/README.md)
