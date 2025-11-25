# Player C Implementation Design Document

## Project Overview

**Goal**: Rewrite player.s (Commodore 64 SID music player) in idiomatic C for llvm-mos compiler, maintaining binary compatibility with existing .sng music files while prioritizing readability and maintainability.

**Constraints**:
- Must play existing GoatTracker .sng files without modification
- Must maintain exact timing (50Hz PAL / 60Hz NTSC)
- Must fit in C64 memory constraints
- Target: llvm-mos compiler for 6502/65C02

**Priorities** (in order):
1. Readability and maintainability
2. Pedagogical value (heavily commented)
3. Correctness (exact playback)
4. Performance (optimized through clarity)

---

## Module Structure

### Directory Layout

```
src/player/
├── include/
│   ├── sid.h              // SID chip register definitions
│   ├── player_types.h     // Type definitions and constants
│   └── player.h           // Public API
├── src/
│   ├── player.c           // Main player loop and initialization
│   ├── sequencer.c        // Order list and pattern parsing
│   ├── effects.c          // Effect handlers (all 16 effects)
│   ├── wavetable.c        // Wavetable interpreter
│   ├── pulsetable.c       // Pulse table interpreter
│   ├── filtertable.c      // Filter table interpreter
│   └── soundfx.c          // Sound effect player
├── test/
│   ├── test_effects.c
│   ├── test_tables.c
│   └── test_integration.c
├── Makefile
└── README.md
```

### Module Responsibilities

#### `sid.h`
- SID chip register layout (struct-based)
- SID register bit definitions
- Inline accessor functions for SID writes

#### `player_types.h`
- All type definitions (Channel, Player, MusicData, etc.)
- All constants and enums
- No executable code

#### `player.h`
- Public API only
- Player initialization and control
- Opaque types (implementation hidden)

#### `player.c`
- Main player loop (`player_play`)
- Initialization (`player_init`)
- Channel execution orchestration
- SID register writing

#### `sequencer.c`
- Order list processing (LOOP, TRANSPOSE, REPEAT)
- Pattern data decoding
- Note fetching and parsing

#### `effects.c`
- All 16 effect handlers (tick 0)
- Continuous effect execution (portamento, vibrato)
- Effect dispatch logic

#### `wavetable.c`
- Wavetable interpreter
- Waveform sequencing
- Note offset application

#### `pulsetable.c`
- Pulse table interpreter
- Pulse width modulation

#### `filtertable.c`
- Filter table interpreter
- Filter modulation (global, shared across channels)

#### `soundfx.c`
- Sound effect playback
- Priority management
- SFX override logic

---

## Type Definitions

### SID Hardware (`sid.h`)

