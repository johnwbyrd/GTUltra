# Player C Implementation - Current Status

**Last Updated:** 2025-11-26
**Branch:** `feature/rewrite-c`
**Phase:** Between Phase 1 (Foundation) and Phase 2 (Sequencer) per PLAYER_IMPLEMENTATION_PLAN.md

---

## Executive Summary

The Player C implementation has all modules present and compiling. The sequencer and pattern decoding work. Tables (wave/pulse/filter) have basic functionality but are missing features. **Continuous effects are entirely stubbed** and **note initialization is incomplete**. Music will not play correctly - notes will sound but without pitch modulation, proper envelope triggering, or many effect commands.

---

## Module-by-Module Status

### player.c

| Function | Status | Notes |
|----------|--------|-------|
| `player_init()` | ✅ Complete | Marks song for deferred initialization |
| `player_play()` | ✅ Complete | Main loop: init check → filter → channels |
| `player_set_master_volume()` | ✅ Complete | Clamps to 0-15 |
| `complete_initialization()` | ✅ Complete | Resets channels, filter, SID, funk tempo |
| `init_channel()` | ✅ Complete | Sets tempo, counter, instrument, gate, waveform |
| `execute_channel()` | ⚠️ Incomplete | See note initialization gap below |
| `update_tick_counter()` | ✅ Complete | Handles funk tempo alternation |
| `write_sid_registers()` | ✅ Complete | Writes freq, pulse, control, ADSR |

**Gap - Note Initialization (lines 224-230):**
```c
// Check if we have a new note to initialize
if (ch->new_note != 0) {
    // Initialize the new note (load instrument parameters, etc.)
    // This is implemented in the next phase
    // For now, just clear the new_note flag
    ch->new_note = 0;  // <-- STUB: Should load instrument params
}
```

**What's missing:** When a new note is triggered, the code should:
1. Load instrument parameters from `music->instr_*` tables:
   - `attack_decay` from `instr_ad[instr-1]`
   - `sustain_release` from `instr_sr[instr-1]`
   - `wave_ptr` from `instr_waveptr[instr-1]`
   - `pulse_ptr` from `instr_pulseptr[instr-1]`
   - Filter pointer from `instr_filterptr[instr-1]`
   - `vibrato_delay` from `instr_vibdelay[instr-1]`
   - `effect_param` (vibrato) from `instr_vibparam[instr-1]`
   - `gate_timer` from `instr_gatetimer[instr-1]`
2. Load first-frame waveform from `instr_firstwave[instr-1]`
3. Set gate to 0xFF (on)
4. Handle hard restart (briefly set AD=0, SR=0 for instruments that need it)
5. Store `last_note` for calculated speed effects

---

### sequencer.c

| Function | Status | Notes |
|----------|--------|-------|
| `sequencer_fetch_pattern()` | ✅ Complete | Handles LOOP, TRANSPOSE, REPEAT |
| `sequencer_fetch_note()` | ✅ Complete | Decodes all pattern markers |
| `read_pattern_pointer()` | ✅ Complete | Combines lo/hi bytes |
| `read_frequency()` | ✅ Complete | Combines lo/hi bytes |

**Pattern markers handled:**
- ✅ `0x00-0x3F` - Instrument change
- ✅ `0x40-0x4F` - Effect + note
- ✅ `0x50-0x5F` - Effect only
- ✅ `0x60-0xBC` - Note value (with transpose)
- ✅ `0xBD` - Rest
- ✅ `0xBE` - Key off
- ✅ `0xBF` - Key on
- ✅ `0xC0-0xFE` - Packed rest
- ✅ `0x00` (when pattern_ptr != 0) - End of pattern

**Order list commands handled:**
- ✅ `0xFF` - Loop to position
- ✅ `0xE0-0xFE` - Transpose (signed offset)
- ✅ `0xD0-0xDF` - Repeat N times

---

### effects.c

#### Tick 0 Effects (execute once when note starts)

