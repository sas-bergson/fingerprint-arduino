# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Planned
- Image recognition with threshold configuration
- Persistent template storage on SD card
- Web dashboard for enrollment management
- Multi-sensor support

## [1.0.0] - 2026-06-28

### Added
- **Complete Fingerprint Lock System** — Production-ready fingerprint enrollment, verification, and image capture
- **Interactive Service Menu** — User-friendly CLI for all operations
- **Arduino Sketch Upload** — Automated compilation and flashing via arduino-cli with board core auto-install
- **Async Progress Display** — Real-time feedback during transfers with background thread rendering
- **File Export & Persistence** — Template (hex/binary) and image (BMP) export with organized directory structure
- **Image Capture & Streaming** — Raw grayscale image acquisition with 4-bit to 8-bit expansion
- **Fingerprint Enrollment** — Dual-capture workflow with finger removal detection
- **Protocol Implementation** — Complete host-firmware communication (v1 ASCII line-based protocol)
- **API Documentation** — Full Doxygen documentation for all functions, classes, and protocol
- **Usage Guide** — Comprehensive documentation with build, flash, run, and troubleshooting sections
- **File Organization** — Automatic `exports/{templates,images}/` directory structure for user-generated data

### Fixed
- **File Organization** (main.cpp, .gitignore, docs)
  - Organize fingerprint exports into `exports/templates/` and `exports/images/` subdirectories
  - Add `ensureExportDirectories()` function for automatic directory creation
  - Update `persistFingerprintTemplate()` to save templates to organized subdirectory
  - Update `persistFingerprintImageBmp()` to save BMP images to organized subdirectory
  - Exclude `exports/` from Git to keep biometric data private
  - Improve project root cleanliness by centralizing user-generated exports
  - Add comprehensive export organization guide (exports/README.md)
  - Update docs/USAGE.md with file location reference and backup examples

## [0.9.0] - 2026-06-27

### Added
- **Interactive Service Menu** (`printServiceMenu`, `runServiceMenu` in main.cpp)
  - CLI-driven command dispatcher with user-friendly menu
  - Integrated flash mode launch from service loop
  - Graceful shutdown and error handling

### Fixed
- Command-line parsing robustness for edge cases

## [0.8.0] - 2026-06-24

### Added
- **File Export & Persistence** (main.cpp)
  - Template export to `.txt` (hex) and `.bin` (binary) formats via `persistFingerprintTemplate()`
  - Grayscale image export to uncompressed 8-bit Windows BMP files via `persistFingerprintImageBmp()`
  - Timestamp-based filename generation with `filenameTimestamp()`
  - Row-major BMP pixel layout with proper Windows padding
  - 4-bit to 8-bit grayscale expansion via `expandGray4PackedToGray8()`

## [0.7.0] - 2026-06-21

### Added
- **Async Progress Display** (main.cpp)
  - `AsyncProgressDisplay` class for real-time transfer feedback
  - Background thread rendering with atomic state updates
  - `printTransferProgress()` for CLI-style progress bar (40-char width, % complete, elapsed time, packet count)
  - `printTransferSummary()` for final transfer statistics
  - `formatDuration()` for human-readable elapsed time (MMmSSs format)

### Changed
- Transfer workflows now show live progress instead of silent delays

## [0.6.0] - 2026-06-18

### Added
- **Image Capture & Export** (FingerPrint_Enroll.ino)
  - `handleCaptureImage()` command dispatcher for image acquisition
  - `streamImageBytes()` with 4-bit grayscale packet streaming (IMG format)
  - `requestImageUpload()` for initiating sensor image upload
  - `setSensorBaudRuntime()` for dynamic baud reconfiguration
  - Multi-packet recovery with fallback to lower baud (57600 → 19200) on persistent errors
  - Image height validation from received bytes for module compatibility
  - Stream error codes 600X-630X range

### Added (main.cpp)
- `requestFingerprintImageCapture()` for coordinating host-side image flow
- `parseImageLine()` and `parseImageBinaryHeader()` for protocol parsing
- `consumeImageFrameDelimiter()` for robust frame boundary handling

## [0.5.0] - 2026-06-14

