# Versioning & Release Workflow

This project uses **Semantic Versioning (SemVer)** with **automated version management** integrated into the build system.

## Version File

Version is stored in a single source of truth: **`version.txt`**

```bash
cat version.txt
# Output: 1.0.0
```

The build system (`CMakeLists.txt`) reads this file automatically and uses it for:
- Project version reporting
- Build artifacts
- Release tags

## Semantic Versioning

Format: `MAJOR.MINOR.PATCH`

- **MAJOR** (1.0.0) — Breaking changes to API or functionality
- **MINOR** (1.1.0) — New features, backward compatible
- **PATCH** (1.0.1) — Bug fixes, no new features

## Conventional Commits

To enable automatic version management, use **conventional commit format** in your git messages:

### Format

```
<type>(<scope>): <subject>

<body>

<footer>
```

### Types That Trigger Version Bumps

| Type               | SemVer | Example                                           |
| ------------------ | ------ | ------------------------------------------------- |
| `feat:`            | MINOR  | `feat(main.cpp): add image capture support`       |
| `fix:`             | PATCH  | `fix(firmware): correct sensor baud rate`         |
| `BREAKING CHANGE:` | MAJOR  | Include in footer: `BREAKING CHANGE: API renamed` |

### Non-Version Types

These do **NOT** trigger version bumps:
- `docs:` — Documentation updates
- `chore:` — Maintenance, dependencies
- `refactor:` — Code cleanup
- `perf:` — Performance improvements
- `test:` — Test additions

### Example Commits

```bash
# Feature (minor version bump)
git commit -m "feat(enrollment): add dual-capture workflow"

# Bug fix (patch version bump)
git commit -m "fix(image export): organize files into subdirectories"

# Breaking change (major version bump)
git commit -m "feat(protocol): redesigned packet format

BREAKING CHANGE: Host firmware protocol v1 → v2"

# Documentation (no version bump)
git commit -m "docs(readme): update usage examples"
```

## Automated Version Management

### How It Works

1. **Build-time Check** — When you run `make`, CMakeLists.txt calls `scripts/bump-version.sh`
2. **Commit Analysis** — Script scans recent git commits for conventional types
3. **Auto-Bump** — If `feat:`, `fix:`, or `BREAKING CHANGE:` found, version increments
4. **CHANGELOG Update** — Updates CHANGELOG.md with release section
5. **Git Commit** — Creates a "chore(release)" commit

### During Development

```bash
# Make a fix
git commit -m "fix(sensor): improve error handling"

# Build project (auto-version bump happens here)
cd build && cmake .. && make
# Output:
#   Current version: 1.0.0
#   Found 1 fix: commits
#   Bumping version: 1.0.0 → 1.0.1 (PATCH)
#   ✓ Updated version.txt
#   ✓ Updated CHANGELOG.md
#   ✓ Version bump committed!
```

### Manual Version Bump Preview

To see what version bump will happen **without** committing:

```bash
./scripts/bump-version.sh --no-commit
```

### Manual Version Bump

To manually trigger version bump:

```bash
./scripts/bump-version.sh
```

## Release Workflow

### Step 1: Create Release Commit(s)

Make your changes and commit with conventional format:

```bash
git commit -m "feat(main.cpp): new feature description"
git commit -m "fix(firmware): bug fix description"
```

### Step 2: Build (Auto-Version)

```bash
cd build && cmake .. && make
```

This automatically:
- Scans commits since last tag
- Increments version if needed
- Updates CHANGELOG.md
- Commits the version bump

### Step 3: Formal Release

Tag and push the release:

```bash
./scripts/release.sh
```

This:
- Creates annotated git tag (v1.0.1)
- Pushes commits to GitHub
- Pushes tag to GitHub
- Shows release summary

### Step 4: Publish (Optional)

Go to GitHub repository → Releases → Edit tag → Add release notes.

## Example Release Scenario

### Scenario: Release v1.0.1 with bug fixes

**1. Make your changes and commits:**

