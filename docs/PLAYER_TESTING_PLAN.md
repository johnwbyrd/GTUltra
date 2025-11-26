# Player C Implementation - Testing Infrastructure

## Overview

This document describes how to validate the Player C implementation against the reference assembly player. The goal is bit-perfect compatibility: given the same song data, both implementations must produce identical SID register writes.

## The Problem: SID Registers Are Write-Only

SID registers at $D400-$D418 are **write-only**. Reading them from the CPU returns undefined values (typically zeros in VICE). This means:

- We cannot read SID state from memory after `player_play()` returns
- Shadow registers in C code only capture what the C player writes, not what the assembly player writes
- Memory dumps of $D400-$D418 are useless

## The Solution: VICE `-sounddev dump`

VICE can log **every SID register write** to a file using the `-sounddev dump` option. This captures the actual writes as they happen, with cycle-accurate timing.

### Dump File Format

The dump file is **space-separated text**:

```
<clock_offset> <reg> <val>
```

Where:
- `clock_offset`: CPU cycles since the previous write (cumulative sum gives absolute cycle)
- `reg`: SID register number (0-24, corresponding to $D400-$D418)
- `val`: Value written (0-255)

Example:
```
0 24 15
100 0 72
5 1 28
12 4 33
```

This means:
- At cycle 0: write 15 to register 24 (volume = 15)
- At cycle 100: write 72 to register 0 (voice 1 freq_lo)
- At cycle 105: write 28 to register 1 (voice 1 freq_hi)
- At cycle 117: write 33 to register 4 (voice 1 control)

### Why This Works

1. **Captures actual writes** - not shadow registers, not memory reads
2. **Works for any player** - assembly or C, no code instrumentation needed
3. **Cycle-accurate timing** - can detect timing differences between implementations
4. **Per-write granularity** - see exact order of writes within a frame

## Implementation

### Running VICE with Sound Dump

```bash
xvfb-run -a x64sc \
    -sounddev dump \
    -soundarg sid_writes.log \
    -warp \
    -autostartprgmode 1 \
    -limitcycles 5000000 \
    program.prg
```

Options:
- `-sounddev dump`: Enable SID write logging
- `-soundarg sid_writes.log`: Output file path
- `-warp`: Maximum speed (no frame sync)
- `-autostartprgmode 1`: Auto-run the .prg file
- `-limitcycles 5000000`: Stop after ~5M cycles (~5 seconds of emulation)

### Test Harness Programs

Both assembly and C test harnesses are simple:

1. Initialize player with test song data
2. Call `player_play()` N times (e.g., 50 ticks)
3. Halt (BRK instruction)

VICE captures all SID writes automatically - no need to copy registers to memory.

**C Test Harness (`src/player/test/c_trace.c`):**
```c
int main(void) {
    static Player player;
    player_init(&player, &test_music_data, 0);

    for (uint8_t tick = 0; tick < 50; tick++) {
        player_play(&player, &test_music_data);
    }

    // Halt - VICE has already logged all SID writes
    for (;;) {
        __asm__ volatile ("brk");
    }
}
```

### Trace Comparison

#### Option A: Per-Frame Comparison

Convert cycle-accurate writes to per-frame snapshots:
- PAL frame = 19656 cycles
- Group writes by frame, take final value of each register
- Output: 25-byte state at end of each frame

Pros: Simpler comparison, smaller output
Cons: Loses intra-frame timing information

#### Option B: Per-Write Comparison

Compare raw write sequences directly:
- Same writes in same order = pass
- Different write = fail, report first difference

Pros: Catches timing bugs, exact comparison
Cons: More complex, larger output

**Recommendation:** Start with Option A (per-frame). If needed, add Option B for debugging.

### Parsing the Dump File

Simple bash/awk script to convert to per-frame format:

```bash
#!/bin/bash
# Parse VICE -sounddev dump output to per-frame SID state

CYCLES_PER_FRAME=19656  # PAL
declare -a regs
for i in {0..24}; do regs[$i]=0; done

frame=0
clock=0

while read offset reg val; do
    clock=$((clock + offset))
    frame_num=$((clock / CYCLES_PER_FRAME))

    # New frame? Output previous state
    while [ $frame -lt $frame_num ]; do
        printf "FRAME:%04X" $frame
        for i in {0..24}; do
            printf " D4%02X:%02X" $i ${regs[$i]}
        done
        echo
        ((frame++))
    done

    regs[$reg]=$val
done < "$1"
```

## CI Integration

### GitHub Actions Workflow

```yaml
- name: Install VICE emulator
  run: |
    sudo apt-get update
    sudo apt-get install -y --no-install-recommends vice xvfb unrar-free
    # Download and install VICE ROMs
    curl -L -o /tmp/vice-roms.rar "https://..."
    cd /tmp && unrar x -o+ vice-roms.rar
    sudo cp -r /tmp/VICE_ROMs/data/* /usr/share/vice/

- name: Run C player trace
  run: |
    xvfb-run -a x64sc -sounddev dump -soundarg c_trace.log \
        -warp -autostartprgmode 1 -limitcycles 5000000 \
        build/c_trace.prg

- name: Convert trace to text
  run: ./test/dump_to_frames.sh c_trace.log > c_trace.txt
```

## File Structure

```
src/player/
├── test/
│   ├── c_trace.c           # C test harness (just runs player)
│   ├── test_data.h         # Minimal test song data
│   ├── run_vice.sh         # Runs VICE with -sounddev dump
│   ├── dump_to_frames.sh   # Converts dump to per-frame format
│   └── compare_traces.sh   # Diffs two trace files
```

## Current Status

- CI infrastructure working (VICE + ROMs installed)
- C test harness compiles and runs
- Need to switch from memory dump to `-sounddev dump`
- Need to write dump parser script
- Assembly reference harness not yet implemented

## Next Steps

1. Update `run_vice.sh` to use `-sounddev dump` instead of remote monitor
2. Write `dump_to_frames.sh` to parse the dump format
3. Simplify `c_trace.c` (remove capture_sid_state, just run player)
4. Create assembly reference harness
5. Implement trace comparison

## References

- [VICE Manual - Sound Options](https://vice-emu.sourceforge.io/vice_2.html)
- [desidulate](https://github.com/anarkiwi/desidulate) - Python tools for parsing VICE SID dumps
