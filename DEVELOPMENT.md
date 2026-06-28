# Development Guide & Release Workflow

This guide covers development setup, versioning practices, and release procedures for the FingerPrint Lock System project.

## 🛠️ Development Setup

### Initial Setup

```bash
# Clone the repository
git clone https://github.com/sas-bergson/fingerprint-arduino.git
cd fingerprint-arduino

# Create build directory
mkdir build && cd build

# Configure CMake
cmake ..

# Build the project
make

# (Optional) Generate API documentation
make docs
```

### Development Workflow

```bash
# Start new feature
git checkout -b feature/my-feature

# Make changes and commit (use conventional commits)
git commit -m "feat(component): description"

# Push to remote
git push origin feature/my-feature

# Create pull request on GitHub
# After review and approval:
git checkout main
git pull origin main
git merge feature/my-feature
git push origin main
```

### Changelog Management

To keep `CHANGELOG.md` synchronized with code changes, we use **conventional commits** and an automated helper script.

**For every commit:**
1. Use conventional format: `type(scope): description`
2. Git will show a template with changelog entry fields
3. Optionally add `[CHANGELOG]` metadata for user-facing changes

**When releasing a version:**
```bash
# Preview recent commits as changelog entries
./scripts/changelog-helper.sh --generate

# Generate and insert new version section
./scripts/changelog-helper.sh --version X.Y.Z --date YYYY-MM-DD

# Edit CHANGELOG.md to polish and consolidate
# Then commit:
git add CHANGELOG.md
git commit -m "docs: release vX.Y.Z"
```

**See full details:** [docs/CHANGELOG_WORKFLOW.md](docs/CHANGELOG_WORKFLOW.md)

## 📦 Semantic Versioning (SEMVER)

