#!/bin/bash
#
# bump-version.sh - Semantic Versioning automation script
#
# Usage: ./bump-version.sh [major|minor|patch] [--tag]
#
# Examples:
#   ./bump-version.sh patch              # Bump patch version
#   ./bump-version.sh minor --tag        # Bump minor and create git tag
#   ./bump-version.sh major              # Bump major version
#
# This script updates version numbers in:
#   - CMakeLists.txt
#   - include/version.h
#   - Git tags (if --tag flag is set)
#

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
CMAKE_FILE="$PROJECT_ROOT/CMakeLists.txt"
VERSION_HEADER="$PROJECT_ROOT/include/version.h"
CHANGELOG_FILE="$PROJECT_ROOT/CHANGELOG.md"

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Parse arguments
BUMP_TYPE="${1:-patch}"
AUTO_TAG="${2:-}"
COMMIT_MESSAGE_FLAG=false

# Validate arguments
if [[ ! "$BUMP_TYPE" =~ ^(major|minor|patch)$ ]]; then
    echo -e "${RED}Error: Invalid bump type. Use: major, minor, or patch${NC}"
    echo "Usage: $0 [major|minor|patch] [--tag]"
    exit 1
fi

# Extract current version from CMakeLists.txt
if ! CURRENT=$(grep -E 'project.*VERSION' "$CMAKE_FILE" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1); then
    echo -e "${RED}Error: Could not find version in $CMAKE_FILE${NC}"
    exit 1
fi

echo -e "${YELLOW}Current version: $CURRENT${NC}"

# Parse version components
IFS='.' read -r MAJOR MINOR PATCH <<< "$CURRENT"

# Bump version based on type
case "$BUMP_TYPE" in
    major)
        MAJOR=$((MAJOR + 1))
        MINOR=0
        PATCH=0
        ;;
    minor)
        MINOR=$((MINOR + 1))
        PATCH=0
        ;;
    patch)
        PATCH=$((PATCH + 1))
        ;;
esac

NEW_VERSION="$MAJOR.$MINOR.$PATCH"
echo -e "${YELLOW}New version: $NEW_VERSION${NC}"

# Verify files exist
if [[ ! -f "$CMAKE_FILE" ]]; then
    echo -e "${RED}Error: CMakeLists.txt not found at $CMAKE_FILE${NC}"
    exit 1
fi

if [[ ! -f "$VERSION_HEADER" ]]; then
    echo -e "${RED}Error: version.h not found at $VERSION_HEADER${NC}"
    exit 1
fi

# Update CMakeLists.txt
echo "Updating CMakeLists.txt..."
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS sed requires -i ''
    sed -i '' "s/VERSION [0-9]\+\.[0-9]\+\.[0-9]\+/VERSION $NEW_VERSION/" "$CMAKE_FILE"
else
    # Linux sed
    sed -i "s/VERSION [0-9]\+\.[0-9]\+\.[0-9]\+/VERSION $NEW_VERSION/" "$CMAKE_FILE"
fi

# Update include/version.h
echo "Updating include/version.h..."
if [[ "$OSTYPE" == "darwin"* ]]; then
    sed -i '' "s/#define FINGERPRINT_VERSION_MAJOR [0-9]\+/#define FINGERPRINT_VERSION_MAJOR $MAJOR/" "$VERSION_HEADER"
    sed -i '' "s/#define FINGERPRINT_VERSION_MINOR [0-9]\+/#define FINGERPRINT_VERSION_MINOR $MINOR/" "$VERSION_HEADER"
    sed -i '' "s/#define FINGERPRINT_VERSION_PATCH [0-9]\+/#define FINGERPRINT_VERSION_PATCH $PATCH/" "$VERSION_HEADER"
    sed -i '' "s/#define FINGERPRINT_VERSION \"[^\"]*\"/#define FINGERPRINT_VERSION \"$NEW_VERSION\"/" "$VERSION_HEADER"
else
    sed -i "s/#define FINGERPRINT_VERSION_MAJOR [0-9]\+/#define FINGERPRINT_VERSION_MAJOR $MAJOR/" "$VERSION_HEADER"
    sed -i "s/#define FINGERPRINT_VERSION_MINOR [0-9]\+/#define FINGERPRINT_VERSION_MINOR $MINOR/" "$VERSION_HEADER"
    sed -i "s/#define FINGERPRINT_VERSION_PATCH [0-9]\+/#define FINGERPRINT_VERSION_PATCH $PATCH/" "$VERSION_HEADER"
    sed -i "s/#define FINGERPRINT_VERSION \"[^\"]*\"/#define FINGERPRINT_VERSION \"$NEW_VERSION\"/" "$VERSION_HEADER"
fi

echo -e "${GREEN}✓ Updated version files to $NEW_VERSION${NC}"

# Optional: Create git tag
if [[ "$AUTO_TAG" == "--tag" ]]; then
    echo "Creating git tag..."
    
    # Check if git is available
    if ! command -v git &> /dev/null; then
        echo -e "${RED}Error: Git not found in PATH${NC}"
        exit 1
    fi
    
    # Check if we're in a git repository
    if ! git rev-parse --git-dir > /dev/null 2>&1; then
        echo -e "${RED}Error: Not in a git repository${NC}"
        exit 1
    fi
    
    # Stage changes
    git add "$CMAKE_FILE" "$VERSION_HEADER"
    
    # Add CHANGELOG.md if it exists and has unreleased changes
    if [[ -f "$CHANGELOG_FILE" ]]; then
        git add "$CHANGELOG_FILE"
    fi
    
    # Create commit
    git commit -m "chore: release v$NEW_VERSION" || {
        echo -e "${YELLOW}Note: No changes to commit (files may already be up-to-date)${NC}"
    }
    
    # Create annotated tag
    git tag -a "v$NEW_VERSION" -m "Release version $NEW_VERSION" || {
        echo -e "${RED}Error: Could not create git tag (tag may already exist)${NC}"
        exit 1
    }
    
    echo -e "${GREEN}✓ Created git tag v$NEW_VERSION${NC}"
    echo -e "${YELLOW}Next: Push tag with: git push origin v$NEW_VERSION${NC}"
fi

echo -e "${GREEN}✓ Version bump complete!${NC}"
echo ""
echo "Summary:"
echo "  Old version: $CURRENT"
echo "  New version: $NEW_VERSION"
echo "  Files updated:"
echo "    - CMakeLists.txt"
echo "    - include/version.h"

if [[ "$AUTO_TAG" == "--tag" ]]; then
    echo "    - Git tag: v$NEW_VERSION"
    echo ""
    echo "Next steps:"
    echo "  1. Review changes: git log -1"
    echo "  2. Push changes: git push origin main"
    echo "  3. Push tag: git push origin v$NEW_VERSION"
    echo "  4. Create release on GitHub"
else
    echo ""
    echo "Next steps:"
    echo "  1. Review changes: git diff"
    echo "  2. Update CHANGELOG.md with release notes"
    echo "  3. Commit: git commit -am 'chore: release v$NEW_VERSION'"
    echo "  4. Tag: git tag -a v$NEW_VERSION -m 'Release $NEW_VERSION'"
    echo "  5. Push: git push origin main && git push origin v$NEW_VERSION"
fi
