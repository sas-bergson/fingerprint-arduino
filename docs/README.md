# API Documentation Guide

This directory contains the Doxygen configuration and generated API documentation for the FingerPrint Lock System project.

## Quick Start

### Regenerate Documentation

To rebuild the API documentation after code changes:

```bash
# Using the regeneration script (recommended)
./scripts/regenerate-docs.sh

# Using CMake
mkdir -p build && cd build
cmake ..
make docs

# Using Doxygen directly
cd docs
doxygen doxygen.cfg
```

### View Documentation

- **HTML Documentation:** Open `docs/html/index.html` in a web browser
- **LaTeX/PDF Documentation:** Requires LaTeX installation
  ```bash
  cd docs/latex
  make
  # Creates refman.pdf
  ```

## Documentation Structure

### Generated Documentation Covers

✅ **Header Files** (`include/`)
- `serialport.hpp` - Cross-platform serial communication wrapper
- `version.h` - Version information and macros

✅ **Source Code** (`src/`)
- `main.cpp` - Host-side command client with interactive service menu
- `FingerPrint_Enroll.ino` - Arduino firmware with fingerprint sensor protocol implementation

✅ **Protocol Documentation** (`docs/`)
- `protocol.md` - Host ↔ Firmware communication specification

## Key Components Documented

### Host Application (main.cpp)
- **Flash Mode:** Arduino sketch compilation and upload automation using arduino-cli
- **Service Mode:** Interactive CLI menu for fingerprint operations
- **Serial Communication:** Protocol implementation and message handling
- **Template/Image Export:** Streaming and file persistence
- **Progress Display:** Async progress tracking for long transfers

### Arduino Firmware (FingerPrint_Enroll.ino)
- **Protocol Handler:** Command parsing and response generation
- **Sensor Integration:** Adafruit fingerprint sensor library usage
- **Message Types:** EVT, RSP, TPL, IMG packet implementations
- **Workflows:** Fingerprint enrollment, capture, and template streaming
- **Error Handling:** Status codes and recovery mechanisms

### Serial Port Library (serialport.hpp)
- Cross-platform UART communication (Linux/Windows/macOS)
- Non-blocking I/O operations
- Timeout handling

## Configuration Files

### doxygen.cfg
Main Doxygen configuration file that controls:
- Input directories: `../../include/` and `../../src/`
- Output directory: `../../docs/html/`
- Output format: HTML, LaTeX/PDF
- Search functionality
- Cross-references

**Key settings:**
```
INPUT = "../../include" "../../src"
RECURSIVE = YES
EXTRACT_ALL = NO
```

### CMakeLists.txt (docs/)
CMake build target that:
- Runs Doxygen automatically during `make docs`
- Creates `doxygen.stamp` timestamp file
- Compiles LaTeX documentation (optional)
- Integrated into main project build

## Automatic Documentation Build

The documentation is automatically built as part of the CMake build process:

```bash
cd build
cmake ..
make  # Builds both application AND docs
```

To rebuild only documentation:
```bash
make docs
```

## Recent Updates

Recent major enhancements now documented:

### main.cpp (v1.0.0)
- ✅ Flash mode with arduino-cli automation
- ✅ Interactive service menu
- ✅ Protocol implementation with EVT/RSP/TPL/IMG parsing
- ✅ Async progress display for transfers
- ✅ Template and image file persistence (binary + text formats)
- ✅ BMP image export with grayscale palette
- ✅ Multithreading support for long operations
- ✅ Sensor baud-rate fallback (57600 ↔ 19200)

### FingerPrint_Enroll.ino (v1.0.0)
- ✅ Structured packet reading with timeout handling
- ✅ Image capture with stream retry logic
- ✅ Template export in TPL packet format
- ✅ Grayscale image export in IMB packet format
- ✅ Comprehensive error codes (400+ range)
- ✅ Frame-based packet reassembly
- ✅ Sensor UART baud negotiation
- ✅ Multithreading-safe operations

## Dependencies

### Required for Documentation Generation
- **Doxygen** - Documentation generator
  ```bash
  # Ubuntu/Debian
  sudo apt-get install doxygen
  
  # macOS
  brew install doxygen
  ```

### Optional for Enhanced Diagrams
- **Graphviz** - Generates visual call/relationship diagrams
  ```bash
  # Ubuntu/Debian
  sudo apt-get install graphviz
  
  # macOS
  brew install graphviz
  ```

### Optional for PDF Generation
- **LaTeX** - PDF documentation
  ```bash
  # Ubuntu/Debian
  sudo apt-get install texlive-full
  
  # macOS
  brew install --cask mactex
  ```

## Troubleshooting

### Doxygen not found
```bash
# Install Doxygen
sudo apt-get install doxygen  # Linux
brew install doxygen          # macOS
```

### Documentation not updating
```bash
# Force rebuild
./scripts/regenerate-docs.sh --clean

# Or manually
rm docs/doxygen.stamp
make docs
```

### Graphviz diagrams not rendering
```bash
# Install Graphviz
sudo apt-get install graphviz  # Linux
brew install graphviz          # macOS
```

### PDF generation fails
```bash
# Install LaTeX
sudo apt-get install texlive-full  # Linux
brew install --cask mactex         # macOS
```

## Documentation Standards

All source files follow Doxygen documentation standards:

### C++ Comments
```cpp
/**
 * @brief Short description of function
 * 
 * Longer detailed description if needed.
 * 
 * @param paramName Description of parameter
 * @return Description of return value
 * @throws ExceptionType Exception documentation
 * 
 * @see relatedFunction()
 * @note Important note
 */
```

### Arduino Sketch Comments
```cpp
/**
 * @file FingerPrint_Enroll.ino
 * @brief Brief description
 * 
 * Longer description explaining the sketch purpose,
 * protocol implementation, and key functions.
 */
```

## Maintenance

### Before Each Release
1. Ensure all new functions have Doxygen comments
2. Regenerate documentation:
   ```bash
   ./scripts/regenerate-docs.sh --clean
   ```
3. Verify HTML output in `docs/html/index.html`
4. Commit updated `docs/html/` directory to git
5. Update CHANGELOG.md with documentation improvements

### Version Synchronization
Documentation version is tied to project version in:
- `CMakeLists.txt` - PROJECT VERSION
- `doxygen.cfg` - PROJECT_NUMBER (optional)
- `include/version.h` - FINGERPRINT_VERSION

## Related Documentation

For information on **how to use** the compiled application:
- **[Usage Guide](USAGE.md)** - Building, flashing Arduino, and running the interactive service menu

For project contribution and development:
- **[Development Guide](../DEVELOPMENT.md)** - Development setup, versioning, and release workflow
- **[Changelog Workflow](CHANGELOG_WORKFLOW.md)** - How to keep CHANGELOG.md in sync with code changes

For protocol implementation details:
- **[Protocol Specification](protocol.md)** - Low-level host ↔ firmware communication

## Contributing

When submitting code changes:
1. Add Doxygen comments to new functions/classes
2. Document parameters and return values
3. Include `@brief` and detailed descriptions
4. Run `./scripts/regenerate-docs.sh` to verify
5. Include updated docs in your commit

---

**Last Updated:** 2026-06-28  
**Documentation Format:** Doxygen HTML + LaTeX  
**Coverage:** 100% of public API
