# Player C Implementation Plan

## Overview

This document outlines the step-by-step implementation plan for rewriting player.s in C. The plan is organized into 8 phases, each building on the previous one, with clear deliverables and test criteria.

---

## Phase 1: Foundation & Build System

**Goal**: Get the basic infrastructure compiling and linking

### Tasks

1. **Set up directory structure** ✓
   - [x] Create `src/player/` directory
   - [x] Create `src/player/include/` for headers
   - [x] Create `src/player/src/` for implementation
   - [ ] Create `src/player/test/` for tests

2. **Create header files** ✓
   - [x] `sid.h` - SID chip register definitions
   - [x] `player_types.h` - All type definitions
   - [x] `player.h` - Public API

3. **Set up build system**
   - [ ] Create `Makefile` for llvm-mos
   - [ ] Configure compiler flags (-O2, -Wall, -Wextra)
   - [ ] Set up debug build target (with -DPLAYER_DEBUG)
   - [ ] Verify llvm-mos toolchain is accessible

4. **Create skeleton implementation files**
   - [ ] `player.c` - Main player loop (stub functions)
   - [ ] `sequencer.c` - Pattern/order list processing (stubs)
   - [ ] `effects.c` - Effect handlers (stubs)
   - [ ] `wavetable.c` - Wavetable interpreter (stub)
   - [ ] `pulsetable.c` - Pulse table interpreter (stub)
   - [ ] `filtertable.c` - Filter table interpreter (stub)
   - [ ] `soundfx.c` - Sound effect player (stub)

5. **Implement SID initialization**
   - [ ] Define SID chip pointer in `player.c`:
     ```c
     volatile SID_Chip* const sid = (SID_Chip*)SID_BASE_ADDRESS;
     ```
   - [ ] Create test program that writes to SID
   - [ ] Verify SID writes compile correctly

### Deliverable

- [ ] Code compiles without errors
- [ ] All function stubs exist
- [ ] Can link into a test program
- [ ] SID pointer is correctly defined

### Test Criteria

```c
// test_phase1.c
void test_compilation(void) {
    Player player;
    player_init(&player, NULL, 0);  // Should compile (stub does nothing)
    assert(true);  // If we got here, it compiled!
}
```

---

## Phase 2: Sequencer Implementation

**Goal**: Read and parse patterns and order lists

### Tasks

1. **Implement order list processing** (`sequencer.c`)
   - [ ] Function: `sequencer_fetch_pattern()`
     - Read order list entry
     - Handle ORDER_LOOP command
     - Handle ORDER_REPEAT command
     - Handle ORDER_TRANS/TRANSDOWN commands
     - Return pattern number to play

2. **Implement pattern decoder** (`sequencer.c`)
   - [ ] Function: `sequencer_fetch_note()`
     - Decode pattern data stream
     - Handle instrument changes (0x00-0x3F)
     - Handle effect commands (0x40-0x4F, 0x50-0x5F)
     - Handle notes (0x60-0xBC)
     - Handle REST, KEYOFF, KEYON (0xBD-0xBF)
     - Handle packed rests (0xC0-0xFE)
     - Handle pattern end (0x00)

3. **Create test data**
   - [ ] Simple order list (3 patterns, then loop)
   - [ ] Simple patterns (just notes, no effects)
   - [ ] Verify pattern decoding with unit tests

4. **Implement helper functions**
   - [ ] `read_pattern_pointer()` - Get pattern address from table
   - [ ] `read_order_entry()` - Get next order list entry

### Deliverable

- [ ] Can step through order list
- [ ] Can decode pattern data
- [ ] Unit tests pass for sequencer functions

### Test Criteria

```c
// test_sequencer.c
void test_simple_pattern(void) {
    // Pattern: Instr 1, Note C-4, Rest, Note D-4, End
    const uint8_t pattern[] = {0x01, 0x60+48, 0xBD, 0x60+50, 0x00};

    Channel ch = {0};
    uint8_t note1 = sequencer_fetch_note(&ch, pattern);
    assert(note1 == 48);  // C-4
    assert(ch.instrument == 1);

    uint8_t note2 = sequencer_fetch_note(&ch, pattern);
    assert(note2 == 0xBD);  // Rest

    uint8_t note3 = sequencer_fetch_note(&ch, pattern);
    assert(note3 == 50);  // D-4
}
```

---

## Phase 3: Basic Playback

**Goal**: Play simple melodies (notes only, no effects yet)

### Tasks

1. **Implement tick counter** (`player.c`)
   - [ ] Function: `update_tick_counter()`
     - Decrement counter
     - Reload from tempo when reaching zero
     - Handle funk tempo (alternating values)

2. **Implement frequency lookup** (`player.c`)
   - [ ] Function: `get_note_frequency()`
     - Look up frequency from tables
     - Return 16-bit frequency value

