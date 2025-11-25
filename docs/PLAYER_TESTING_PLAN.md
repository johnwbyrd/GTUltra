# Player C Implementation - Testing Infrastructure Plan

## Overview

This document outlines the strategy for validating the Player C implementation against the reference assembly implementation (player.s). The goal is bit-perfect compatibility: given the same song data, both implementations must produce identical SID register writes on every tick.

## Testing Strategy

### Core Concept

1. Load identical song data in both implementations
2. Run tick-by-tick in lockstep
3. After each tick, capture complete SID state (all 25 registers: $D400-$D418)
4. Compare traces - any divergence indicates a bug

**Note:** We only compare final SID register state per tick, not internal player variables. The internal state may differ between implementations as long as the SID output is identical.

### Why This Matters

- SID music is what the user hears - that's what must match
- Effects like portamento/vibrato modify frequency every tick
- Even small errors compound and become audible
- Real songs from examples/ folder provide comprehensive coverage

## Implementation Plan

### Phase 1: CI Infrastructure - Install VICE Emulator

**GitHub Actions Workflow Updates:**

```yaml
# Add to .github/workflows/player.yml
- name: Install VICE emulator
  run: |
    sudo apt-get update
    sudo apt-get install -y vice xvfb
```

**Headless Execution:**
- Use `xvfb-run` to provide virtual framebuffer (VICE needs X even in "headless" mode)
- Use `x64sc` (cycle-accurate C64 emulator) for accurate SID timing
- Example: `xvfb-run x64sc -console -sounddev dummy ...`

**Local Testing with gh act:**
- Same setup in Docker container
- VICE + xvfb installed in container

### Phase 2: Reference Trace Generator (Assembly Binary)

**Purpose:** Build a .prg that runs the assembly player and outputs SID state

**Components:**

1. **Reference Test Binary (Assembly)**
   - Location: `src/player/test/ref_trace.s`
   - Built with existing assembler toolchain
   - Loads song data (embedded or from fixed location)
   - Calls `player_init` then loops calling `player_play`
   - After each tick, copies $D400-$D418 (25 bytes) to output buffer
   - After N ticks, outputs trace data (to screen, serial, or file)
   - Output format: simple hex dump, one line per tick

2. **Trace Format**
   ```
   TICK:0000 D400:xx D401:xx D402:xx ... D418:xx
   TICK:0001 D400:xx D401:xx D402:xx ... D418:xx
   ...
   ```

### Phase 3: C Implementation Trace Generator (C Binary)

**Purpose:** Build a .prg that runs the C player and outputs SID state

**Components:**

1. **C Test Binary**
   - Location: `src/player/test/c_trace.c`
   - Built with llvm-mos, links against libplayer.a
   - Loads same song data as reference binary
   - Calls `player_init` and `player_play` in same loop structure
   - After each tick, copies $D400-$D418 (25 bytes) to output buffer
   - Outputs trace in identical format to reference
   - Same tick count as reference

2. **Output Method**
   - Both binaries output to screen (VICE can capture screen output)
   - Or: write to a fixed memory region, then use VICE monitor to dump

### Phase 4: VICE Harness Script

**Purpose:** Drive VICE to run both binaries and capture output

**Components:**

1. **Test Runner Script**
   - Location: `src/player/test/run_comparison.sh`
   - Runs reference binary in VICE, captures output
   - Runs C binary in VICE, captures output
   - Uses VICE command line options:
     - `-console` for text mode
     - `-sounddev dummy` to disable audio
     - `-warp` for maximum speed
     - `-limitcycles N` to run for exactly N cycles then exit
     - Or use monitor commands to control execution

2. **Trace Extraction**
   - Option A: Binary writes trace to screen, capture VICE output
   - Option B: Binary writes to memory, use VICE `-moncommands` to dump after run
   - Option C: Use VICE's built-in logging/tracing features

### Phase 5: Comparison Tool

**Purpose:** Diff the traces and report mismatches

**Components:**

1. **Trace Comparator**
   - Location: `src/player/test/compare_traces.sh` (simple diff)
   - Reads reference trace and C trace files
   - Compares line-by-line (each line = one tick)
   - Reports first divergence with context:
     - Which tick number
     - Which register(s) differ
     - Expected vs actual values
   - Exit code 0 = match, 1 = mismatch

### Phase 6: Test Suite

**Test Songs:**

Use real songs from `examples/` folder - they provide comprehensive coverage of all features without needing synthetic test cases:
- `examples/Jammer/*.sng`
- `examples/JasonPage/*.sng`
- `examples/Linus/*.sng`
- `examples/LMan/*.sng`
- `examples/Mibri/*.sng`
- `examples/Shogoon/*.sng`

**Test Parameters:**
- Run each song for 1000-3000 ticks (20-60 seconds at 50Hz)
- This covers initialization, note changes, effects, table execution

### Phase 7: CI Integration

**Workflow:**

```yaml
name: Player Reference Tests

on: [push, pull_request]

jobs:
  reference-test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Install VICE
        run: sudo apt-get install -y vice

      - name: Install LLVM-MOS
        # ... existing llvm-mos setup ...

      - name: Build reference harness
        run: make -C src/player ref-trace

      - name: Build C harness
        run: make -C src/player c-trace

      - name: Generate reference traces
        run: |
          for song in src/player/test/*.sng; do
            x64sc -console -moncommands capture_trace.mon "$song"
          done

      - name: Generate C traces
        run: |
          for song in src/player/test/*.sng; do
            x64sc -console ./build/c_trace.prg "$song"
          done

      - name: Compare traces
        run: ./src/player/test/compare_traces.sh
```

## File Structure

```
src/player/
├── test/
│   ├── ref_trace.s          # Assembly reference harness
│   ├── c_trace.c            # C implementation harness
│   ├── run_comparison.sh    # Main test runner script
│   ├── capture_trace.mon    # VICE monitor script
│   ├── compare_traces.sh    # Comparison script
│   └── traces/              # Generated trace files (gitignored)
│       ├── ref/
│       └── c/

examples/                    # Real test songs (already exist)
├── Jammer/*.sng
├── JasonPage/*.sng
├── Linus/*.sng
├── LMan/*.sng
├── Mibri/*.sng
└── Shogoon/*.sng
```

## Open Questions

1. **VICE headless mode:** Need to verify `x64sc -console` works in GitHub Actions Ubuntu runner with xvfb.

2. **Song loading mechanism:** How do we load .sng files into the test binaries? Options:
   - Embed song data at compile time
   - Load from disk at runtime (VICE supports file I/O)
   - Use VICE monitor to inject data into memory

3. **Tick count per song:** Need to balance coverage vs CI time. 1000-3000 ticks per song seems reasonable.

## Implementation Actions

1. **Update GitHub Actions workflow** to install VICE + xvfb

2. **Create reference trace harness** (`src/player/test/ref_trace.s`)
   - Assembly program that runs player.s and dumps SID state

3. **Create C trace harness** (`src/player/test/c_trace.c`)
   - C program that runs player.c and dumps SID state

4. **Create VICE runner script** (`src/player/test/run_comparison.sh`)
   - Drives VICE to run both binaries and capture output

5. **Create comparison script** (`src/player/test/compare_traces.sh`)
   - Diffs traces and reports first divergence

6. **Verify locally with `gh act`**

7. **Integrate into CI pipeline**

## Success Criteria

- All test songs produce identical traces between assembly and C implementations
- CI runs in < 5 minutes
- Clear error messages when traces diverge
- Easy to add new test cases
