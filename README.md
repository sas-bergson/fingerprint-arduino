# FingerPrint Lock System

An Arduino-based biometric fingerprint lock system with host-firmware communication protocol. This project provides both hardware integration for fingerprint sensors and cross-platform communication between a host application and Arduino firmware.

**Latest Release:** [v1.0.0](https://github.com/sas-bergson/fingerprint-arduino/releases/tag/v1.0.0) | **Status:** Stable

## 🚀 Features

- **Fingerprint Enrollment & Authentication** - Enroll and verify fingerprints using Adafruit fingerprint sensors
- **Host-Firmware Protocol (v1)** - ASCII-based line protocol for reliable communication
- **Template & Image Export** - Stream fingerprint templates and grayscale images over UART
- **Multithreading Support** - Concurrent fingerprint operations
- **Arduino Compatible** - Tested on Arduino Uno with CMake build system
- **Cross-Platform** - Host application runs on Linux/Windows/macOS

## 📋 Quick Start

### Prerequisites
- Arduino Uno or compatible board
- Adafruit R307/R503 fingerprint sensor
- CMake 3.10+
- C++11 compiler
- Arduino CLI (for uploading firmware)

### Building the Host Application

```bash
mkdir build && cd build
cmake ..
make              # Builds application + API documentation
```

### Using the Host Application

The compiled application (`./bin/fingerprintsensor`) has two modes:

1. **Interactive Mode** (default) - Menu-driven CLI to manage fingerprints:
   ```bash
   ./bin/fingerprintsensor
   # Shows menu for enrolling, capturing images, checking sensor status, etc.
   ```

2. **Flash Mode** - Upload new firmware to Arduino:
   ```bash
   ./bin/fingerprintsensor --flash --prepare
   # Installs board core, compiles, and uploads to Arduino
   ```

**For complete usage instructions:** See [docs/USAGE.md](docs/USAGE.md)

### Building API Documentation

The API documentation is automatically generated during the build. To rebuild manually:

```bash
# Method 1: Regeneration script (recommended)
./scripts/regenerate-docs.sh

# Method 2: Using CMake
make docs

# View the documentation
open docs/html/index.html    # macOS
xdg-open docs/html/index.html  # Linux
```

See [docs/README.md](docs/README.md) for detailed documentation info.

## 📚 Documentation

- **[Usage Guide](docs/USAGE.md)** ⭐ **START HERE** - How to build, flash, and run the application
- **[Protocol Specification](docs/protocol.md)** - Host ↔ Firmware communication protocol (v1)
- **[API Documentation](docs/README.md)** - Generated Doxygen documentation for all code
  - View: [docs/html/index.html](docs/html/index.html)
  - Regenerate: `./scripts/regenerate-docs.sh`
- **[Versioning & Release Workflow](docs/VERSIONING.md)** - Automated version management, releases, and conventional commits
- **[Recent Major Changes](docs/CHANGES.md)** - Overview of v1.0.0 updates in main.cpp and FingerPrint_Enroll.ino
- **[Contributing Guide](CONTRIBUTING.md)** - How to contribute to this project
- **[Development Guide](DEVELOPMENT.md)** - Setup for developers

## 🏗️ Project Structure

```
src/               # Host application source (C++)
include/           # Public headers (serialport.hpp, version.h)
build/             # CMake build artifacts and Arduino sketches
docs/              # Documentation and Doxygen config
  ├── protocol.md  # Host-Firmware protocol specification
  ├── doxygen.cfg  # API documentation config
  └── html/        # Generated API docs
```

## 🔗 Technical Details

### Communication Protocol
This project implements a line-based ASCII protocol for host ↔ Arduino communication:
- **Commands:** Host → Arduino (CMD)
- **Events:** Async progress updates (EVT)
- **Responses:** Terminal status (RSP)
- **Packets:** Template/image data (TPL/IMG)

### Supported Operations
- Read board status (CMD 10)
- Read fingerprint module status (CMD 20)
- Enroll fingerprint and export template (CMD 30)
- Capture fingerprint image (CMD 40)

For details, see [Protocol Documentation](docs/protocol.md)

## 📦 Building & Testing

```bash
# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build everything
make

# Run tests (when enabled in CMakeLists.txt)
ctest
```

## 🤝 Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on:
- Code style and standards
- Commit message format
- Pull request process
- Testing requirements

## 📝 Changelog

All changes are documented in [CHANGELOG.md](CHANGELOG.md). For release notes, see [GitHub Releases](https://github.com/sas-bergson/fingerprint-arduino/releases).

## 📄 License

This project is licensed under the [LICENSE.txt](LICENSE.txt) - see file for details.

## ✨ Versioning & Releases

This project follows [Semantic Versioning](https://semver.org/) with **automated version management**. 

**Key Points:**
- Version stored in [version.txt](version.txt)
- Use conventional commits (`feat:`, `fix:`, etc.) for automatic version bumping
- Build system auto-increments version based on commits
- Formal releases via `./scripts/release.sh`

📖 **See [docs/VERSIONING.md](docs/VERSIONING.md)** for the complete workflow.