3. **Implement SID register writing** (`player.c`)
   - [ ] Function: `write_sid_registers()`
     - Write frequency (low and high bytes)
     - Write pulse width
     - Write waveform with gate
     - Write ADSR envelope

4. **Implement basic gate control** (`player.c`)
   - [ ] Handle gate on (new note starts)
   - [ ] Handle gate off (note releases)
   - [ ] Implement gate timer from instrument

5. **Implement player_init()** (`player.c`)
   - [ ] Initialize all channels to zero
   - [ ] Set default tempo
   - [ ] Load initial waveforms
   - [ ] Reset SID filter

6. **Implement player_play() skeleton** (`player.c`)
   - [ ] Complete pending initialization
   - [ ] Loop over 3 channels
   - [ ] Call execute_channel() for each
   - [ ] Write final SID values

7. **Implement execute_channel() basic** (`player.c`)
   - [ ] Update tick counter
   - [ ] Fetch new note on tick 0
   - [ ] Load note frequency
   - [ ] Handle gate on/off
   - [ ] Write to SID

### Deliverable

- [ ] Can play simple melodies
- [ ] Timing works correctly (50/60 Hz)
- [ ] Notes start and stop properly
- [ ] Multiple channels work independently

### Test Criteria

```c
// test_playback.c
void test_simple_melody(void) {
    Player player;
    MusicData music = create_test_song_simple();  // C-D-E-C melody

    player_init(&player, &music, 0);

    // Simulate 50 frames at tempo 6
    for (int frame = 0; frame < 50; frame++) {
        player_play(&player, &music);

        if (frame == 0) {
            assert(player.channels[0].frequency == freq_table[NOTE_C4]);
        }
        if (frame == 6) {
            assert(player.channels[0].frequency == freq_table[NOTE_D4]);
        }
    }
}
```

---

## Phase 4: Effect Implementation

**Goal**: Implement all 16 effects

### Tasks

1. **Implement tick 0 effect dispatch** (`effects.c`)
   - [ ] Function: `execute_tick0_effect()`
     - Switch on effect number
     - Call appropriate handler

2. **Implement tick 0 effect handlers** (`effects.c`)
   - [ ] `effect_tick0_setad()` - Set Attack/Decay
   - [ ] `effect_tick0_setsr()` - Set Sustain/Release
   - [ ] `effect_tick0_setwave()` - Set waveform
   - [ ] `effect_tick0_setwaveptr()` - Set wavetable pointer
   - [ ] `effect_tick0_setpulseptr()` - Set pulse table pointer
   - [ ] `effect_tick0_setfiltptr()` - Set filter table pointer
   - [ ] `effect_tick0_setfiltctrl()` - Set filter control
   - [ ] `effect_tick0_setfiltcutoff()` - Set filter cutoff
   - [ ] `effect_tick0_setmastervol()` - Set master volume
   - [ ] `effect_tick0_setfunktempo()` - Set funk tempo
   - [ ] `effect_tick0_settempo()` - Set tempo

3. **Implement continuous effect dispatch** (`effects.c`)
   - [ ] Function: `execute_continuous_effect()`
     - Switch on effect number
     - Call appropriate handler
     - Only on ticks > 0

4. **Implement continuous effect handlers** (`effects.c`)
   - [ ] `effect_arpeggio()` - Instrument vibrato
   - [ ] `effect_portamento_up()` - Pitch slide up
   - [ ] `effect_portamento_down()` - Pitch slide down
   - [ ] `effect_toneporta()` - Slide to target note
   - [ ] `effect_vibrato()` - Oscillating pitch

5. **Implement speed calculation** (`effects.c`)
   - [ ] Function: `calculate_effect_speed()`
     - Normal speed mode (direct values)
     - Calculated speed mode (note-relative)
     - Support for both modes

6. **Add effect support to execute_channel()** (`player.c`)
   - [ ] Call tick 0 effects when appropriate
   - [ ] Call continuous effects every tick
   - [ ] Apply effect results to frequency

### Deliverable

- [ ] All 16 effects work correctly
- [ ] Portamento slides smoothly
- [ ] Vibrato oscillates correctly
- [ ] Tempo and volume changes work

### Test Criteria

```c
// test_effects.c
void test_portamento_up(void) {
    Channel ch = {0};
    ch.frequency = 1000;
    ch.effect = FX_PORTAUP;
    ch.effect_param = 1;  // Slow speed

    uint16_t start_freq = ch.frequency;
    effect_portamento_up(&ch, &test_music);

    assert(ch.frequency > start_freq);
}

void test_vibrato(void) {
    Channel ch = {0};
    ch.frequency = 1000;
    ch.effect = FX_VIBRATO;
    ch.effect_param = 2;  // Medium speed

    uint16_t freqs[8];
    for (int i = 0; i < 8; i++) {
        effect_vibrato(&ch, &test_music);
        freqs[i] = ch.frequency;
    }

    // Should oscillate around 1000
    assert(freqs[0] != freqs[4]);  // Different phase
    assert(abs(average(freqs) - 1000) < 50);  // Centered on 1000
}
```