### Added
- **Fingerprint Enrollment Flow** (FingerPrint_Enroll.ino)
  - `handleReadStoreExport()` command dispatcher for dual-capture enrollment
  - Dual-finger capture workflow with removal detection via `waitUntilNoFinger()`
  - `waitForFingerImage()` for polling image acquisition with timeout
  - `captureErrorLabel()` for human-readable error mapping
  - Image-to-template conversion via `image2Tz()`
  - Model creation and storage via `createModel()` and `storeModel()`
  - `streamTemplateBytes()` for exporting stored templates as TPL packets
  - Stream error codes 90X-130X range

### Added (main.cpp)
- `requestFingerprintReadStore()` for host-side enrollment orchestration
- `parseTemplateLine()` for TPL packet parsing
- `parseHexBytes()` for protocol hex-string decoding

## [0.4.0] - 2026-06-11

### Added
- **Board & Sensor Status Reporting** (FingerPrint_Enroll.ino)
  - `handleBoardStatus()` with firmware version, UART baud, board type metadata
  - `handleFingerprintStatus()` with sensor capacity, security level, packet length, stored template count
  - Sensor readiness tracking via `sensorReady` and `sensorUartBaud` globals

### Added (main.cpp)
- `requestSimpleStatus()` for single-command status queries
- `parseResponseLine()` for RSP protocol line parsing
- Global constants for command codes (CMD_BOARD_STATUS, CMD_FP_STATUS, CMD_READ_STORE_EXPORT, CMD_CAPTURE_IMAGE)
- Global constants for status codes (STS_OK, STS_ERR, STS_INVALID_CMD, STS_SENSOR_NOT_FOUND, STS_TIMEOUT, STS_ENROLL_FAIL)

## [0.3.0] - 2026-06-07

### Added
- **Arduino Sketch Flash/Upload Automation** (main.cpp)
  - `flashArduino()` for arduino-cli compile and upload pipeline
  - `prepareSketchDirectory()` for sketch staging into Arduino-compatible layout
  - `resolveArduinoCliPath()` for CLI binary auto-discovery (system PATH, ~/.local/bin)
  - Board core and library auto-installation via arduino-cli with `--prepare` flag
  - Shell escaping via `shellEscape()` for safe argument passing
  - Comprehensive error reporting and diagnostics

### Added
- Path utilities: `dirnameOf()`, `basenameOf()`, `stemOf()`, `endsWith()`
- `runCommand()` for shell command execution with status reporting

## [0.2.0] - 2026-06-04

### Added
- **Serial Communication Framework** (FingerPrint_Enroll.ino & main.cpp)
  - Line-delimited protocol with CMD/EVT/RSP/TPL packet types
  - `sendEvent()`, `sendResponse()` for firmware message transmission
  - `sendCommand()` for host command sending
  - `readSerialLine()` with timeout handling
  - `readSerialByteWithTimeout()` and `readSerialBytesExact()` for low-level I/O
  - `drainSerialInput()` for synchronization and buffer clearing
  - Structured packet reading via `readSensorStructuredPacket()` with multi-stage error reporting
  - Checksum validation and packet size constraints (300-byte limit)

### Added (FingerPrint_Enroll.ino)
- Sensor UART link initialization: `initializeSensorLink()`, `trySensorBaud()`
  - Baud detection sequence: 57600 → 19200 → 9600
  - Password verification and parameter readout
- `readSensorByte()` with millisecond-precision timeout
- Helper utilities: `byteToHex()` for hex string conversion

### Added (main.cpp)
- Input parsing: `split()` for delimiter-based string tokenization
- Protocol constants and global state management

## [0.1.0] - 2026-06-01

### Added
- **Project Foundation** (main.cpp & FingerPrint_Enroll.ino)
  - CMake build system with C++11 standard
  - Arduino Uno firmware skeleton
  - Host application entry point via `main()` and Arduino entry points `setup()`, `loop()`
  - CLI argument parsing via `parseArguments()` supporting `--flash`, `--port`, `--board`, `--sketch`, `--prepare`, `--cli`
  - `printUsage()` for help text
  - Time utilities: `currentDateTime()` for logging, `filenameTimestamp()` for safe filenames
  - Command dispatching backbone via `processCommandLine()` in firmware
  - Initial status code framework

## [1.0.0] - 2026-06-28