This project follows [Semantic Versioning 2.0.0](https://semver.org/).

### Version Format: `MAJOR.MINOR.PATCH[-PRERELEASE]`

Example: `1.2.3` or `2.0.0-rc.1`

### When to Increment

| Change                            | Action                                  | Example                 |
| --------------------------------- | --------------------------------------- | ----------------------- |
| Bug fixes, non-breaking           | Increment PATCH                         | 1.0.0 → 1.0.1           |
| New features, backward-compatible | Increment MINOR, reset PATCH to 0       | 1.0.5 → 1.1.0           |
| Breaking changes                  | Increment MAJOR, reset MINOR/PATCH to 0 | 1.5.0 → 2.0.0           |
| Pre-release                       | Append suffix                           | 2.0.0-alpha, 2.0.0-rc.1 |

### Breaking Changes

A **breaking change** occurs when:
- Protocol incompatibility introduced
- API signature changes requiring code updates
- Dependency removal or major version jump
- Data format incompatibility

**Always document breaking changes prominently in CHANGELOG.md**

## 🔖 Version Files

Versions are maintained in these locations:

1. **CMakeLists.txt** - Project VERSION field
   ```cmake
   project("fingerprintsensor" VERSION 1.0.0 ...)
   ```

2. **include/version.h** - Version macros (for C++ code)
   ```cpp
   #define FINGERPRINT_VERSION_MAJOR 1
   #define FINGERPRINT_VERSION_MINOR 0
   #define FINGERPRINT_VERSION_PATCH 0
   #define FINGERPRINT_VERSION "1.0.0"
   ```

3. **CHANGELOG.md** - Release notes and version history

## 🚀 Release Process

### Step 1: Prepare Release

1. **Update version in CMakeLists.txt:**
   ```cmake
   project("fingerprintsensor" VERSION 1.1.0 ...)  # Change here
   ```

2. **Update version in include/version.h:**
   ```cpp
   #define FINGERPRINT_VERSION_MAJOR 1
   #define FINGERPRINT_VERSION_MINOR 1
   #define FINGERPRINT_VERSION_PATCH 0
   #define FINGERPRINT_VERSION "1.1.0"
   ```

3. **Regenerate API documentation:**
   ```bash
   ./scripts/regenerate-docs.sh --clean
   # Verifies all code changes are properly documented
   ```

4. **Update CHANGELOG.md** - Move [Unreleased] to new release version:
   ```markdown
   ## [1.1.0] - 2026-06-28

   ### Added
   - New feature description
   
   ### Fixed
   - Bug fix description
   
   ## [Unreleased]
   
   (Now empty for next development cycle)
   ```

5. **Commit these changes:**
   ```bash
   git add CMakeLists.txt include/version.h CHANGELOG.md docs/html/
   git commit -m "chore: release v1.1.0"
   ```

### Step 2: Create Release Tag

```bash
# Create annotated tag
git tag -a v1.1.0 -m "Release version 1.1.0

### Features:
- Feature 1 description
- Feature 2 description

### Bug Fixes:
- Bug fix description

See CHANGELOG.md for full details."

# Verify tag
git tag -l -n v1.1.0

# Push tag to GitHub
git push origin v1.1.0
```

### Step 3: Build Release Artifacts

```bash
cd build
make clean
make
# Optional: Create distribution package
cpack
```

### Step 4: Create GitHub Release

1. Go to [GitHub Releases](https://github.com/sas-bergson/fingerprint-arduino/releases)
2. Click "Draft a new release"
3. Choose tag: `v1.1.0`
4. Title: `Version 1.1.0`
5. Description: Copy from CHANGELOG.md
6. Attach binary artifacts (if applicable)
7. Click "Publish release"

### Step 5: Post-Release

1. **Verify release** on GitHub
2. **Update project README** if version-dependent docs changed
3. **Announce** (email, social media, etc.)
4. **Monitor** for early issues with new release

## 🔄 Pre-Release Workflow

For beta/alpha releases:

```bash
# During development, use pre-release versions
VERSION="2.0.0-alpha"  # In version files

# Tag as pre-release
git tag -a v2.0.0-alpha -m "Alpha release for 2.0.0"

# Later: upgrade to RC
VERSION="2.0.0-rc.1"
git tag -a v2.0.0-rc.1 -m "Release candidate 1 for 2.0.0"

# Finally: stable release
VERSION="2.0.0"
git tag -a v2.0.0 -m "Stable release 2.0.0"
```

## 🚨 Hotfix (Patch Release)

For critical bug fixes to a released version:

```bash
# Create hotfix branch from release tag
git checkout -b hotfix/critical-bug v1.0.0

# Fix the bug
# Commit: git commit -m "fix: critical bug description"

# Update version to patch release
# In CMakeLists.txt: 1.0.1
# In include/version.h: 1.0.1
# In CHANGELOG.md: add [1.0.1] section

# Commit version bump
git commit -m "chore: release v1.0.1 (hotfix)"

# Tag release
git tag -a v1.0.1 -m "Hotfix: critical bug fix"

# Merge back to main
git checkout main
git merge hotfix/critical-bug
git push origin main
git push origin v1.0.1
```

## 📋 Automation Script

### bump-version.sh

Create `scripts/bump-version.sh` to automate version bumping:

```bash
#!/bin/bash
# scripts/bump-version.sh
# Usage: ./bump-version.sh [major|minor|patch] [--tag]

set -e

BUMP_TYPE="${1:-patch}"
AUTO_TAG="${2:-}"

# Extract current version from CMakeLists.txt
CURRENT=$(grep 'project.*VERSION' CMakeLists.txt | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)
echo "Current version: $CURRENT"

# Parse version
IFS='.' read -r MAJOR MINOR PATCH <<< "$CURRENT"

# Bump version
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
  *)
    echo "Usage: $0 [major|minor|patch] [--tag]"
    exit 1
    ;;
esac

NEW_VERSION="$MAJOR.$MINOR.$PATCH"
echo "New version: $NEW_VERSION"

# Update CMakeLists.txt
sed -i "s/VERSION [0-9]\+\.[0-9]\+\.[0-9]\+/VERSION $NEW_VERSION/" CMakeLists.txt

# Update include/version.h
sed -i "s/#define FINGERPRINT_VERSION_MAJOR [0-9]\+/#define FINGERPRINT_VERSION_MAJOR $MAJOR/" include/version.h
sed -i "s/#define FINGERPRINT_VERSION_MINOR [0-9]\+/#define FINGERPRINT_VERSION_MINOR $MINOR/" include/version.h
sed -i "s/#define FINGERPRINT_VERSION_PATCH [0-9]\+/#define FINGERPRINT_VERSION_PATCH $PATCH/" include/version.h
sed -i "s/#define FINGERPRINT_VERSION \"[^\"]*\"/#define FINGERPRINT_VERSION \"$NEW_VERSION\"/" include/version.h

echo "✓ Updated version files to $NEW_VERSION"

if [ "$AUTO_TAG" == "--tag" ]; then
  git add CMakeLists.txt include/version.h CHANGELOG.md
  git commit -m "chore: release v$NEW_VERSION"
  git tag -a "v$NEW_VERSION" -m "Release version $NEW_VERSION"
  echo "✓ Created git tag v$NEW_VERSION"
fi
```

Make it executable:
```bash
chmod +x scripts/bump-version.sh
```

Usage:
```bash
# Bump patch version
./scripts/bump-version.sh patch

# Bump minor version and create git tag
./scripts/bump-version.sh minor --tag

# Bump major version
./scripts/bump-version.sh major
```

## 📋 Release Checklist

Before release, verify:

- [ ] All PRs merged to main
- [ ] CHANGELOG.md updated with all changes
- [ ] Version numbers updated in all files (CMakeLists.txt, version.h)
- [ ] API documentation regenerated: `./scripts/regenerate-docs.sh --clean`
- [ ] Build passes locally: `make clean && make`
- [ ] Tests pass (when enabled): `ctest`
- [ ] Documentation builds successfully: `make docs`
- [ ] Documentation opens correctly: `open docs/html/index.html`
- [ ] API docs include all recent code changes from src/ and include/
- [ ] Git commit created with version bump
- [ ] Git tag created with semantic version
- [ ] GitHub release published with notes
- [ ] Artifacts uploaded (if applicable)
- [ ] docs/html/ directory committed to git

## 📝 Git Log Viewing

View project history:

```bash
# Show tags
git tag -l

# Show commits since version
git log v1.0.0..HEAD --oneline

# Show graph of commits
git log --graph --oneline --all

# Show specific version
git show v1.0.0

# Compare versions
git diff v1.0.0 v1.1.0
```

## 🔗 References

- [Semantic Versioning](https://semver.org/)
- [Keep a Changelog](https://keepachangelog.com/)
- [Conventional Commits](https://www.conventionalcommits.org/)
- [Git Tags](https://git-scm.com/book/en/v2/Git-Basics-Tagging)
- [GitHub Releases](https://docs.github.com/en/repositories/releasing-projects-on-github/about-releases)

## ❓ Questions?

Refer to [CONTRIBUTING.md](CONTRIBUTING.md) for contribution guidelines and contact information.