---

## Phase 5: Table Interpreters

**Goal**: Implement wavetable, pulse table, and filter table

### Tasks

1. **Implement wavetable interpreter** (`wavetable.c`)
   - [ ] Function: `execute_wavetable()`
     - Read wavetable command
     - Handle delay (0-15)
     - Handle waveform change
     - Handle note offsets (relative/absolute)
     - Handle loop command
     - Handle wavetable commands ($E0-$EF)

2. **Implement pulse table interpreter** (`pulsetable.c`)
   - [ ] Function: `execute_pulsetable()`
     - Read pulse table command
     - Handle set command ($80+)
     - Handle modulation (1-$7F)
     - Apply modulation to pulse width
     - Handle loop command

3. **Implement filter table interpreter** (`filtertable.c`)
   - [ ] Function: `execute_filtertable()`
     - Read filter table command
     - Handle set parameters ($80+)
     - Handle cutoff-only ($00)
     - Handle modulation (1-$7F)
     - Apply modulation to cutoff
     - Handle loop command

4. **Add table execution to player loop** (`player.c`)
   - [ ] Call wavetable executor for each channel
   - [ ] Call pulse table executor for each channel
   - [ ] Call filter table executor (global, once per frame)

### Deliverable

- [ ] Wavetables change waveform and pitch over time
- [ ] Pulse tables create PWM effects
- [ ] Filter tables sweep and modulate filters
- [ ] Complex instruments work correctly

### Test Criteria

```c
// test_tables.c
void test_wavetable_simple(void) {
    // Wavetable: Triangle, delay 5, Sawtooth, loop
    const uint8_t wave_table[] = {
        0x10 + SID_WAVE_TRIANGLE,  // Triangle
        0x05,                       // Delay 5 frames
        0x10 + SID_WAVE_SAWTOOTH,  // Sawtooth
        WAVE_LOOP, 0x01            // Loop to start
    };

    Channel ch = {0};
    ch.wave_ptr = 1;  // Start at first entry

    execute_wavetable(&ch, wave_table, NULL);
    assert(ch.waveform == SID_WAVE_TRIANGLE);

    for (int i = 0; i < 5; i++) {
        execute_wavetable(&ch, wave_table, NULL);
        assert(ch.waveform == SID_WAVE_TRIANGLE);  // Still delayed
    }

    execute_wavetable(&ch, wave_table, NULL);
    assert(ch.waveform == SID_WAVE_SAWTOOTH);  // Now sawtooth
}
```

---

## Phase 6: Sound Effects

**Goal**: Implement sound effect playback

### Tasks

1. **Implement SFX initialization** (`soundfx.c`)
   - [ ] Function: `player_play_sfx()`
     - Check priority (address comparison)
     - Set SFX active flag
     - Store SFX data pointer

2. **Implement SFX execution** (`soundfx.c`)
   - [ ] Function: `execute_soundfx()`
     - Frame 0: Hard restart (AD=0, SR=0)
     - Frame 1: Load ADSR and pulse, set test bit
     - Frame 2+: Read notes and waveforms
     - End detection ($00 byte)

3. **Add SFX override to player loop** (`player.c`)
   - [ ] Check if SFX active before normal processing
   - [ ] Call SFX executor if active
   - [ ] Resume normal music when SFX ends

### Deliverable

- [ ] Sound effects play correctly
- [ ] Priority system works
- [ ] Music resumes after SFX ends

### Test Criteria

```c
// test_sfx.c
void test_simple_sfx(void) {
    const uint8_t explosion[] = {
        0x09,       // AD
        0xF0,       // SR
        0x80,       // Pulse
        0xC0, 0x80, // Note + waveform
        0x00        // End
    };

    Player player = {0};
    player_play_sfx(&player, &test_music, 0, explosion);

    assert(player.channels[0].sfx_frame == 1);
    assert(player.channels[0].sfx_data == explosion);

    // Execute one frame
    execute_soundfx(&player.channels[0], &test_music, 0);

    // Check that ADSR was loaded
    assert(player.channels[0].attack_decay == 0x09);
}
```

---

## Phase 7: Optimization

**Goal**: Performance tuning for real hardware

### Tasks

1. **Benchmark critical paths**
   - [ ] Measure cycles for player_play()
   - [ ] Identify hot spots
   - [ ] Profile on real C64 or cycle-accurate emulator