### Documentation
- **Automated API Documentation** - Doxygen integration for all C++ and Arduino code
  - `docs/README.md` - Comprehensive documentation guide
  - `scripts/regenerate-docs.sh` - Automated documentation regeneration script
  - Updated `doxygen.cfg` to include source code (`src/` folder)
  - Documentation now covers all major components: host app, firmware, serial library
  - Added to CMake build system - docs automatically generated with `make docs`

### Added (Code)
- Initial stable release
- Host-Firmware Protocol v1 implementation
- Fingerprint enrollment and verification
- Template and grayscale image export over UART
- Multithreading support for concurrent operations
- CMake build system with C++11 standard
- Arduino Uno firmware sketch
- Doxygen API documentation
- ASCII line-based communication protocol with CMD/EVT/RSP/TPL/IMG message types
- Status codes for board and sensor diagnostics
- SerialPort C++ abstraction for cross-platform UART communication

### Added (Features - main.cpp)
- **Flash Mode** - Arduino sketch compilation and upload automation using arduino-cli
  - CLI path resolution (system PATH, ~/.local/bin)
  - Automatic sketch staging into Arduino-compatible directory structure
  - Board core and library auto-installation with `--prepare`
  - Error handling and diagnostic messages
  
- **Interactive Service Mode**
  - Teletype-style CLI menu with command prompts
  - Four main operations: board status, FP status, enroll/export, capture image
  
- **Protocol Implementation**
  - Command parsing (CMD|code|arg)
  - Response parsing (RSP|cmd|status|payload)
  - Template packet streaming (TPL|idx|hex-bytes)
  - Binary image packet streaming (IMB|idx|payload_len)
  - Event processing (EVT|cmd|code|message)
  
- **Transfer Management**
  - Async progress display with real-time status updates
  - Timeout handling for long operations
  - Byte counting and packet tracking
  - Duration calculation and formatting
  
- **File Export**
  - Template export to .txt (hex) and .bin (binary) formats
  - Fingerprint image export to uncompressed 8-bit Windows BMP files
  - Grayscale palette support
  - Timestamp-based filename generation
  - Row-major BMP pixel layout with proper padding
  
- **Serial Communication**
  - Newline-delimited protocol parsing
  - Byte-exact reads with timeout
  - Input draining and synchronization
  - Error recovery and retry logic

### Added (Features - FingerPrint_Enroll.ino)
- **Command Dispatcher**
  - CMD 10 - Board status (firmware version, UART baud, board type)
  - CMD 20 - Fingerprint module status (sensor online/offline, capacity, security level)
  - CMD 30 - Read/store fingerprint and export template
  - CMD 40 - Capture fingerprint image and export grayscale data
  
- **Structured Packet Reading**
  - Robust 0xEF01 sync-code detection with timeout
  - Multi-stage packet parsing with error reporting
  - Checksum handling (payload + 2-byte checksum)
  - Packet size validation (300-byte limit)
  - Timeout per stage for debugging
  
- **Template Streaming**
  - Model loading from sensor storage
  - Template getModel() with retry logic
  - Packet-by-packet hex streaming (TPL format)
  - End-of-packet detection
  - Stream error codes (90X-130X range)
  
- **Image Streaming**
  - Image upload command (0x0A) with acknowledgment
  - 4-bit grayscale packed format (2 pixels/byte)
  - Multi-packet streaming with bad-frame recovery
  - Baud fallback (57600 → 19200) on persistent errors
  - Computed image height validation
  - Stream error codes (600X-630X range)
  
- **Sensor Initialization**
  - UART baud detection (57600, 19200, 9600)
  - Password verification
  - Parameter reading (capacity, security level, baud rate)
  - Dynamic baud-rate reconfiguration
  
- **Workflow Management**
  - Dual-capture fingerprint enrollment flow
  - Finger removal detection with timeout
  - Image-to-template conversion (image2Tz)
  - Model creation and storage
  - Model matching and quality checks
  
- **Error Handling**
  - Comprehensive status codes (0-12 for protocol, 90-634 for operations)
  - Descriptive error messages in EVT/RSP payloads
  - Timeout tracking and reporting
  - Retry logic for transient failures
  - Frame validation and recovery

### Fixed
- Fingerprint template upload data corruption bug (#1)
  - Fixed by implementing proper structured packet reading with checksums
  - Added multi-stage timeout handling
  - Implemented bad-frame recovery loop

### Changed
- Protocol revised from initial drafts to v1 stable format
