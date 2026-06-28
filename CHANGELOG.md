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

## [1.0.0] - 2026-01-15

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
