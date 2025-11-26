#!/bin/bash
#===============================================================================
# Trace Binary to Text Converter
#===============================================================================
# Converts binary trace dump from VICE to human-readable text format.
#
# Usage: ./trace_to_text.sh <trace.bin> <output.txt>
#
# Binary format (from run_vice.sh dump at $3FF0-$44E1):
#   $3FF0-$3FF3: Magic "TRC\0"
#   $3FF4-$3FF5: Number of ticks (16-bit little-endian)
#   $3FF6: Trace size per tick (25)
#   $3FF7-$3FFF: Reserved
#   $4000+: Trace data (ticks * 25 bytes)
#
# Output format:
#   TICK:00 D400:xx D401:xx ... D418:xx
#   TICK:01 D400:xx D401:xx ... D418:xx
#   ...
#===============================================================================

set -e

if [ $# -lt 2 ]; then
    echo "Usage: $0 <trace.bin> <output.txt>"
    exit 1
fi

INPUT="$1"
OUTPUT="$2"

if [ ! -f "$INPUT" ]; then
    echo "Error: Input file not found: $INPUT"
    exit 1
fi

# Check file size
# VICE save format: 2-byte load address + data
# Expected: 2 + 16 header + 1250 data = 1268 bytes minimum
FILE_SIZE=$(wc -c < "$INPUT")
if [ "$FILE_SIZE" -eq 0 ]; then
    echo "Note: Input file is empty (placeholder)"
    touch "$OUTPUT"
    exit 0
fi

if [ "$FILE_SIZE" -lt 1268 ]; then
    echo "Warning: File too small ($FILE_SIZE bytes), expected 1268+"
fi

# Convert entire file to hex at once (efficient)
HEX=$(xxd -p "$INPUT" | tr -d '\n')

# VICE save adds a 2-byte load address header, skip it (4 hex chars)
VICE_HEADER_OFFSET=4

# Extract header info (starts after VICE header)
# Our header at $3FF0: Magic "TRC\0", tick count, trace size
MAGIC_HEX="${HEX:$VICE_HEADER_OFFSET:8}"
TICK_LO="${HEX:$((VICE_HEADER_OFFSET + 8)):2}"
TICK_HI="${HEX:$((VICE_HEADER_OFFSET + 10)):2}"
TRACE_SIZE_HEX="${HEX:$((VICE_HEADER_OFFSET + 12)):2}"

# Convert hex to values (little-endian for tick count)
TICK_COUNT=$((16#${TICK_HI}${TICK_LO}))
TRACE_SIZE=$((16#${TRACE_SIZE_HEX}))

echo "Header: Magic=${MAGIC_HEX} Ticks=$TICK_COUNT TraceSize=$TRACE_SIZE"

if [ "$TRACE_SIZE" -ne 25 ]; then
    echo "Warning: Unexpected trace size: $TRACE_SIZE (expected 25)"
    TRACE_SIZE=25
fi

if [ "$TICK_COUNT" -eq 0 ] || [ "$TICK_COUNT" -gt 100 ]; then
    echo "Warning: Invalid tick count ($TICK_COUNT), using default 50"
    TICK_COUNT=50
fi

# Data starts after VICE header (2 bytes) + our header (16 bytes) = 18 bytes = 36 hex chars
DATA_START=$((VICE_HEADER_OFFSET + 32))

# Generate output efficiently
{
    for ((tick=0; tick<TICK_COUNT; tick++)); do
        printf "TICK:%02X " "$tick"
        BASE=$((DATA_START + tick * TRACE_SIZE * 2))

        for ((reg=0; reg<TRACE_SIZE; reg++)); do
            POS=$((BASE + reg * 2))
            BYTE="${HEX:$POS:2}"
            printf "D4%02X:%s " "$reg" "${BYTE^^}"
        done
        echo
    done
} > "$OUTPUT"

echo "Converted $TICK_COUNT ticks to: $OUTPUT"
