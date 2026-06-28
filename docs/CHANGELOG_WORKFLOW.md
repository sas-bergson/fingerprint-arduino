# Changelog Management Workflow

This document describes how to keep `CHANGELOG.md` in sync with code changes using automated tools and consistent commit conventions.

## Quick Start

### For Every Commit

1. **Use conventional commit format** in your commit messages:
   ```
   feat(main.cpp): add temperature sensor support
   fix(FingerPrint_Enroll.ino): prevent buffer overflow in packet reading
   docs(README): clarify protocol specification
   ```

2. **Git commit template will appear automatically** when you run `git commit` — it includes the changelog entry fields.

3. **Include changelog metadata** in your commit message (optional but recommended):
   ```
   feat(main.cpp): add temperature sensor support
   
   [CHANGELOG]
   Version: 0.10.0
   Section: Added
   Entry: - Temperature sensor support with calibration
   ```

### When Releasing a Version

1. **View recent commits as changelog entries:**
   ```bash
   ./scripts/changelog-helper.sh --generate
   ```

2. **Generate and insert new version section:**
   ```bash
   ./scripts/changelog-helper.sh --version 0.10.0 --date 2026-06-30
   ```

3. **Edit `CHANGELOG.md` manually** to:
   - Remove technical/non-user-facing commits
   - Consolidate related changes
   - Add context and details
   - Group logically by feature

4. **Commit the changelog:**
   ```bash
   git add CHANGELOG.md
   git commit -m "docs: release v0.10.0"
   ```

## Conventional Commit Format

Structure: `<type>(<scope>): <description>`

### Types (for CHANGELOG)

| Type       | Section        | Example                                                  |
| ---------- | -------------- | -------------------------------------------------------- |
| `feat`     | Added          | `feat(serialport): add CRC validation`                   |
| `fix`      | Fixed          | `fix(main.cpp): correct baud rate fallback logic`        |
| `docs`     | Documentation  | `docs(README): clarify protocol`                         |
| `refactor` | Changed        | `refactor(cmake): simplify build rules`                  |
| `perf`     | Changed        | `perf(FingerPrint_Enroll.ino): optimize image streaming` |
| `build`    | (usually skip) | Infrastructure changes                                   |
| `ci`       | (usually skip) | CI/CD changes                                            |
| `test`     | (usually skip) | Test-only changes                                        |
| `chore`    | (usually skip) | Dependency updates, config                               |

### Scopes

Common scopes for this project:
- `main.cpp` — Host application
- `FingerPrint_Enroll.ino` — Arduino firmware
- `serialport` — SerialPort library (cpp/hpp)
- `cmake` — Build system
- `docs` — Documentation files
- `doxygen` — API documentation config
- `scripts` — Utility scripts
- `protocol` — Protocol specification

## Detailed Workflow Example

### Step 1: Make code changes

```bash
# Edit files...
git add src/main.cpp
```

### Step 2: Commit with template

```bash
git commit
```

This opens your editor with the `.gitmessage` template pre-filled:

```
# Commit message format:
# <type>(<scope>): <subject>
# ...
# CHANGELOG ENTRY (if releasing):
# Version: (e.g., 0.10.0 or 1.0.1)
# Section: Added, Changed, Fixed, Removed, Deprecated, Security
# Entry: Brief description of user-facing change

# Fill in like this:
feat(main.cpp): add retry logic for failed template uploads

This implements exponential backoff for transient serial errors.

[CHANGELOG]
Version: 0.10.0
Section: Added
Entry: - Automatic retry logic for template upload failures with exponential backoff
```

### Step 3: When ready to release (collect multiple commits)

```bash
# See what commits look like as changelog entries
./scripts/changelog-helper.sh --generate

# This shows recent commits parsed as changelog sections
# Output:
# === RECENT COMMITS (as potential CHANGELOG entries) ===
#
# ### Added
# - **main.cpp**: Add retry logic for failed template uploads
# - **serialport**: Add timeout validation
#
# ### Fixed
# - **FingerPrint_Enroll.ino**: Prevent buffer overflow in packet reading
```