```c
#ifndef PLAYER_SID_H
#define PLAYER_SID_H

#include <stdint.h>

// SID chip base address
#define SID_BASE_ADDRESS 0xD400

// SID frequency type (16-bit)
typedef uint16_t sid_freq_t;

// SID pulse width type (12-bit, stored as 16-bit)
typedef uint16_t sid_pulse_t;

// SID voice registers (7 bytes per voice)
typedef struct {
    uint8_t freq_lo;        // $00/$07/$0E - Frequency low byte
    uint8_t freq_hi;        // $01/$08/$0F - Frequency high byte
    uint8_t pulse_lo;       // $02/$09/$10 - Pulse width low byte
    uint8_t pulse_hi;       // $03/$0A/$11 - Pulse width high byte (bits 0-3)
    uint8_t control;        // $04/$0B/$12 - Waveform and gate control
    uint8_t attack_decay;   // $05/$0C/$13 - Envelope attack/decay
    uint8_t sustain_release;// $06/$0D/$14 - Envelope sustain/release
} SID_Voice;

// SID chip registers (25 bytes total)
typedef struct {
    SID_Voice voice[3];           // $00-$14 - Three voices
    uint8_t filter_cutoff_lo;     // $15 - Filter cutoff low byte
    uint8_t filter_cutoff_hi;     // $16 - Filter cutoff high byte (bits 0-2)
    uint8_t filter_control;       // $17 - Filter routing and resonance
    uint8_t filter_mode_volume;   // $18 - Filter mode and master volume
} SID_Chip;

// Global SID pointer
extern volatile SID_Chip* const sid;

// Waveform control bits (for control register)
#define SID_WAVE_GATE     0x01  // Gate bit (note on/off)
#define SID_WAVE_SYNC     0x02  // Sync bit
#define SID_WAVE_RINGMOD  0x04  // Ring modulation
#define SID_WAVE_TEST     0x08  // Test bit (resets oscillator)
#define SID_WAVE_TRIANGLE 0x10  // Triangle waveform
#define SID_WAVE_SAWTOOTH 0x20  // Sawtooth waveform
#define SID_WAVE_PULSE    0x40  // Pulse waveform
#define SID_WAVE_NOISE    0x80  // Noise waveform

// Filter mode bits (for filter_mode_volume register)
#define SID_FILTER_LP     0x10  // Low-pass filter
#define SID_FILTER_BP     0x20  // Band-pass filter
#define SID_FILTER_HP     0x40  // High-pass filter
#define SID_FILTER_OFF    0x80  // Voice 3 off

// Filter routing bits (for filter_control register)
#define SID_FILTER_VOICE1 0x01  // Route voice 1 to filter
#define SID_FILTER_VOICE2 0x02  // Route voice 2 to filter
#define SID_FILTER_VOICE3 0x04  // Route voice 3 to filter
#define SID_FILTER_EXT    0x08  // Route external input to filter

// Inline SID accessor functions for performance
static inline void sid_write_freq(uint8_t voice, sid_freq_t freq) {
    sid->voice[voice].freq_lo = freq & 0xFF;
    sid->voice[voice].freq_hi = freq >> 8;
}

static inline void sid_write_pulse(uint8_t voice, sid_pulse_t pulse) {
    sid->voice[voice].pulse_lo = pulse & 0xFF;
    sid->voice[voice].pulse_hi = (pulse >> 8) & 0x0F;
}

static inline void sid_write_control(uint8_t voice, uint8_t waveform, uint8_t gate) {
    // Combine waveform with gate bit (gate is 0xFF or 0xFE)
    // ANDing with gate clears bit 0 if gate is 0xFE (off)
    sid->voice[voice].control = waveform & gate;
}

static inline void sid_write_adsr(uint8_t voice, uint8_t attack_decay, uint8_t sustain_release) {
    sid->voice[voice].attack_decay = attack_decay;
    sid->voice[voice].sustain_release = sustain_release;
}

#endif // PLAYER_SID_H
```

### Core Types (`player_types.h`)