| Effect | # | Status | Notes |
|--------|---|--------|-------|
| FX_ARPEGGIO | 0 | ✅ Complete | Stores effect/param for continuous |
| FX_PORTAUP | 1 | ✅ Complete | Resets vibrato, stores effect/param |
| FX_PORTADOWN | 2 | ✅ Complete | Resets vibrato, stores effect/param |
| FX_TONEPORTA | 3 | ✅ Complete | Stores effect/param |
| FX_VIBRATO | 4 | ✅ Complete | Stores effect/param |
| FX_SETAD | 5 | ✅ Complete | Sets `attack_decay` |
| FX_SETSR | 6 | ✅ Complete | Sets `sustain_release` |
| FX_SETWAVE | 7 | ✅ Complete | Sets `waveform` |
| FX_SETWAVEPTR | 8 | ✅ Complete | Sets `wave_ptr`, resets timer |
| FX_SETPULSEPTR | 9 | ✅ Complete | Sets `pulse_ptr`, resets timer |
| FX_SETFILTPTR | A | ❌ Stub | `// TODO: Implement filter table pointer setting` |
| FX_SETFILTCTRL | B | ❌ Stub | `// TODO: Implement filter control setting` |
| FX_SETFILTCUTOFF | C | ❌ Stub | `// TODO: Implement filter cutoff setting` |
| FX_SETMASTERVOL | D | ⚠️ Partial | Checks param < 16 but doesn't write to player |
| FX_SETFUNKTEMPO | E | ❌ Stub | `// TODO: Implement funk tempo setting` |
| FX_SETTEMPO | F | ⚠️ Partial | Per-channel works, global (param < 0x80) is stub |

#### Continuous Effects (execute every tick) - ALL STUBBED

| Effect | # | Status | Code Location |
|--------|---|--------|---------------|
| FX_ARPEGGIO | 0 | ❌ Stub | Line 171: `// TODO: Implement instrument vibrato` |
| FX_PORTAUP | 1 | ❌ Stub | Line 176: `// TODO: Implement portamento up with speed calculation` |
| FX_PORTADOWN | 2 | ❌ Stub | Line 180: `// TODO: Implement portamento down with speed calculation` |
| FX_TONEPORTA | 3 | ❌ Stub | Line 184: `// TODO: Implement tone portamento` |
| FX_VIBRATO | 4 | ❌ Stub | Line 188: `// TODO: Implement vibrato` |

**What's needed for continuous effects:**

1. **Speed calculation function** - Lookup from `speed_left_table` / `speed_right_table`:
   ```c
   // If speed_left_table[param-1] >= 0x80: calculated speed (note-relative)
   // Else: normal speed (direct values)
   ```

2. **FX_PORTAUP** - Add speed to frequency each tick:
   ```c
   ch->frequency += speed;
   ```

3. **FX_PORTADOWN** - Subtract speed from frequency each tick:
   ```c
   ch->frequency -= speed;
   ```

4. **FX_TONEPORTA** - Slide toward target note:
   ```c
   // Calculate distance to target frequency
   // Add or subtract speed to move toward target
   // Snap to exact target when reached
   ```

5. **FX_VIBRATO** - Oscillate around base frequency:
   ```c
   // Use vibrato_phase to track position in oscillation
   // Alternate adding/subtracting speed
   // Reset phase when vibrato_phase crosses threshold
   ```

6. **FX_ARPEGGIO** - Instrument vibrato with delay:
   ```c
   // Check vibrato_delay, decrement if > 0
   // When delay expires, execute vibrato logic
   ```

---

### wavetable.c

| Feature | Status | Notes |
|---------|--------|-------|
| Delay (0-15) | ✅ Complete | Pauses for N frames |
| Waveform change | ✅ Complete | Sets `ch->waveform` |
| Note offset (relative) | ✅ Complete | Adds to current note |
| Note offset (absolute) | ✅ Complete | Sets note directly |
| Loop command (0xFF) | ✅ Complete | Jumps to target position |
| Wavetable commands (0xE0-0xEF) | ❌ Stub | Line 58: `// TODO: Implement wavetable commands` |

**Wavetable commands ($E0-$EF) not implemented:**
These are special commands that can trigger effects like:
- Setting ADSR
- Gate control
- Filter control
- etc.

Most songs don't use these advanced features, so this is low priority.

---

### pulsetable.c

| Feature | Status | Notes |
|---------|--------|-------|
| Modulation (1-127) | ✅ Complete | Adds speed to pulse width |
| Set command (128+) | ✅ Complete | Sets pulse width directly |
| Loop command (255) | ✅ Complete | Jumps to target position |
| 12-bit clamping | ✅ Complete | Clamps to 0-4095 |

---

### filtertable.c

| Feature | Status | Notes |
|---------|--------|-------|
| Cutoff-only (0) | ✅ Complete | Sets cutoff from speed table |
| Modulation (1-127) | ✅ Complete | Adds speed to cutoff |
| Set parameters (128+) | ✅ Complete | Sets type and control |
| Loop command (255) | ✅ Complete | Jumps to target position |
| SID writes | ✅ Complete | Writes cutoff, control, mode/volume |

**Note:** Master volume is hardcoded to 0x0F (line 44, 122). Should use `player->master_volume`.

---

