#!/bin/bash
#===============================================================================
# VICE Runner Script - runs C64 program and dumps memory via remote monitor
#===============================================================================
set -e

PRG_FILE="$(realpath "$1")"
OUTPUT_FILE="$(realpath -m "$2")"

[ -f "$PRG_FILE" ] || { echo "Error: $PRG_FILE not found"; exit 1; }
command -v x64sc &>/dev/null || { echo "Error: x64sc not found"; exit 1; }

cd "$(mktemp -d)"
trap 'rm -rf "$(pwd)"' EXIT

XVFB=""
command -v xvfb-run &>/dev/null && XVFB="xvfb-run -a"

echo "Running $PRG_FILE in VICE..."

# Use remote monitor on port 6510 - we can connect via netcat
MONITOR_PORT=6510

# Start VICE in background with remote monitor enabled
$XVFB x64sc -sounddev dummy -warp -autostartprgmode 1 \
    -remotemonitor -remotemonitoraddress 127.0.0.1:$MONITOR_PORT \
    "$PRG_FILE" >/dev/null 2>&1 &
VICE_PID=$!

# Wait for VICE to start and program to run (BRK will stop execution)
sleep 4

# Connect to remote monitor and send commands
{
    echo 'save "trace.bin" 0 3ff0 44e1'
    sleep 0.5
    echo 'quit'
} | nc -q 1 127.0.0.1 $MONITOR_PORT 2>/dev/null || true

# Wait for VICE to exit
wait $VICE_PID 2>/dev/null || true

if [ -f "trace.bin" ]; then
    cp trace.bin "$OUTPUT_FILE"
    echo "Trace saved: $OUTPUT_FILE ($(wc -c < "$OUTPUT_FILE") bytes)"
else
    echo "Error: trace.bin not created"
    exit 1
fi