```c
#ifndef PLAYER_TYPES_H
#define PLAYER_TYPES_H

#include <stdint.h>
#include <stdbool.h>

// Number of channels (1 SID = 3 voices)
#define NUM_CHANNELS 3

// Maximum number of songs in a multi-song file
#define MAX_SONGS 32

// First and last note in frequency table
#define FIRST_NOTE 0
#define LAST_NOTE 95

//=============================================================================
// PATTERN DATA ENCODING
//=============================================================================

// Pattern data markers and commands
typedef enum {
    PATTERN_END         = 0x00,  // End of pattern
    PATTERN_INSTR_MAX   = 0x3F,  // 0x00-0x3F = Instrument change
    PATTERN_FX          = 0x40,  // 0x40-0x4F = Effect + note follows
    PATTERN_FXONLY      = 0x50,  // 0x50-0x5F = Effect only (no note)
    PATTERN_NOTE        = 0x60,  // 0x60-0xBC = Note value
    PATTERN_REST        = 0xBD,  // Rest (no sound)
    PATTERN_KEYOFF      = 0xBE,  // Gate off without changing note
    PATTERN_KEYON       = 0xBF,  // Gate on without changing note
    PATTERN_PACKED_REST = 0xC0,  // 0xC0-0xFF = Packed rest (saves space)
} PatternMarker;

//=============================================================================
// EFFECT DEFINITIONS
//=============================================================================

// Effect numbers (0-15)
typedef enum {
    FX_ARPEGGIO     = 0,   // Instrument vibrato / arpeggio
    FX_PORTAUP      = 1,   // Pitch slide up
    FX_PORTADOWN    = 2,   // Pitch slide down
    FX_TONEPORTA    = 3,   // Slide to target note (portamento)
    FX_VIBRATO      = 4,   // Oscillating pitch
    FX_SETAD        = 5,   // Set Attack/Decay envelope
    FX_SETSR        = 6,   // Set Sustain/Release envelope
    FX_SETWAVE      = 7,   // Set waveform immediately
    FX_SETWAVEPTR   = 8,   // Set wavetable pointer
    FX_SETPULSEPTR  = 9,   // Set pulse table pointer
    FX_SETFILTPTR   = 10,  // Set filter table pointer
    FX_SETFILTCTRL  = 11,  // Set filter control (routing/resonance)
    FX_SETFILTCUTOFF= 12,  // Set filter cutoff frequency
    FX_SETMASTERVOL = 13,  // Set master volume (0-15) or timing mark (16+)
    FX_SETFUNKTEMPO = 14,  // Set funk tempo (alternating speeds)
    FX_SETTEMPO     = 15,  // Set tempo (global or per-channel)
} EffectNumber;

//=============================================================================
// ORDER LIST COMMANDS
//=============================================================================

// Order list special commands
typedef enum {
    ORDER_REPEAT    = 0xD0,  // Repeat pattern N times
    ORDER_TRANSDOWN = 0xE0,  // Transpose down (0xE0-0xEF)
    ORDER_TRANS     = 0xF0,  // Transpose (0xF0-0xFE)
    ORDER_TRANSUP   = 0xF0,  // Same as ORDER_TRANS
    ORDER_LOOP      = 0xFF,  // Loop to position
} OrderCommand;

//=============================================================================
// TABLE COMMANDS
//=============================================================================

// Wavetable commands
typedef enum {
    WAVE_LOOP       = 0xFF,  // Loop to position
} WaveCommand;

// Pulse table commands
typedef enum {
    PULSE_LOOP      = 0xFF,  // Loop to position
    PULSE_SET       = 0x80,  // Set pulse directly (0x80+)
} PulseCommand;

// Filter table commands
typedef enum {
    FILTER_LOOP     = 0xFF,  // Loop to position
    FILTER_SET      = 0x80,  // Set filter parameters (0x80+)
    FILTER_CUTOFF   = 0x00,  // Set cutoff only
} FilterCommand;

//=============================================================================
// MUSIC DATA STRUCTURES
//=============================================================================

// Music data pointers (generated by relocator, in ROM)
// All pointers are const - this data lives in flash/ROM
typedef struct {
    // Frequency table (one entry per note)
    const uint8_t* freq_table_lo;    // Low bytes of frequency values
    const uint8_t* freq_table_hi;    // High bytes of frequency values

    // Order lists (one per channel)
    const uint8_t* const* order_lists;  // Array of 3 order list pointers

    // Pattern data
    const uint8_t* pattern_table_lo;    // Low bytes of pattern pointers
    const uint8_t* pattern_table_hi;    // High bytes of pattern pointers

    // Instrument definitions (parallel arrays)
    const uint8_t* instr_gatetimer;     // Gate-off timer
    const uint8_t* instr_firstwave;     // First frame waveform
    const uint8_t* instr_pulseptr;      // Pulse table index
    const uint8_t* instr_filterptr;     // Filter table index
    const uint8_t* instr_waveptr;       // Wave table index
    const uint8_t* instr_ad;            // Attack/Decay
    const uint8_t* instr_sr;            // Sustain/Release
    const uint8_t* instr_vibdelay;      // Vibrato delay
    const uint8_t* instr_vibparam;      // Vibrato parameter

    // Wavetable
    const uint8_t* wave_table;          // Waveform sequences
    const uint8_t* note_table;          // Note offsets (relative/absolute)

    // Pulse modulation table
    const uint8_t* pulse_time_table;    // Pulse step durations
    const uint8_t* pulse_speed_table;   // Pulse step values/speeds

    // Filter modulation table
    const uint8_t* filter_time_table;   // Filter step durations
    const uint8_t* filter_speed_table;  // Filter step values/speeds

    // Effect speed tables (for calculated speed effects)
    const uint8_t* speed_left_table;    // High byte / flags
    const uint8_t* speed_right_table;   // Low byte / shift count
} MusicData;

//=============================================================================
// CHANNEL STATE
//=============================================================================

// Per-channel player state (one per voice)
// This is the runtime state that changes as music plays
typedef struct {
    //-------------------------------------------------------------------------
    // Sequencer State
    //-------------------------------------------------------------------------
    uint8_t song_ptr;        // Current position in order list
    int8_t transpose;        // Transpose amount (signed, -16 to +15)
    uint8_t repeat_count;    // Repeat counter for ORDER_REPEAT
    uint8_t pattern_ptr;     // Current position in pattern data
    uint8_t packed_rest;     // Packed rest counter

    //-------------------------------------------------------------------------
    // Current Note State
    //-------------------------------------------------------------------------
    uint8_t pattern_num;     // Current pattern number
    uint8_t instrument;      // Current instrument number (1-based)
    uint8_t note;            // Current note value (0-95)
    uint8_t gate;            // Gate flag: 0xFF=on, 0xFE=off

    //-------------------------------------------------------------------------
    // Effect State
    //-------------------------------------------------------------------------
    uint8_t effect;          // Current active effect (0-15)
    uint8_t effect_param;    // Current effect parameter
    uint8_t new_effect;      // New effect to apply on tick 0
    uint8_t new_param;       // New effect parameter to apply on tick 0
    uint8_t new_note;        // New note to initialize (0=none, else NOTE+value)

    //-------------------------------------------------------------------------
    // Timing
    //-------------------------------------------------------------------------
    uint8_t tempo;           // Tempo (ticks between notes)
    int8_t tick_counter;     // Tick counter (counts down, can go negative)
    uint8_t gate_timer;      // Gate-off timer from instrument

    //-------------------------------------------------------------------------
    // Wavetable
    //-------------------------------------------------------------------------
    uint8_t wave_ptr;        // Current wavetable position (0=stopped)
    uint8_t wave_timer;      // Wavetable delay counter
    uint8_t waveform;        // Current waveform value

    //-------------------------------------------------------------------------
    // Vibrato/Portamento
    //-------------------------------------------------------------------------
    int8_t vibrato_phase;    // Vibrato phase position (signed)
    uint8_t vibrato_delay;   // Vibrato delay counter
    uint8_t last_note;       // Last note (for calculated speed effects)

    //-------------------------------------------------------------------------
    // Current SID Hardware State
    //-------------------------------------------------------------------------
    sid_freq_t frequency;    // Current frequency (16-bit)
    sid_pulse_t pulse_width; // Current pulse width (12-bit in 16-bit)
    uint8_t attack_decay;    // Current Attack/Decay envelope
    uint8_t sustain_release; // Current Sustain/Release envelope

    //-------------------------------------------------------------------------
    // Pulse Table
    //-------------------------------------------------------------------------
    uint8_t pulse_ptr;       // Current pulse table position (0=stopped)
    uint8_t pulse_timer;     // Pulse modulation timer

    //-------------------------------------------------------------------------
    // Sound Effects
    //-------------------------------------------------------------------------
    uint8_t sfx_frame;       // Sound effect frame counter (0=inactive)
    const uint8_t* sfx_data; // Sound effect data pointer (NULL=inactive)
} Channel;

//=============================================================================
// FILTER STATE
//=============================================================================

// Global filter state (shared by all channels)
typedef struct {
    uint8_t step_ptr;        // Current filter table position (0=stopped)
    uint8_t mod_timer;       // Filter modulation timer
    uint8_t cutoff;          // Current cutoff value
    uint8_t control;         // Current control (routing/resonance)
    uint8_t type;            // Current filter type (LP/BP/HP)
} FilterState;

//=============================================================================
// PLAYER STATE
//=============================================================================

// Top-level player state
typedef struct {
    Channel channels[NUM_CHANNELS];  // Three channels (one per voice)
    FilterState filter;              // Global filter state

    uint8_t master_volume;           // Master volume (0-15)
    uint8_t funk_tempo[2];           // Funk tempo table (alternating values)

    int8_t init_song_num;            // Song to initialize (negative = pending)
    uint8_t song_num;                // Current song number (for multi-song files)
} Player;

#endif // PLAYER_TYPES_H
```

