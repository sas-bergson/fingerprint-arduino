# Fingerprint Exports Directory

This directory is automatically created when running the fingerprint service to organize exported templates and images.

## Directory Structure

```
exports/
├── templates/     # Fingerprint template data (hex and binary formats)
│   ├── 2026-06-28.13-45-22_id1.txt
│   ├── 2026-06-28.13-45-22_id1.bin
│   ├── 2026-06-29.14-30-15_id2.txt
│   └── 2026-06-29.14-30-15_id2.bin
└── images/        # Fingerprint images (BMP format)
    ├── fingerprint_2026-06-28.13-47-11.bmp
    └── fingerprint_2026-06-29.14-32-45.bmp
```

## Files

### Template Exports (`templates/` subdirectory)

Each fingerprint enrollment creates two files with the same timestamp and ID:

- **`.txt` file** — Hex-encoded template data (human-readable, one byte per space-separated pair)
  ```
  6F 00 E7 FF E7 FF BF 7F DF 7F EF 7F F7 7F ...
  ```
  
- **`.bin` file** — Raw binary template data (256-306 bytes typical)

**Naming:** `YYYY-MM-DD.HH-MM-SS_idN.{txt,bin}`
- `YYYY-MM-DD.HH-MM-SS` — Export timestamp
- `N` — Fingerprint ID (1-127)

### Image Exports (`images/` subdirectory)

Each fingerprint image capture creates a single BMP file:

- **`.bmp` file** — 8-bit grayscale Windows BMP image (uncompressed)
  - Dimensions: 256×288 pixels (typical for Adafruit sensors)
  - Palette: Standard 256-entry grayscale (0=black, 255=white)
  - Format: Row-major with proper Windows padding

**Naming:** `fingerprint_YYYY-MM-DD.HH-MM-SS.bmp`
- `YYYY-MM-DD.HH-MM-SS` — Export timestamp

## Usage

### View Fingerprint Templates

```bash
# List all templates
ls -lh exports/templates/

# Display hex content
cat exports/templates/2026-06-28.13-45-22_id1.txt

# View binary size
ls -la exports/templates/2026-06-28.13-45-22_id1.bin
```

### View Fingerprint Images

```bash
# List all captured images
ls -lh exports/images/

# Open in image viewer
feh exports/images/fingerprint_2026-06-28.13-47-11.bmp
gimp exports/images/fingerprint_2026-06-28.13-47-11.bmp
# Or any Windows-compatible BMP viewer (Photoshop, Paint, Preview, etc.)
```

## Import/Use Templates

### Using Templates for Authentication

To verify a fingerprint against stored templates:

1. **Convert hex to binary** (if needed):
   ```bash
   # Using Python
   python3 -c "
   with open('template.txt', 'r') as f:
     hex_data = f.read().split()
   with open('template.bin', 'wb') as f:
     f.write(bytes(int(x, 16) for x in hex_data))
   "
   ```

2. **Upload to fingerprint sensor** (via Arduino sketch):
   - Modify `FingerPrint_Enroll.ino` to load template from serial
   - Or implement a custom template upload endpoint

### Backing Up Templates

```bash
# Archive all exports
tar -czf fingerprint_exports_backup.tar.gz exports/

# Copy to external storage
cp -r exports/ /mnt/usb/backup/
```

## File Size Reference

### Template Files
- Typical size: 2.7–3.0 KB per template
- Hex format (.txt): ~3× larger than binary
- Adafruit sensor capacity: 127 fingerprints

### Image Files
- Grayscale 8-bit BMP: ~75 KB per 256×288 image
- Uncompressed (no compression)
- Same as standard fingerprint scanner output

## Cleanup

To remove old exports while keeping the directory structure:

```bash
# Remove all exports but keep subdirectories
rm -rf exports/templates/* exports/images/

# Or remove entire exports directory (will be recreated on next run)
rm -rf exports/
```

## Note on Version Control

The `exports/` directory is **excluded from Git** (see `.gitignore`). This is intentional because:
- Exports are user-generated data (templates/images)
- Files are too large for Git
- Biometric data should not be committed to version control

Always back up important exports separately and keep them secure.
