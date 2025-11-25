# CI Testing Guide

This document explains how to test the GitHub Actions CI workflow locally before pushing.

## Using `act` to Test GitHub Actions Locally

[nektos/act](https://github.com/nektos/act) is a tool that allows you to run GitHub Actions workflows locally using Docker.

### Installation

**Linux/macOS (using curl):**
```bash
curl https://raw.githubusercontent.com/nektos/act/master/install.sh | sudo bash
```

**macOS (using Homebrew):**
```bash
brew install act
```

**Linux (using package managers):**
```bash
# Arch Linux
sudo pacman -S act

# Other distributions - download from releases:
# https://github.com/nektos/act/releases
```

**Manual installation:**
```bash
# Download latest release for your platform
curl -L -o act.tar.gz https://github.com/nektos/act/releases/latest/download/act_Linux_x86_64.tar.gz
tar xzf act.tar.gz
sudo mv act /usr/local/bin/
```

### Prerequisites

- Docker must be installed and running
- Sufficient disk space for Docker images (~2-5GB)

### Running the CI Workflow Locally

**Test the full workflow:**
```bash
cd /path/to/GTUltra
act -W .github/workflows/player-ci.yml
```

**Test a specific job:**
```bash
# Test only the build-and-test job
act -W .github/workflows/player-ci.yml -j build-and-test

# Test only the code-quality job
act -W .github/workflows/player-ci.yml -j code-quality
```

**Dry run (list jobs without executing):**
```bash
act -W .github/workflows/player-ci.yml -l
```

**Verbose output for debugging:**
```bash
act -W .github/workflows/player-ci.yml -v
```

### Expected Output

When running the workflow, you should see:

1. **Setup phase:**
   - Checkout repository
   - Cache restoration (or download if first run)
   - LLVM-MOS SDK installation

2. **Build phase:**
   - Library compilation
   - Test compilation
   - Verification checks

3. **Success indicators:**
   ```
   ✓ Library built successfully
   ✓ Tests compiled successfully
   ✓ All checks passed!
   ```

### Common Issues

**Issue: Docker not running**
```
Error: Cannot connect to the Docker daemon
```
**Solution:** Start Docker:
```bash
sudo systemctl start docker  # Linux
# or
open -a Docker  # macOS
```

**Issue: Permission denied**
```
Error: permission denied while trying to connect to Docker
```
**Solution:** Add your user to the docker group:
```bash
sudo usermod -aG docker $USER
# Then log out and back in
```

**Issue: Act uses too much disk space**
**Solution:** Use a smaller Docker image:
```bash
act -W .github/workflows/player-ci.yml -P ubuntu-latest=catthehacker/ubuntu:act-latest
```

### Workflow File Location

The CI workflow is defined in:
```
.github/workflows/player-ci.yml
```

### What the CI Tests

The workflow performs the following checks:

1. **Build validation:**
   - Compiles all source files
   - Creates `libplayer.a` library
   - Builds test executables

2. **Code quality:**
   - Checks for tabs vs spaces
   - Detects trailing whitespace
   - Verifies file permissions
   - Generates code statistics

3. **Artifacts:**
   - Uploads compiled library and tests
   - Available for 7 days after build

### Manual Testing Without `act`

If you don't want to use `act`, you can test the build manually:

```bash
# Install LLVM-MOS SDK (one-time setup)
curl -L -o /tmp/llvm-mos-linux.tar.xz \
  https://github.com/llvm-mos/llvm-mos-sdk/releases/download/v22.3.2/llvm-mos-linux.tar.xz
sudo mkdir -p /opt/llvm-mos
sudo tar -xf /tmp/llvm-mos-linux.tar.xz -C /opt/llvm-mos --strip-components=1
export PATH="/opt/llvm-mos/bin:$PATH"

# Verify installation
mos-c64-clang --version

# Build and test
cd src/player
make clean
make check
```

### CI Triggers

The workflow runs automatically on:

- **Push events:**
  - To `main` or `master` branches
  - To any `claude/**` branches
  - Only when files in `src/player/` change

- **Pull request events:**
  - Targeting `main` or `master` branches
  - Only when files in `src/player/` change

### Viewing CI Results on GitHub

After pushing, view the CI results:

1. Go to the repository on GitHub
2. Click the "Actions" tab
3. Select the "Player C Implementation CI" workflow
4. View logs, artifacts, and summaries

### Debugging Failed Builds

If the CI fails:

1. **Check the logs:**
   - Click on the failed job in GitHub Actions
   - Expand each step to see detailed output

2. **Test locally with act:**
   ```bash
   act -W .github/workflows/player-ci.yml -v
   ```

3. **Test manually:**
   ```bash
   cd src/player
   make clean
   make check
   ```

4. **Common fixes:**
   - Ensure all source files are committed
   - Check for syntax errors in C code
   - Verify header file paths in `#include` statements
   - Run `make clean` before rebuilding

### Performance Tips

**Speed up act runs:**
```bash
# Use cached Docker images
act -W .github/workflows/player-ci.yml --pull=false

# Reuse previous containers
act -W .github/workflows/player-ci.yml --reuse
```

**Speed up GitHub Actions:**
- The workflow uses caching for LLVM-MOS SDK
- Subsequent runs are much faster (~30 seconds vs 2 minutes)
- Cache is invalidated only when SDK version changes

## Integration with Development Workflow

### Recommended workflow:

1. **Make changes** to player source files
2. **Test locally:**
   ```bash
   cd src/player
   make check
   ```
3. **Test with act (optional):**
   ```bash
   act -W .github/workflows/player-ci.yml -j build-and-test
   ```
4. **Commit and push:**
   ```bash
   git add .
   git commit -m "Your commit message"
   git push
   ```
5. **Verify CI passes** on GitHub

### Pre-commit Hook (Optional)

Create `.git/hooks/pre-commit` to run checks automatically:

```bash
#!/bin/bash
echo "Running Player build check..."
cd src/player
if make check; then
    echo "✓ Build check passed!"
else
    echo "✗ Build check failed. Fix errors before committing."
    exit 1
fi
```

Make it executable:
```bash
chmod +x .git/hooks/pre-commit
```

## Additional Resources

- [nektos/act GitHub Repository](https://github.com/nektos/act)
- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [LLVM-MOS SDK Documentation](https://llvm-mos.org/wiki/)
- [Player Implementation Plan](docs/PLAYER_IMPLEMENTATION_PLAN.md)

---

**Questions or issues?** Open an issue on the GTUltra repository.
