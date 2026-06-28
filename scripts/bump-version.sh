#!/bin/bash
#
# Auto-increment project version based on recent git commits
# Reads conventional commits (feat:, fix:, BREAKING CHANGE:)
# Updates version.txt and CHANGELOG.md automatically
#
# Usage:
#   ./bump-version.sh              # Auto-detect and bump version
#   ./bump-version.sh --no-commit  # Preview version bump without committing
#

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
VERSION_FILE="$PROJECT_ROOT/version.txt"
CHANGELOG_FILE="$PROJECT_ROOT/CHANGELOG.md"

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Parse arguments
NO_COMMIT=false
if [[ "$1" == "--no-commit" ]]; then
  NO_COMMIT=true
fi

# Read current version
if [[ ! -f "$VERSION_FILE" ]]; then
  echo -e "${RED}Error: version.txt not found at $VERSION_FILE${NC}"
  exit 1
fi

CURRENT_VERSION=$(cat "$VERSION_FILE")
IFS='.' read -r MAJOR MINOR PATCH <<< "$CURRENT_VERSION"

echo -e "${YELLOW}Current version: $CURRENT_VERSION${NC}"

# Determine latest git tag for comparison
LATEST_TAG=""
if git -C "$PROJECT_ROOT" rev-parse "v$CURRENT_VERSION" >/dev/null 2>&1; then
  LATEST_TAG="v$CURRENT_VERSION"
else
  # Try to find any version tag
  LATEST_TAG=$(git -C "$PROJECT_ROOT" describe --tags --abbrev=0 2>/dev/null || echo "")
fi

if [[ -n "$LATEST_TAG" ]]; then
  echo "Latest tag: $LATEST_TAG"
  COMMIT_RANGE="$LATEST_TAG..HEAD"
else
  echo "No tags found, scanning recent commits"
  COMMIT_RANGE="HEAD~20..HEAD"
fi

# Analyze commits for version bump type
HAS_BREAKING=false
HAS_FEATURE=false
HAS_FIX=false

echo -e "\n${YELLOW}Analyzing commits in range: $COMMIT_RANGE${NC}"

# Check for BREAKING CHANGE in commit bodies
if git -C "$PROJECT_ROOT" log "$COMMIT_RANGE" --format="%b" 2>/dev/null | grep -q "^BREAKING CHANGE:"; then
  HAS_BREAKING=true
  echo "  ✓ Found BREAKING CHANGE"
fi

# Check for feat: commits
FEATURE_COUNT=$(git -C "$PROJECT_ROOT" log "$COMMIT_RANGE" --format="%s" 2>/dev/null | grep -c "^feat" || true)
if [[ $FEATURE_COUNT -gt 0 ]]; then
  HAS_FEATURE=true
  echo "  ✓ Found $FEATURE_COUNT feat: commits"
fi

# Check for fix: commits
FIX_COUNT=$(git -C "$PROJECT_ROOT" log "$COMMIT_RANGE" --format="%s" 2>/dev/null | grep -c "^fix" || true)
if [[ $FIX_COUNT -gt 0 ]]; then
  HAS_FIX=true
  echo "  ✓ Found $FIX_COUNT fix: commits"
fi

# Determine new version
BUMP_TYPE=""
if [[ "$HAS_BREAKING" == true ]]; then
  NEW_MAJOR=$((MAJOR + 1))
  NEW_VERSION="$NEW_MAJOR.0.0"
  BUMP_TYPE="MAJOR (breaking change)"
elif [[ "$HAS_FEATURE" == true ]]; then
  NEW_MINOR=$((MINOR + 1))
  NEW_VERSION="$MAJOR.$NEW_MINOR.0"
  BUMP_TYPE="MINOR (new features)"
elif [[ "$HAS_FIX" == true ]]; then
  NEW_PATCH=$((PATCH + 1))
  NEW_VERSION="$MAJOR.$MINOR.$NEW_PATCH"
  BUMP_TYPE="PATCH (bug fixes)"
else
  echo -e "\n${YELLOW}No version-triggering commits found${NC}"
  echo "  Need: feat:, fix:, or BREAKING CHANGE: in commit messages"
  exit 0
fi

echo -e "\n${GREEN}Version bump: $CURRENT_VERSION → $NEW_VERSION ($BUMP_TYPE)${NC}"

if [[ "$NO_COMMIT" == true ]]; then
  echo -e "${YELLOW}Preview mode (--no-commit): Not making changes${NC}"
  exit 0
fi

# Update version.txt
echo "$NEW_VERSION" > "$VERSION_FILE"
echo "✓ Updated version.txt"

# Update CHANGELOG.md - change [Unreleased] to [NEW_VERSION] with date
if grep -q "## \[Unreleased\]" "$CHANGELOG_FILE"; then
  DATE=$(date +%Y-%m-%d)
  sed -i "s/## \[Unreleased\]/## [$NEW_VERSION] - $DATE/" "$CHANGELOG_FILE"
  echo "✓ Updated CHANGELOG.md"
fi

# Git add and commit
cd "$PROJECT_ROOT"
git add version.txt "$CHANGELOG_FILE"
git commit -m "chore(release): bump version to $NEW_VERSION

$BUMP_TYPE"

echo -e "\n${GREEN}✓ Version bump committed!${NC}"
echo "Commit: $(git rev-parse --short HEAD)"
echo ""
echo "Next step: Run 'scripts/release.sh' to tag and push"

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