### Public API (`player.h`)

```c
#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>
#include "player_types.h"

//=============================================================================
// PLAYER API
//=============================================================================

/**
 * Initialize the player with a song number.
 *
 * This sets up the player state for the specified song and prepares all
 * channels for playback. The actual initialization happens on the next
 * call to player_play().
 *
 * @param player Pointer to player state
 * @param music Pointer to music data (must remain valid)
 * @param song_num Song number (0-based, must be < number of songs)
 *
 * NOTE: For single-song files, always pass song_num=0
 */
void player_init(Player* player, const MusicData* music, uint8_t song_num);

/**
 * Main playback routine - call once per frame.
 *
 * This is the heart of the player. It must be called at exactly 50Hz (PAL)
 * or 60Hz (NTSC) for correct playback timing. Typically called from a
 * raster interrupt or VBlank handler.
 *
 * @param player Pointer to player state
 * @param music Pointer to music data
 *
 * TIMING: This function must complete within one frame (< 20000 cycles)
 */
void player_play(Player* player, const MusicData* music);

/**
 * Play a sound effect on a channel.
 *
 * Sound effects override music playback on the specified channel. They have
 * priority management - a SFX will only play if its address is higher than
 * any currently playing SFX on that channel (higher address = higher priority).
 *
 * @param player Pointer to player state
 * @param music Pointer to music data
 * @param channel Channel number (0-2)
 * @param sfx_data Pointer to sound effect data
 *
 * Sound effect format:
 *   Byte 0: Attack/Decay
 *   Byte 1: Sustain/Release
 *   Byte 2: Pulse width
 *   Byte 3+: Note values (0x80-0xFF) or waveforms (0x00-0x81)
 *   End with 0x00
 */
void player_play_sfx(Player* player, const MusicData* music,
                     uint8_t channel, const uint8_t* sfx_data);

/**
 * Set master volume.
 *
 * @param player Pointer to player state
 * @param volume Volume level (0-15)
 */
void player_set_master_volume(Player* player, uint8_t volume);

#endif // PLAYER_H
```

