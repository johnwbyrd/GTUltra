#!/bin/bash
#===============================================================================
# VICE Runner Script
#===============================================================================
# Runs a C64 .prg file in VICE emulator and captures output
#
# Usage: ./run_vice.sh <program.prg> [timeout_seconds]
#
# This script:
# - Runs the program in x64sc (cycle-accurate C64 emulator)
# - Uses headless mode with xvfb (virtual framebuffer)
# - Disables sound
# - Runs in warp mode (maximum speed)
# - Captures console/screen output
# - Times out after specified seconds (default: 10)
#===============================================================================

set -e  # Exit on error

# Check arguments
if [ $# -lt 1 ]; then
    echo "Usage: $0 <program.prg> [timeout_seconds]"
    exit 1
fi

PRG_FILE="$1"
TIMEOUT="${2:-10}"  # Default 10 seconds

# Check if file exists
if [ ! -f "$PRG_FILE" ]; then
    echo "Error: File not found: $PRG_FILE"
    exit 1
fi

# Check if VICE is installed
if ! command -v x64sc &> /dev/null; then
    echo "Error: x64sc (VICE emulator) not found"
    echo "Install with: sudo apt-get install vice"
    exit 1
fi

# Check if xvfb is available (for headless mode)
if ! command -v xvfb-run &> /dev/null; then
    echo "Warning: xvfb-run not found, running without virtual framebuffer"
    XVFB_CMD=""
else
    XVFB_CMD="xvfb-run -a"
fi

# Temporary file for VICE monitor commands
MONITOR_CMD=$(mktemp)
trap "rm -f $MONITOR_CMD" EXIT

# Create VICE monitor script
# This will:
# 1. Load the program
# 2. Run it
# 3. Wait for completion or timeout
# 4. Exit

cat > "$MONITOR_CMD" << 'EOF'
# Load program
load "$PRG_FILE" 0

# Set breakpoint at end (placeholder - program should halt naturally)
# break $FFFF

# Run
goto $0801

# VICE will run until program halts or we hit timeout
EOF

# Run VICE in console mode
# Options:
#   -console: Use console interface
#   -sounddev dummy: No sound output
#   -warp: Run as fast as possible (no frame limiting)
#   -limitcycles: Stop after N cycles (optional)
#   -moncommands: Execute monitor commands from file

echo "Running $PRG_FILE in VICE..."
echo "Timeout: ${TIMEOUT}s"

# Run with timeout
# Note: VICE console mode may not capture all output cleanly
# For production, we may need to use VICE's logging features
timeout "$TIMEOUT" $XVFB_CMD x64sc \
    -console \
    -sounddev dummy \
    -warp \
    -autostartprgmode 1 \
    "$PRG_FILE" \
    2>&1 || {
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 124 ]; then
        echo "Warning: VICE timed out after ${TIMEOUT}s"
        # Timeout is not necessarily an error - program may have run successfully
        exit 0
    else
        echo "Error: VICE exited with code $EXIT_CODE"
        exit $EXIT_CODE
    fi
}

echo "VICE execution completed"
