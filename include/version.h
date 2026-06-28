/**
 * @file version.h
 * @brief Version information for FingerPrint Lock System
 *
 * This file contains version macros used to track the release version
 * of the FingerPrint Lock System. Update these values before each release.
 *
 * Version format: MAJOR.MINOR.PATCH[PRERELEASE]
 * See DEVELOPMENT.md for versioning guidelines
 */

#ifndef FINGERPRINT_VERSION_H
#define FINGERPRINT_VERSION_H

/// @brief Major version number
#define FINGERPRINT_VERSION_MAJOR 1

/// @brief Minor version number
#define FINGERPRINT_VERSION_MINOR 0

/// @brief Patch version number
#define FINGERPRINT_VERSION_PATCH 0

/// @brief Pre-release identifier (empty string for stable releases)
/// Examples: "", "alpha", "beta", "rc.1"
#define FINGERPRINT_VERSION_PRERELEASE ""

/// @brief Full version string (e.g., "1.0.0" or "2.0.0-rc.1")
#define FINGERPRINT_VERSION "1.0.0"

/// @brief Version string with CMake integration
/// Can be used in compiler flags or runtime version checks
#define FINGERPRINT_VERSION_STRING "FingerPrint Lock System v" FINGERPRINT_VERSION

#endif // FINGERPRINT_VERSION_H