### soundfx.c

| Function | Status | Notes |
|----------|--------|-------|
| `player_play_sfx()` | ✅ Complete | Priority check, starts SFX |
| `execute_soundfx()` | ✅ Complete | Hard restart, ADSR, notes, waveforms |

**SFX execution phases:**
- ✅ Frame 1: Hard restart (AD=0, SR=0)
- ✅ Frame 2: Load ADSR, pulse, set test bit
- ✅ Frame 3+: Read notes and waveforms
- ✅ End detection: Returns to normal music

---

### Header Files

| File | Status | Contents |
|------|--------|----------|
| `player.h` | ✅ Complete | Public API (init, play, sfx, volume) |
| `player_types.h` | ✅ Complete | All types, enums, constants |
| `player_internal.h` | ✅ Complete | Internal functions for testing |
| `sid.h` | ✅ Complete | SID struct, constants, inline accessors |

---

## Test Infrastructure Status

| Component | Status | Location |
|-----------|--------|----------|
| Unit test framework | ✅ Present | `test/test_sequencer.c` |
| Test data | ✅ Present | `test/test_data.h` |
| C trace harness | ✅ Present | `test/c_trace.c` |
| Assembly trace harness | ⚠️ Placeholder | `test/ref_trace.s` (needs player.s integration) |
| VICE runner script | ✅ Present | `test/run_vice.sh` |
| Trace comparison script | ✅ Present | `test/compare_traces.sh` |
| CI workflow | ✅ Working | `.github/workflows/player-ci.yml` |

---

## Build Status

```bash
# All targets build successfully:
make all        # ✅ Builds libplayer.a
make debug      # ✅ Builds with debug flags
make build-tests # ✅ Compiles test binaries
make check      # ✅ Full compilation check
```

**CI Status:** All three jobs pass (build-and-test, trace-test, code-quality)

---

## Impact on Playback

### What Will Work
- Notes play at correct pitch
- Order list sequencing (patterns play in order)
- Transpose commands
- Repeat commands
- Pattern data decoding
- Wavetable waveform/pitch changes
- Pulse width modulation
- Filter sweeps
- Sound effects

### What Won't Work
- **Portamento up/down** - Frequency stays constant
- **Vibrato** - No pitch oscillation
- **Tone portamento** - No slide to target note
- **Instrument vibrato/arpeggio** - No automatic pitch variation
- **Proper envelope triggering** - Notes won't have correct ADSR on each trigger
- **Hard restart** - Percussive sounds may not trigger properly
- **Some filter effects** - FX commands for filter don't work

### Audible Result
Music will sound "flat" and "mechanical":
- No pitch expression (slides, vibrato)
- Notes may not have proper attack on each trigger
- Drum sounds may be weak (no hard restart)
- Filter sweeps from patterns work, but FX-triggered ones don't

---

## Priority Order for Completion

### High Priority (Required for Basic Playback)

1. **Note initialization in player.c** (~50 lines)
   - Load instrument parameters on new note
   - Handle first-frame waveform
   - Set gate properly
   - Hard restart logic

2. **Continuous effects in effects.c** (~100 lines)
   - Speed calculation function
   - FX_PORTAUP / FX_PORTADOWN
   - FX_VIBRATO
   - FX_TONEPORTA
   - FX_ARPEGGIO (instrument vibrato)

### Medium Priority (Needed for Full Compatibility)

3. **Remaining tick-0 effects** (~30 lines)
   - FX_SETFILTPTR
   - FX_SETFILTCTRL
   - FX_SETFILTCUTOFF
   - FX_SETMASTERVOL (connect to player)
   - FX_SETFUNKTEMPO
   - FX_SETTEMPO (global mode)

4. **Master volume connection** (~5 lines)
   - Pass `player->master_volume` to filtertable.c

### Low Priority (Edge Cases)

5. **Wavetable commands ($E0-$EF)** (~20 lines)
   - Rarely used in most songs

---

## Code Locations for Implementation

### Note Initialization
**File:** `src/player/src/player.c`
**Location:** Lines 224-230 in `execute_channel()`
**Reference:** Assembly analysis in `docs/PLAYER_ANALYSIS.md` Section V.C (lines 809-917)

### Continuous Effects
**File:** `src/player/src/effects.c`
**Location:** Lines 157-199 in `execute_continuous_effect()`
**Reference:** Assembly analysis in `docs/PLAYER_ANALYSIS.md` Section V.E (lines 984-1056)

### Speed Calculation
**File:** `src/player/src/effects.c`
**New function needed:** `calculate_effect_speed()`
**Reference:** Assembly analysis Section V.E describes two modes:
- Normal: Direct values from `speed_right_table`
- Calculated: Note-relative speeds (when `speed_left_table[param-1] >= 0x80`)

