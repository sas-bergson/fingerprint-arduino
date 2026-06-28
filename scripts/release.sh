#!/bin/bash
#
# Formal release workflow - tags, pushes, and creates release documentation
#
# Usage:
#   ./release.sh                  # Interactive release
#   ./release.sh --auto           # Auto-tag and push latest version
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
BLUE='\033[0;34m'
NC='\033[0m' # No Color

AUTO_MODE=false
if [[ "$1" == "--auto" ]]; then
  AUTO_MODE=true
fi

# Verify we're in a git repo
if ! git rev-parse --git-dir > /dev/null 2>&1; then
  echo -e "${RED}Error: Not in a git repository${NC}"
  exit 1
fi

# Read current version
if [[ ! -f "$VERSION_FILE" ]]; then
  echo -e "${RED}Error: version.txt not found${NC}"
  exit 1
fi

VERSION=$(cat "$VERSION_FILE")
TAG="v$VERSION"

echo -e "${BLUE}════════════════════════════════════════${NC}"
echo -e "${BLUE}  Fingerprint Lock System Release${NC}"
echo -e "${BLUE}════════════════════════════════════════${NC}"
echo ""
echo "Version: $VERSION"
echo "Tag: $TAG"
echo ""

# Check if tag already exists
if git rev-parse "$TAG" >/dev/null 2>&1; then
  echo -e "${RED}Error: Tag $TAG already exists${NC}"
  exit 1
fi

# Show release notes from CHANGELOG
echo -e "${YELLOW}Release Notes:${NC}"
if [[ -f "$CHANGELOG_FILE" ]]; then
  # Extract version section from CHANGELOG
  sed -n "/^## \[$VERSION\]/,/^## \[/p" "$CHANGELOG_FILE" | head -n -1 | sed 's/^/  /'
else
  echo "  (no CHANGELOG found)"
fi

echo ""

# Confirm release
if [[ "$AUTO_MODE" == false ]]; then
  echo -e "${YELLOW}Proceed with release? (y/N)${NC}"
  read -r CONFIRM
  if [[ "$CONFIRM" != "y" && "$CONFIRM" != "Y" ]]; then
    echo "Cancelled."
    exit 0
  fi
fi

# Get current branch
CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)

# Verify working tree is clean
if [[ -n $(git status --porcelain) ]]; then
  echo -e "${RED}Error: Working tree is dirty. Commit changes first.${NC}"
  exit 1
fi

# Verify we're on main/master branch
if [[ "$CURRENT_BRANCH" != "main" && "$CURRENT_BRANCH" != "master" ]]; then
  echo -e "${YELLOW}Warning: You're on branch '$CURRENT_BRANCH', not main/master${NC}"
  if [[ "$AUTO_MODE" == false ]]; then
    echo "Continue? (y/N)"
    read -r CONFIRM
    if [[ "$CONFIRM" != "y" && "$CONFIRM" != "Y" ]]; then
      exit 0
    fi
  fi
fi

echo ""
echo -e "${GREEN}Creating release...${NC}"

# Create annotated tag
git tag -a "$TAG" -m "Release version $VERSION

Changes:
$(sed -n "/^## \[$VERSION\]/,/^## \[/p" "$CHANGELOG_FILE" | head -n -1 | sed 's/^/  /')"

echo -e "${GREEN}✓ Created tag $TAG${NC}"

# Push commits
echo -e "${YELLOW}Pushing commits to origin...${NC}"
git push origin "$CURRENT_BRANCH"
echo -e "${GREEN}✓ Pushed commits${NC}"

# Push tag
echo -e "${YELLOW}Pushing tag to origin...${NC}"
git push origin "$TAG"
echo -e "${GREEN}✓ Pushed tag $TAG${NC}"

echo ""
echo -e "${GREEN}════════════════════════════════════════${NC}"
echo -e "${GREEN}  Release Complete!${NC}"
echo -e "${GREEN}════════════════════════════════════════${NC}"
echo ""
echo "Version: $VERSION"
echo "Tag: $TAG"
echo "Repository: $(git config --get remote.origin.url)"
echo ""
echo -e "${BLUE}Summary:${NC}"
echo "  • Version bumped to $VERSION in version.txt"
echo "  • CHANGELOG.md updated with release notes"
echo "  • Git tag created: $TAG"
echo "  • Changes pushed to origin/$CURRENT_BRANCH"
echo "  • Tag pushed to origin"
echo ""
echo -e "${BLUE}Next steps:${NC}"
echo "  1. Check GitHub releases page to verify tag"
echo "  2. Optionally create a GitHub Release with notes"
echo "  3. Announce release to users"
echo ""