```bash
# Fix sensor communication issue
git commit -m "fix(firmware): correct sensor baud fallback sequence"

# Fix image export path
git commit -m "fix(main.cpp): organize fingerprint exports into subdirectories"

# Documentation update (no version impact)
git commit -m "docs(usage): update image file location references"
```

**2. Build (auto-bumps version 1.0.0 → 1.0.1):**

```bash
cd build && cmake .. && make

# Output:
#   Current version: 1.0.0
#   Latest tag: v1.0.0
#   Analyzing commits in range: v1.0.0..HEAD
#     ✓ Found 2 fix: commits
#   Version bump: 1.0.0 → 1.0.1 (PATCH)
#   ✓ Updated version.txt
#   ✓ Updated CHANGELOG.md
#   ✓ Version bump committed!
#   Commit: abc1234
```

**3. Release:**

```bash
./scripts/release.sh

# Output:
#   ════════════════════════════════════════
#     Fingerprint Lock System Release
#   ════════════════════════════════════════
#   
#   Version: 1.0.1
#   Tag: v1.0.1
#   
#   Release Notes:
#     - Fix sensor communication issue
#     - Fix image export paths
#   
#   Proceed with release? (y/N) y
#   
#   Creating release...
#   ✓ Created tag v1.0.1
#   ✓ Pushed commits
#   ✓ Pushed tag v1.0.1
#   
#   ════════════════════════════════════════
#     Release Complete!
#   ════════════════════════════════════════
```

**4. Verify on GitHub:**

Tag appears at: https://github.com/sas-bergson/fingerprint-arduino/releases

## CHANGELOG Format

The CHANGELOG.md follows [Keep a Changelog](https://keepachangelog.com/) format:

```markdown
## [1.0.1] - 2026-06-28

### Fixed
- Corrected sensor baud rate fallback sequence
- Organized fingerprint exports into subdirectories

### Changed
- Improved error messages for sensor communication

## [1.0.0] - 2026-06-28

### Added
- Complete fingerprint enrollment system
- Image capture and export
- [... etc ...]
```

## File Reference

| File                      | Purpose                                        |
| ------------------------- | ---------------------------------------------- |
| `version.txt`             | Single source of truth for version             |
| `CMakeLists.txt`          | Reads version.txt; defines auto-version target |
| `scripts/bump-version.sh` | Auto-detects and applies version bumps         |
| `scripts/release.sh`      | Formal release: tags, pushes, summarizes       |
| `CHANGELOG.md`            | User-facing release history                    |

## Best Practices

✅ **DO:**
- Use conventional commits (`feat:`, `fix:`, `docs:`, etc.)
- Run `make` before release to auto-bump version
- Use `./scripts/release.sh` for formal releases
- Document significant changes in commit messages
- Review CHANGELOG.md before releasing

❌ **DON'T:**
- Manually edit version.txt directly (use build system)
- Skip the build step (it auto-manages version)
- Mix unrelated changes in one commit (atomic commits)
- Commit with `--no-verify` to skip hooks

## Troubleshooting

### Version didn't bump when expected

**Problem:** Made `feat:` commits but version didn't bump.

**Solution:**
```bash
# Check git log for correct commit format
git log --oneline -10

# Manually run version script to debug
./scripts/bump-version.sh --no-commit

# Look for "found X feat: commits" in output
```

### Version file is out of sync

**Problem:** version.txt doesn't match expected version.

**Solution:**
```bash
# Check current version
cat version.txt

# Check latest git tag
git describe --tags

# If they're out of sync, rebuild to auto-sync
cd build && cmake .. && make
```

### Can't create release tag

**Problem:** Tag already exists or push fails.

**Solution:**
```bash
# Check existing tags
git tag -l

# If tag already exists, delete it first (careful!)
git tag -d v1.0.1
git push origin :refs/tags/v1.0.1

# Then try release again
./scripts/release.sh
```

## Further Reading

- [Semantic Versioning Spec](https://semver.org/)
- [Conventional Commits](https://www.conventionalcommits.org/)
- [Keep a Changelog](https://keepachangelog.com/)
- [Git Tagging](https://git-scm.com/book/en/v2/Git-Basics-Tagging)