---

## Coding Conventions

### Naming Conventions

**Types**: PascalCase
```c
typedef struct Channel;
typedef enum EffectNumber;
```

**Functions**: snake_case with module prefix
```c
void player_init(...);
void sequencer_fetch_pattern(...);
void effect_portamento_up(...);
```

**Variables**: snake_case
```c
uint8_t song_num;
sid_freq_t frequency;
```

**Constants/Enums**: UPPER_CASE
```c
#define NUM_CHANNELS 3
enum { FX_PORTAUP = 1 };
```

**Static functions**: snake_case with static keyword
```c
static void init_channel(Channel* ch, const MusicData* music);
```

**Inline functions**: snake_case with static inline
```c
static inline void sid_write_freq(uint8_t voice, sid_freq_t freq);
```

### File Organization

Each `.c` file should have:
```c
//=============================================================================
// FILENAME.C - Brief description
//=============================================================================
// Detailed description of what this module does, its responsibilities,
// and how it fits into the overall player architecture.
//=============================================================================

#include "player.h"
#include "player_internal.h"

//-----------------------------------------------------------------------------
// SECTION NAME
//-----------------------------------------------------------------------------
// Description of this section
//-----------------------------------------------------------------------------

// Code here...
```

### Comment Style

**Pedagogical commenting** - explain WHY, not just WHAT:

