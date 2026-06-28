# Contributing to FingerPrint Lock System

Thank you for your interest in contributing! This document provides guidelines and instructions for contributing to the project.

## 🎯 Code of Conduct

- Be respectful and inclusive
- Assume good intentions
- Focus on constructive feedback
- Report harmful behavior to maintainers

## 🚀 Getting Started

### Prerequisites
- Git knowledge
- C++11 proficiency
- CMake 3.10+
- Arduino development environment
- Fingerprint sensor hardware (for testing)

### Setup Development Environment

```bash
# Clone repository
git clone https://github.com/sas-bergson/fingerprint-arduino.git
cd fingerprint-arduino

# Create feature branch
git checkout -b feature/your-feature-name

# Install dependencies
mkdir build && cd build
cmake ..
make
```

## 📝 Commit Message Format

Follow conventional commits for clear history:

```
<type>(<scope>): <subject>

<body>

<footer>
```

### Types
- **feat:** New feature
- **fix:** Bug fix
- **docs:** Documentation changes
- **style:** Code style (formatting, missing semicolons, etc.)
- **refactor:** Code refactoring without feature/fix changes
- **test:** Adding or updating tests
- **build:** Changes to build system or dependencies
- **ci:** CI/CD configuration changes

### Examples
```bash
# Feature
git commit -m "feat(protocol): add acknowledgment handshake to v2 protocol"

# Bug fix
git commit -m "fix(uart): resolve template upload data corruption on retry"

# Documentation
git commit -m "docs(readme): add troubleshooting section"
```

## 🌿 Branch Naming Convention

```
feature/<feature-name>      # New features
bugfix/<bug-name>          # Bug fixes
docs/<doc-name>            # Documentation
refactor/<component>       # Code refactoring
```

Examples:
```
feature/image-recognition
bugfix/uart-timeout-handling
docs/protocol-v2-draft
```

## 🔄 Pull Request Process

### Before Submitting
1. **Update your branch** with latest main:
   ```bash
   git fetch origin
   git rebase origin/main
   ```

2. **Build and test locally:**
   ```bash
   cd build
   make clean && make
   ctest  # if tests enabled
   ```

3. **Check code style** - follow existing conventions in the codebase

4. **Run linting** (if available in future)

### Pull Request Checklist
- [ ] Follows commit message format (conventional commits)
- [ ] Code builds without warnings
- [ ] No breaking changes (or documented in PR)
- [ ] Added tests for new features
- [ ] Updated CHANGELOG.md (Unreleased section)
- [ ] Updated documentation if needed
- [ ] Includes descriptive PR title and description

### PR Template
```markdown
## Description
Brief description of changes

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Breaking change
- [ ] Documentation update

## How to Test
Steps to verify the changes

## Related Issue
Closes #(issue number)

## Checklist
- [ ] My code follows style guidelines
- [ ] I've added tests for new features
- [ ] I've updated documentation
```

## 📋 Code Style Guidelines

### C++ Standards
- Use C++11 standard (minimum)
- Follow RAII principles
- Use meaningful variable names
- Keep functions focused and small
- Document complex logic with comments

### File Organization
```cpp
// Include guards (include/myheader.hpp)
#ifndef FINGERPRINT_MYHEADER_HPP
#define FINGERPRINT_MYHEADER_HPP

// Includes
#include <system>
#include "local.hpp"

// Declarations
class MyClass { };

#endif
```

### Naming Conventions
- **Classes/Types:** PascalCase (`SerialPort`, `FingerprintSensor`)
- **Functions/Methods:** camelCase (`readStatus()`, `enrollFingerprint()`)
- **Variables:** snake_case (`template_data`, `sensor_timeout`)
- **Constants:** UPPER_SNAKE_CASE (`MAX_RETRIES`, `UART_BAUDRATE`)

### Comment Style
```cpp
// Single line comment for brief notes

/*
 * Multi-line comment for detailed explanations
 * of complex logic or important notes
 */
```

## 🐛 Bug Reports

Submit bugs via GitHub Issues with:
- **Title:** Clear, descriptive bug title
- **Description:** What you expected vs. actual behavior
- **Steps:** How to reproduce the issue
- **Environment:** OS, Arduino board, compiler version
- **Logs:** Error messages or debug output

**Bug Report Template:**
```markdown
## Description
[Describe the bug]

## Steps to Reproduce
1. [First step]
2. [Second step]

## Expected Behavior
[What should happen]

## Actual Behavior
[What actually happened]

## Environment
- OS: [Linux/Windows/macOS]
- Arduino Board: [Uno/Nano/etc]
- CMake Version: [version]
```

## ✨ Feature Requests

Suggest enhancements via GitHub Issues:
- **Title:** Clear feature description
- **Use Case:** Why this feature is needed
- **Proposed Solution:** How it might work (optional)
- **Alternatives:** Other approaches (optional)

## 📚 Documentation Contributions

Documentation improvements are highly valued!

### Types of Documentation
- README updates
- API documentation (Doxygen comments)
- Protocol specifications
- Troubleshooting guides
- Development guides

### Doxygen Comments
```cpp
/**
 * @brief Read fingerprint template from sensor
 * @param index Template index (0-127)
 * @param data Output buffer for template data
 * @return Status code (0 for success)
 * @throws SerialException if communication fails
 */
int readTemplate(uint8_t index, uint8_t* data);
```

## 🔄 Review Process

### What Reviewers Look For
- Code correctness and logic
- Performance implications
- Security concerns
- Test coverage
- Documentation clarity
- Breaking changes
- Alignment with project goals

### Reviewer Expectations
- Reviews aim to be constructive and helpful
- Approval means code is ready to merge
- Changes may be requested before approval
- Discussion is encouraged

## 📦 Release Process

See [DEVELOPMENT.md](DEVELOPMENT.md) for semantic versioning and release procedures.

## 📞 Questions or Need Help?

- **Email:** [maintainer email]
- **Issues:** Open a GitHub issue with `question` label
- **Discussions:** Use GitHub Discussions for general questions

## 🙏 Recognition

Contributors are recognized in:
- CHANGELOG.md (for releases)
- GitHub contributors page
- Project README (if preferred)

---

**Thank you for contributing to FingerPrint Lock System!**
