#!/bin/bash
#
# regenerate-docs.sh - Regenerate API documentation via the CMake build system.
#
# This script is a thin wrapper around `cmake --build`. CMake owns the correct
# working directory and dependency logic, so invoking it here avoids the path
# issues that occur when calling doxygen directly from an arbitrary directory.
#
# Usage:
#   ./scripts/regenerate-docs.sh              # Rebuild docs (incremental)
#   ./scripts/regenerate-docs.sh --clean      # Delete old docs, then rebuild
#   ./scripts/regenerate-docs.sh --help       # Show this message
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
DOCS_HTML="$PROJECT_ROOT/docs/html/index.html"

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

CLEAN_MODE=false

for arg in "$@"; do
    case "$arg" in
        --clean)  CLEAN_MODE=true ;;
        --help|-h)
            sed -n '2,12p' "$0" | sed 's/^# \?//'
            exit 0
            ;;
        *)
            echo "Unknown option: $arg  (run with --help for usage)"
            exit 1
            ;;
    esac
done

# ------------------------------------------------------------------
# Ensure the build directory exists and is configured
# ------------------------------------------------------------------
if [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
    echo -e "${YELLOW}Build directory not configured. Running cmake...${NC}"
    cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR"
fi

# ------------------------------------------------------------------
# Optional clean: remove old generated output and stamp
# ------------------------------------------------------------------
if [[ "$CLEAN_MODE" == true ]]; then
    echo -e "${YELLOW}Cleaning old documentation...${NC}"
    rm -rf "$PROJECT_ROOT/docs/html" "$PROJECT_ROOT/docs/latex" \
           "$PROJECT_ROOT/docs/doxygen.stamp"
    echo -e "${GREEN}✓ Old documentation removed${NC}"
fi

# ------------------------------------------------------------------
# Build the docs target — CMake handles working directory and deps
# ------------------------------------------------------------------
echo -e "${YELLOW}Generating API documentation...${NC}"
cmake --build "$BUILD_DIR" --target docs

echo ""
echo -e "${GREEN}✓ Documentation generation complete!${NC}"
echo "  HTML: file://$DOCS_HTML"