```c
/**
 * Execute continuous effects (vibrato, portamento).
 *
 * Continuous effects run every tick (not just tick 0) and modify the
 * channel's frequency in real-time. This creates smooth pitch changes
 * like vibrato (oscillating pitch) or portamento (sliding to a target).
 *
 * The assembly version uses computed jump tables for effect dispatch.
 * We use a switch statement here, which llvm-mos will likely optimize
 * to a jump table anyway, while being much more readable.
 */
static void execute_continuous_effects(Channel* ch, const MusicData* music) {
    // Only execute effects on non-zero ticks
    // (Tick 0 is reserved for fetching new notes)
    if (ch->tick_counter == 0) {
        return;
    }

    // Get the current effect and its parameter
    uint8_t effect = ch->effect;
    uint8_t param = ch->effect_param;

    // Calculate effect speed from the speed tables
    // The original player supports two speed modes:
    // 1. Normal speed: direct values from tables
    // 2. Calculated speed: note-relative speeds (for smooth glissando)
    uint16_t speed = calculate_effect_speed(ch, music, param);

    // Dispatch to the appropriate effect handler
    switch (effect) {
        case FX_ARPEGGIO:
            // Instrument vibrato - automatic vibrato defined by instrument
            effect_instrument_vibrato(ch, music);
            break;

        case FX_PORTAUP:
            // Slide pitch up by speed amount every tick
            ch->frequency += speed;
            break;

        // ... more effects
    }
}
```

### Inline Documentation

Every function should have:
```c
/**
 * Brief one-line description.
 *
 * Detailed explanation of what this function does, any important
 * implementation details, and how it relates to the original assembly.
 *
 * @param param1 Description of parameter
 * @param param2 Description of parameter
 * @return Description of return value
 *
 * NOTE: Any important notes or gotchas
 * TIMING: If this is time-critical
 * ASSEMBLY: Reference to original assembly location (line numbers)
 */
```

### Code Structure

Keep functions small and focused:
```c
// GOOD: Small, focused function
static void apply_transpose(Channel* ch, int8_t transpose) {
    // Apply transpose to note, clamping to valid range
    int16_t transposed = ch->note + transpose;
    if (transposed < FIRST_NOTE) transposed = FIRST_NOTE;
    if (transposed > LAST_NOTE) transposed = LAST_NOTE;
    ch->note = transposed;
}

// BAD: Giant monolithic function
static void do_everything(Channel* ch) {
    // 500 lines of spaghetti...
}
```

Use const correctness:
```c
// Data that won't be modified should be const
static uint16_t read_pattern_pointer(const MusicData* music, uint8_t pattern_num) {
    //                               ^^^^^ - music data is read-only
    return music->pattern_table_lo[pattern_num] |
           (music->pattern_table_hi[pattern_num] << 8);
}
```

Prefer explicit over implicit:
```c
// GOOD: Clear intent
if (ch->gate == 0xFF) {
    ch->gate = 0xFE;  // Turn gate off
}

// BAD: Magic values
if (ch->gate) {
    ch->gate--;
}
```

---

## Implementation Strategy

### Phase 1: Foundation (Start Here)

**Goal**: Get basic infrastructure compiling

1. Create all header files
2. Implement SID accessor functions
3. Implement player_init() skeleton
4. Implement player_play() skeleton (empty loop)
5. Verify it compiles with llvm-mos

**Deliverable**: Code that compiles but doesn't play music yet.

### Phase 2: Sequencer

**Goal**: Read patterns and order lists

1. Implement order list processing
2. Implement pattern decoder
3. Implement note parsing
4. Test with simple pattern data

**Deliverable**: Can step through a song's structure.

### Phase 3: Basic Playback

**Goal**: Play simple notes

1. Implement tick counter
2. Implement frequency table lookup
3. Implement SID register writing
4. Implement basic gate control

**Deliverable**: Plays simple melodies (no effects).

### Phase 4: Effects

**Goal**: Implement all 16 effects

