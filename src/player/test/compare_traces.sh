#!/bin/bash
#===============================================================================
# Trace Comparison Script
#===============================================================================
# Compares SID register traces from assembly and C implementations
#
# Usage: ./compare_traces.sh <reference_trace.txt> <c_trace.txt>
#
# Exit codes:
#   0 - Traces match (success)
#   1 - Traces differ (test failure)
#   2 - Error (missing files, etc.)
#===============================================================================

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check arguments
if [ $# -ne 2 ]; then
    echo "Usage: $0 <reference_trace.txt> <c_trace.txt>"
    exit 2
fi

REF_TRACE="$1"
C_TRACE="$2"

# Check if files exist
if [ ! -f "$REF_TRACE" ]; then
    echo -e "${RED}Error: Reference trace not found: $REF_TRACE${NC}"
    exit 2
fi

if [ ! -f "$C_TRACE" ]; then
    echo -e "${RED}Error: C trace not found: $C_TRACE${NC}"
    exit 2
fi

echo "Comparing traces..."
echo "  Reference: $REF_TRACE"
echo "  C version: $C_TRACE"
echo ""

# Quick check: are files identical?
if diff -q "$REF_TRACE" "$C_TRACE" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PASS: Traces are identical!${NC}"
    echo ""
    echo "SID register outputs match bit-for-bit."
    exit 0
fi

# Files differ - find first mismatch
echo -e "${YELLOW}Traces differ. Finding first mismatch...${NC}"
echo ""

# Use diff with context to show the mismatch
DIFF_OUTPUT=$(diff -u "$REF_TRACE" "$C_TRACE" | head -50)

if [ -z "$DIFF_OUTPUT" ]; then
    echo -e "${RED}✗ FAIL: Files differ but diff produced no output${NC}"
    exit 1
fi

# Parse the diff to find tick number
FIRST_DIFF_LINE=$(echo "$DIFF_OUTPUT" | grep -n "^[-+]TICK:" | head -1 | cut -d: -f1)

if [ -n "$FIRST_DIFF_LINE" ]; then
    # Extract tick number from the line
    TICK_LINE=$(echo "$DIFF_OUTPUT" | sed -n "${FIRST_DIFF_LINE}p")
    TICK_NUM=$(echo "$TICK_LINE" | grep -o "TICK:[0-9A-F]*" | head -1 | cut -d: -f2)

    echo -e "${RED}✗ FAIL: First mismatch at tick $TICK_NUM${NC}"
    echo ""
    echo "Diff context:"
    echo "$DIFF_OUTPUT" | head -20
    echo ""
    echo "To see full diff:"
    echo "  diff -u $REF_TRACE $C_TRACE"
else
    echo -e "${RED}✗ FAIL: Traces differ${NC}"
    echo ""
    echo "Diff output:"
    echo "$DIFF_OUTPUT" | head -20
fi

echo ""
echo "Trace comparison failed."
exit 1