### Step 4: Generate new version section

```bash
./scripts/changelog-helper.sh --version 0.10.0 --date 2026-06-30
```

This:
1. Creates `[0.10.0] - 2026-06-30` section
2. Groups recent commits by type (Added, Fixed, Changed, etc.)
3. Inserts into `CHANGELOG.md` after `[Unreleased]` section
4. Leaves `[Unreleased]` → Planned section intact

### Step 5: Manual polish

Edit `CHANGELOG.md` to:
- Remove commits that aren't user-facing
- Add context: why, not just what
- Link to issue numbers if relevant
- Consolidate: "Add retry logic" + "Add timeout validation" → "Improve reliability"

Example:

```markdown
## [0.10.0] - 2026-06-30

### Added
- **Transfer reliability** - Retry logic with exponential backoff for failed template uploads
  - Automatic timeout detection and recovery
  - Detailed error codes for debugging

### Changed
- Serial communication now validates all timeout parameters before I/O
```

### Step 6: Commit and tag

```bash
git add CHANGELOG.md
git commit -m "docs: release v0.10.0"
git tag -a v0.10.0 -m "Release v0.10.0"
git push origin main --tags
```

## Git Commit Template Reference

The `.gitmessage` file is automatically shown when you run `git commit`. It includes:

1. **Commit message format** - conventional commits structure
2. **Type definitions** - feat, fix, docs, refactor, perf, test, build, ci, chore, style
3. **Scope examples** - main.cpp, FingerPrint_Enroll.ino, etc.
4. **Optional changelog entry fields** - version, section, description
5. **Example** - shows what a complete commit looks like

## Automation Rules

### What Gets Automatic Grouping

The `changelog-helper.sh` script automatically groups commits by conventional type:
- `feat(...)` → "### Added"
- `fix(...)` → "### Fixed"
- `docs(...)` → "### Documentation"
- `refactor(...)`/`perf(...)` → "### Changed"

### What Still Requires Manual Work

1. **Deciding what to include** - technical commits vs user-facing features
2. **Writing context** - why did we add this? what problem does it solve?
3. **Consolidation** - grouping related commits into cohesive features
4. **Cross-referencing** - linking to issues, PRs, or related commits

This is intentional — changelog entries are marketing and communication, not just automated logs.

## Best Practices

1. **Write commit subjects in imperative mood**
   - ✅ Good: `fix: prevent buffer overflow`
   - ❌ Avoid: `fixed buffer overflow`, `fixes buffer overflow`

2. **Be specific in scope**
   - ✅ Good: `feat(main.cpp): add retry logic`
   - ❌ Avoid: `feat: improvements`

3. **Keep commits atomic**
   - One feature/fix per commit makes changelog generation cleaner
   - Easier to revert if needed

4. **Reference issues in commit body** (not subject)
   ```
   feat(serialport): add CRC validation
   
   Fixes #42. Implements CRC16-CCITT for all packet types.
   ```

5. **Group changelog entries by user impact**
   - Not just technical categories
   - "Template upload reliability" > "Retry logic + timeout validation"

## Troubleshooting

### Git template not showing on `git commit`

Verify it's configured:
```bash
git config --local commit.template
# Should output: .gitmessage
```

### Changelog helper returns unexpected output

Check recent commits format:
```bash
git log --oneline -10
# Should show conventional format: feat(...), fix(...), etc.
```

### Manual changelog format validation

The CHANGELOG.md should follow [Keep a Changelog](https://keepachangelog.com/) format:
```markdown
## [VERSION] - YYYY-MM-DD

### Added
- User-facing feature 1
- User-facing feature 2

### Fixed
- Bug fix 1

### Changed
- Breaking change or significant modification
```