1. Tick 0 effects (setup effects)
2. Continuous effects (portamento, vibrato)
3. Effect dispatch logic

**Deliverable**: All effects working.

### Phase 5: Tables

**Goal**: Wavetable, pulse, and filter modulation

1. Wavetable interpreter
2. Pulse table interpreter
3. Filter table interpreter

**Deliverable**: Complex instruments work.

### Phase 6: Sound Effects

**Goal**: SFX playback

1. SFX initialization
2. SFX execution
3. Priority management

**Deliverable**: Sound effects work.

### Phase 7: Optimization

**Goal**: Performance tuning

1. Benchmark critical paths
2. Add inline hints
3. Optimize hot loops
4. Verify timing

**Deliverable**: Runs at full speed on real hardware.

### Phase 8: Testing & Documentation

**Goal**: Production ready

1. Unit tests for all modules
2. Integration tests with real songs
3. Documentation completion
4. Example usage code

**Deliverable**: Fully tested, documented, production-ready player.

---

## Build System

### Makefile Structure

```makefile
# Compiler and flags
CC = mos-clang
CFLAGS = -O2 -Wall -Wextra -std=c11
CFLAGS += -I include

# Target platform
TARGET = c64

# Source files
SRCS = src/player.c \
       src/sequencer.c \
       src/effects.c \
       src/wavetable.c \
       src/pulsetable.c \
       src/filtertable.c \
       src/soundfx.c

OBJS = $(SRCS:.c=.o)

# Output
TARGET_BIN = player.lib

# Rules
all: $(TARGET_BIN)

$(TARGET_BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET_BIN)

# Debug build
debug: CFLAGS += -DPLAYER_DEBUG -g
debug: $(TARGET_BIN)

# Test build
test: CFLAGS += -DPLAYER_TEST
test: $(TARGET_BIN)
	./run_tests.sh

.PHONY: all clean debug test
```

### Debug Build Features

```c
// In player_types.h
#ifdef PLAYER_DEBUG
    #define PLAYER_ASSERT(cond, msg) \
        do { if (!(cond)) { \
            printf("ASSERT: %s at %s:%d\n", msg, __FILE__, __LINE__); \
            while(1); \
        } } while(0)

    #define PLAYER_LOG(msg) \
        printf("PLAYER: %s\n", msg)
#else
    #define PLAYER_ASSERT(cond, msg)
    #define PLAYER_LOG(msg)
#endif
```

---

## Testing Strategy

### Unit Tests

Test individual functions in isolation:

```c
// test/test_effects.c
void test_portamento_up(void) {
    Player player = {0};
    Channel* ch = &player.channels[0];

    ch->frequency = 1000;
    ch->effect = FX_PORTAUP;
    ch->effect_param = 1;  // Slow speed

    effect_portamento_up(ch, &test_music);

    assert(ch->frequency > 1000);  // Frequency should increase
}
```

### Integration Tests

Test with real music data:

```c
// test/test_integration.c
void test_simple_song(void) {
    Player player;
    const MusicData* music = load_test_song("test_data/simple.sng");

    player_init(&player, music, 0);

    // Simulate 50 frames of playback
    for (int i = 0; i < 50; i++) {
        player_play(&player, music);
        capture_sid_output();
    }

    // Verify expected SID register states
    verify_output_matches_expected();
}
```

### Timing Tests

Verify frame timing:

```c
// test/test_timing.c
void test_frame_timing(void) {
    Player player;
    uint32_t start_cycles, end_cycles;

    start_cycles = get_cycle_count();
    player_play(&player, &test_music);
    end_cycles = get_cycle_count();

    uint32_t elapsed = end_cycles - start_cycles;

    // Should complete within one frame (~20000 cycles at 1MHz)
    assert(elapsed < 20000);
}
```

### Audio Comparison Tests

Compare output to original assembly:

```c
// test/test_audio.c
void test_audio_match(void) {
    // Play same song with both assembly and C versions
    uint8_t asm_output[1000];
    uint8_t c_output[1000];

    play_with_assembly(&asm_player, music, asm_output);
    play_with_c(&c_player, music, c_output);

    // Outputs should be identical (bit-exact)
    assert(memcmp(asm_output, c_output, 1000) == 0);
}
```