---

## Reference Information

### Effect Numbers Quick Reference

| # | Name | Tick 0 | Continuous | Description |
|---|------|--------|------------|-------------|
| 0 | ARPEGGIO | setup | ✓ | Instrument vibrato |
| 1 | PORTAUP | setup | ✓ | Pitch slide up |
| 2 | PORTADOWN | setup | ✓ | Pitch slide down |
| 3 | TONEPORTA | setup | ✓ | Slide to target note |
| 4 | VIBRATO | setup | ✓ | Oscillating pitch |
| 5 | SETAD | ✓ | - | Set Attack/Decay |
| 6 | SETSR | ✓ | - | Set Sustain/Release |
| 7 | SETWAVE | ✓ | - | Set waveform |
| 8 | SETWAVEPTR | ✓ | - | Set wavetable pointer |
| 9 | SETPULSEPTR | ✓ | - | Set pulse table pointer |
| A | SETFILTPTR | ✓ | - | Set filter table pointer |
| B | SETFILTCTRL | ✓ | - | Set filter control |
| C | SETFILTCUTOFF | ✓ | - | Set filter cutoff |
| D | SETMASTERVOL | ✓ | - | Set volume or timing mark |
| E | SETFUNKTEMPO | ✓ | - | Alternating tempo |
| F | SETTEMPO | ✓ | - | Set tempo |

### SID Register Layout

**Per-voice (×3 at offsets 0, 7, 14):**

| Offset | Register | Description |
|--------|----------|-------------|
| +0 | FREQ_LO | Frequency low byte |
| +1 | FREQ_HI | Frequency high byte |
| +2 | PULSE_LO | Pulse width low byte |
| +3 | PULSE_HI | Pulse width high (bits 0-3) |
| +4 | CONTROL | Waveform + gate |
| +5 | AD | Attack/Decay |
| +6 | SR | Sustain/Release |

**Global:**

| Address | Register | Description |
|---------|----------|-------------|
| $D415 | FC_LO | Filter cutoff low (bits 0-2) |
| $D416 | FC_HI | Filter cutoff high |
| $D417 | RES_FILT | Resonance (4-7) + voice routing (0-2) |
| $D418 | MODE_VOL | Filter mode (4-7) + volume (0-3) |

### Code Patterns

**Gate bit convention:**
- `0xFF` = gate ON (bit 0 set when ANDed with waveform)
- `0xFE` = gate OFF (bit 0 cleared)
- Usage: `sid->voice[ch].control = waveform & gate;`

**1-based vs 0-based indexing:**
- Instruments: 1-based in pattern data (0 = no change)
- Table pointers: 1-based (0 = disabled)
- Notes and channels: 0-based

**Instrument array access:**
```c
// Instruments are 1-based, arrays are 0-based
uint8_t instr_idx = ch->instrument - 1;
ch->attack_decay = music->instr_ad[instr_idx];
```

---

## Files Changed Recently

```
src/player/
├── include/
│   ├── player.h           # Public API
│   ├── player_types.h     # Types and constants
│   ├── player_internal.h  # Internal API for testing
│   └── sid.h              # SID hardware interface
├── src/
│   ├── player.c           # Main loop (note init incomplete)
│   ├── sequencer.c        # Pattern processing (complete)
│   ├── effects.c          # Effects (continuous stubbed)
│   ├── wavetable.c        # Wavetable (mostly complete)
│   ├── pulsetable.c       # Pulse table (complete)
│   ├── filtertable.c      # Filter table (complete)
│   └── soundfx.c          # Sound effects (complete)
├── test/
│   ├── test_sequencer.c   # Unit tests
│   ├── test_data.h        # Test data
│   ├── c_trace.c          # C trace harness
│   ├── ref_trace.s        # Assembly trace harness (placeholder)
│   ├── run_vice.sh        # VICE runner
│   └── compare_traces.sh  # Trace comparison
└── Makefile               # Build system
```

---

## Next Steps

1. **Implement note initialization** in `player.c` `execute_channel()`
2. **Implement speed calculation** function in `effects.c`
3. **Implement continuous effects** (portamento, vibrato, toneporta, arpeggio)
4. **Implement remaining tick-0 effects** (filter, tempo, volume)
5. **Connect master volume** to filter table execution
6. **Run trace comparison** against assembly reference
7. **Fix any divergences** found in trace comparison

---

## Document History

| Date | Changes |
|------|---------|
| 2025-11-26 | Initial detailed status document |