2. **Optimize hot functions**
   - [ ] Add inline hints to small functions
   - [ ] Unroll critical loops if needed
   - [ ] Use register hints for hot variables (if llvm-mos supports)

3. **Verify timing**
   - [ ] Ensure player_play() completes in < 20,000 cycles (1MHz)
   - [ ] Test on real hardware (if available)
   - [ ] Verify no frame drops during complex songs

4. **Memory optimization**
   - [ ] Verify const data is in ROM
   - [ ] Check RAM usage (should be ~200 bytes)
   - [ ] Optimize struct packing if needed

5. **Compare to assembly version**
   - [ ] Play same song with both versions
   - [ ] Compare SID register outputs (should be identical)
   - [ ] Measure performance difference

### Deliverable

- [ ] Player runs at full speed on C64
- [ ] No timing glitches or frame drops
- [ ] Performance is comparable to assembly version
- [ ] Memory usage is acceptable

### Test Criteria

```c
// test_performance.c
void test_frame_timing(void) {
    Player player;
    MusicData music = load_complex_song();  // Worst case

    uint32_t start = get_cycle_count();
    player_play(&player, &music);
    uint32_t end = get_cycle_count();

    uint32_t cycles = end - start;

    // Must complete in one frame
    assert(cycles < 20000);  // ~20ms at 1MHz

    printf("Cycles used: %u / 20000\n", cycles);
}
```

---

## Phase 8: Testing & Documentation

**Goal**: Production-ready code with full test coverage

### Tasks

1. **Unit tests**
   - [ ] Test all effect handlers individually
   - [ ] Test all table interpreters
   - [ ] Test sequencer functions
   - [ ] Test SID register writing
   - [ ] Achieve >90% code coverage

2. **Integration tests**
   - [ ] Test with real .sng files
   - [ ] Test multi-song files
   - [ ] Test all instrument types
   - [ ] Test all effect combinations
   - [ ] Test sound effects with music

3. **Audio comparison tests**
   - [ ] Record assembly version output
   - [ ] Record C version output
   - [ ] Compare waveforms (should be identical)
   - [ ] Test on multiple songs

4. **Documentation**
   - [ ] Add code comments (already planned - heavy commenting)
   - [ ] Create API documentation
   - [ ] Write usage examples
   - [ ] Document build process
   - [ ] Document testing process

5. **Example programs**
   - [ ] Simple music player
   - [ ] Music + SFX demo
   - [ ] Interactive demo (keyboard controls)

### Deliverable

- [ ] All tests pass
- [ ] Code is fully documented
- [ ] Example programs work
- [ ] Ready for production use

### Test Criteria

```c
// test_integration.c
void test_full_song_playback(void) {
    Player player;
    MusicData music = load_song("test_data/complex_song.sng");

    player_init(&player, &music, 0);

    // Playback 10 seconds (500 frames at 50Hz)
    uint8_t sid_output[500][25];  // Record all SID writes

    for (int frame = 0; frame < 500; frame++) {
        player_play(&player, &music);

        // Capture SID state
        memcpy(sid_output[frame], sid, 25);
    }

    // Load expected output from assembly version
    uint8_t expected[500][25];
    load_expected_output("test_data/complex_song_output.bin", expected);

    // Compare (should be bit-exact)
    assert(memcmp(sid_output, expected, sizeof(sid_output)) == 0);
}
```

---

## Implementation Order

Execute phases in order:

1. ✅ Phase 1: Foundation & Build System *(start here)*
2. Phase 2: Sequencer Implementation
3. Phase 3: Basic Playback
4. Phase 4: Effect Implementation
5. Phase 5: Table Interpreters
6. Phase 6: Sound Effects
7. Phase 7: Optimization
8. Phase 8: Testing & Documentation

Each phase should be fully complete before moving to the next.

---

## Success Criteria

The implementation is complete when:

- ✅ All header files exist and compile
- [ ] All 8 phases are complete
- [ ] All tests pass
- [ ] Audio output matches assembly version bit-for-bit
- [ ] Performance is acceptable on real C64 hardware
- [ ] Code is fully documented
- [ ] Example programs demonstrate all features
- [ ] Can play any GoatTracker .sng file

---

## Current Status

**Current Phase**: Phase 1 (Foundation & Build System)

**Completed**:
- [x] Directory structure created
- [x] Header files created (sid.h, player_types.h, player.h)
- [x] Design document written

**Next Steps**:
1. Set up Makefile for llvm-mos
2. Create skeleton .c files with function stubs
3. Verify compilation works
4. Begin Phase 2 (Sequencer)

---

## Notes

- Keep commits small and focused (one feature per commit)
- Write tests before implementation (TDD where practical)
- Document as you go (don't save it for the end)
- Benchmark early and often
- Compare to assembly version frequently

---

**Last Updated**: 2025-11-25