---

## Performance Considerations

### Inline Strategy

**Always inline** (hot path, small functions):
```c
static inline void sid_write_freq(uint8_t voice, sid_freq_t freq);
static inline uint16_t read_pattern_pointer(const MusicData* music, uint8_t pattern);
```

**Let compiler decide** (medium-sized functions):
```c
static void execute_wavetable(Channel* ch, const MusicData* music);
```

**Never inline** (large functions, cold paths):
```c
void player_init(Player* player, const MusicData* music, uint8_t song_num);
```

### Loop Optimization

Use simple loops for clarity; trust compiler to optimize:

```c
// GOOD: Clear and simple
for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
    execute_channel(&player->channels[i], i, music);
}

// ONLY if benchmarking shows loop overhead is significant:
execute_channel(&player->channels[0], 0, music);
execute_channel(&player->channels[1], 1, music);
execute_channel(&player->channels[2], 2, music);
```

### Register Pressure

6502 has only 3 registers (A, X, Y). Keep hot variables simple:

```c
// GOOD: Simple variables compiler can keep in registers
uint8_t note = ch->note;
uint16_t freq = frequency_table[note];

// BAD: Complex expressions that force spills
ch->frequency = ((music->freq_table_hi[ch->note + ch->transpose] << 8) |
                 music->freq_table_lo[ch->note + ch->transpose]) +
                 calculate_vibrato(ch);
```

### Const Data in ROM

Keep read-only data in flash:

```c
// This stays in ROM, not RAM
static const uint8_t funk_tempo_default[2] = {8, 5};

// Music data pointers are all const
typedef struct {
    const uint8_t* freq_table_lo;  // Points to ROM
    const uint8_t* freq_table_hi;  // Points to ROM
    // ...
} MusicData;
```

---

## Memory Layout

### Expected Memory Usage

**Code segment** (~4-6 KB):
- player.c: ~1.5 KB
- sequencer.c: ~1 KB
- effects.c: ~1.5 KB
- tables: ~1 KB
- soundfx.c: ~0.5 KB

**Data segment** (~200 bytes):
- Player state: ~150 bytes (3 channels × ~50 bytes each)
- Filter state: ~10 bytes
- Global state: ~40 bytes

**Music data** (variable, in ROM):
- Depends on song complexity
- Typical: 2-10 KB per song

**Total**: ~5-7 KB code + ~200 bytes RAM + song data in ROM

---

## Next Steps

1. **Create all header files** (sid.h, player_types.h, player.h)
2. **Create skeleton .c files** with function stubs
3. **Set up build system** (Makefile, verify llvm-mos works)
4. **Implement Phase 1** (foundation - get it compiling)
5. **Create test harness** (basic framework for testing)
6. **Implement Phase 2-8** (sequentially, with tests for each)

---

## References

- Original assembly: `src/player.s`
- Architecture analysis: `PLAYER_ANALYSIS.md`
- GoatTracker documentation: `README.md`, `goattrk2.txt`
- SID chip documentation: Various online resources
- llvm-mos documentation: https://llvm-mos.org/

---

## Appendix: Example Usage

```c
#include "player.h"

// Music data (in ROM, generated by relocator)
extern const MusicData my_song_data;

// Player state (in RAM)
static Player player;

// Initialize player
void init_music(void) {
    player_init(&player, &my_song_data, 0);  // Song 0
}

// Call from interrupt (50Hz PAL or 60Hz NTSC)
void music_interrupt(void) {
    player_play(&player, &my_song_data);
}

// Play a sound effect
void play_explosion_sfx(void) {
    extern const uint8_t explosion_sfx[];
    player_play_sfx(&player, &my_song_data, 0, explosion_sfx);
}

// Change master volume
void set_volume(uint8_t vol) {
    player_set_master_volume(&player, vol & 0x0F);
}
```

---

**End of Design Document**
