#!/bin/bash
# Helper script to generate CHANGELOG.md entries from recent commits

set -e

CHANGELOG="CHANGELOG.md"
REPO_ROOT="$(git rev-parse --show-toplevel)"

print_usage() {
    cat <<EOF
Usage: $0 [options]

Options:
  --version VERSION     Target version (e.g., 0.10.0, 1.1.0)
  --since COMMIT        Show commits since COMMIT (default: last 5 commits)
  --generate            Generate template from recent commits (doesn't modify CHANGELOG.md)
  --date DATE           Release date in YYYY-MM-DD format (default: today)
  --help                Show this message

Examples:
  # View recent commits and generate template
  $0 --generate

  # Add new version section to CHANGELOG.md
  $0 --version 0.10.0 --date 2026-06-30

  # Show commits since specific commit
  $0 --generate --since abc1234

EOF
}

# Default values
GENERATE_ONLY=false
VERSION=""
SINCE=""
DATE=$(date +%Y-%m-%d)

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --version)
            VERSION="$2"
            shift 2
            ;;
        --since)
            SINCE="$2"
            shift 2
            ;;
        --generate)
            GENERATE_ONLY=true
            shift
            ;;
        --date)
            DATE="$2"
            shift 2
            ;;
        --help)
            print_usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            print_usage
            exit 1
            ;;
    esac
done

# Function to parse commits and group by type
generate_changelog_entries() {
    local commit_range="$1"
    
    echo "Parsing commits: $commit_range"
    echo ""
    
    # Arrays for different types
    declare -A features
    declare -A fixes
    declare -A changes
    declare -A docs
    declare -A other
    
    # Get commits
    local commits
    if [ -z "$commit_range" ]; then
        commits=$(git log --oneline -20)
    else
        commits=$(git log --oneline "$commit_range")
    fi
    
    while IFS= read -r line; do
        commit_hash=$(echo "$line" | awk '{print $1}')
        message=$(echo "$line" | cut -d' ' -f2-)
        
        # Parse conventional commit format: type(scope): description
        if [[ $message =~ ^([a-z]+)\(([^)]+)\):\s*(.+)$ ]]; then
            type="${BASH_REMATCH[1]}"
            scope="${BASH_REMATCH[2]}"
            desc="${BASH_REMATCH[3]}"
            
            # Capitalize first letter of description if not already
            desc="$(tr '[:lower:]' '[:upper:]' <<< ${desc:0:1})${desc:1}"
            
            case "$type" in
                feat)
                    echo "### Added"
                    echo "- **${scope}**: ${desc}"
                    ;;
                fix)
                    echo "### Fixed"
                    echo "- **${scope}**: ${desc}"
                    ;;
                docs)
                    echo "### Documentation"
                    echo "- **${scope}**: ${desc}"
                    ;;
                refactor|perf)
                    echo "### Changed"
                    echo "- **${scope}**: ${desc}"
                    ;;
            esac
        else
            # Fallback for non-conventional commits
            echo "- ${message} (${commit_hash})"
        fi
    done <<< "$commits"
}

# Function to create new version section
create_version_section() {
    local version="$1"
    local date="$2"
    
    cat <<EOF

## [$version] - $date

EOF
    
    # Generate entries from recent commits
    generate_changelog_entries "HEAD~20..HEAD"
}

# Main logic
if [ "$GENERATE_ONLY" = true ]; then
    # Just show what commits look like as changelog entries
    echo "=== RECENT COMMITS (as potential CHANGELOG entries) ==="
    echo ""
    generate_changelog_entries "$SINCE"
    echo ""
    echo "=== HOW TO USE ==="
    echo "1. Review entries above"
    echo "2. Run: $0 --version X.Y.Z --date $(date +%Y-%m-%d)"
    echo "3. Edit CHANGELOG.md to remove non-user-facing commits"
    
elif [ -n "$VERSION" ]; then
    # Create new version section
    if [ ! -f "$REPO_ROOT/$CHANGELOG" ]; then
        echo "Error: $CHANGELOG not found"
        exit 1
    fi
    
    echo "Creating new version section: [$VERSION] - $DATE"
    
    # Generate section content
    new_section=$(create_version_section "$VERSION" "$DATE")
    
    # Insert after [Unreleased] section (after the Planned subsection)
    # Find line with "## [Unreleased]" and insert after next blank section
    if grep -q "## \[Unreleased\]" "$REPO_ROOT/$CHANGELOG"; then
        # Create temp file with new section inserted
        awk -v section="$new_section" '
            /^## \[Unreleased\]/ {print; section_found=1; next}
            section_found && /^### Planned/ {print; skip_plan=1; next}
            section_found && skip_plan && /^$/ && !inserted {print section; inserted=1}
            {print}
        ' "$REPO_ROOT/$CHANGELOG" > "$REPO_ROOT/$CHANGELOG.tmp"
        
        mv "$REPO_ROOT/$CHANGELOG.tmp" "$REPO_ROOT/$CHANGELOG"
        echo "✓ Updated $CHANGELOG"
        echo ""
        echo "Review and edit $CHANGELOG manually to:"
        echo "  1. Remove non-user-facing technical commits"
        echo "  2. Consolidate related changes"
        echo "  3. Add missing context where needed"
        echo ""
        echo "Then run: git add CHANGELOG.md && git commit -m 'docs: release $VERSION'"
    else
        echo "Error: Could not find [Unreleased] section in $CHANGELOG"
        exit 1
    fi
else
    print_usage
    exit 1
fi
