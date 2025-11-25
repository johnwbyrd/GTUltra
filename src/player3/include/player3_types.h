//=============================================================================
// PLAYER3_TYPES.H - Type Definitions for Player3
//=============================================================================
// This file contains all the type definitions, constants, and enums used
// by the player3 music player. It defines the data structures for:
//
// - Music data format (patterns, instruments, tables)
// - Runtime player state (channels, effects, timing)
// - Pattern encoding and commands
// - Effect numbers and parameters
//
// This file contains NO executable code, only type definitions. This allows
// it to be included in multiple compilation units without causing linker
// errors.
//=============================================================================

#ifndef PLAYER3_TYPES_H
#define PLAYER3_TYPES_H

#include <stdint.h>
#include <stddef.h>  // for NULL
#include <stdbool.h>
#include "sid.h"

//-----------------------------------------------------------------------------
// CONFIGURATION CONSTANTS
//-----------------------------------------------------------------------------

// Number of channels for player3 (1 SID chip = 3 voices)
#define NUM_CHANNELS 3

// Maximum number of songs in a multi-song file
// The relocator supports multi-song .sng files where multiple songs
// share common instrument and table data
#define MAX_SONGS 32

// Note range in frequency table
// GoatTracker supports 8 octaves (96 notes total)
#define FIRST_NOTE 0
#define LAST_NOTE 95

// Default tempo (in ticks per note)
// Higher values = slower tempo
#define DEFAULT_TEMPO 6

//=============================================================================
// PATTERN DATA ENCODING
//=============================================================================
// Pattern data is compressed to save memory. Each byte in the pattern
// stream is interpreted based on its value range:
//
// $00-$3F : Instrument change (followed by FX or NOTE)
// $40-$4F : Effect + parameter + note
// $50-$5F : Effect + parameter only (no note)
// $60-$BC : Note value
// $BD     : Rest (silence)
// $BE     : Key off (gate off, no new note)
// $BF     : Key on (gate on, no new note)
// $C0-$FE : Packed rest (rest for N frames)
// $00     : End of pattern (when pattern_ptr != 0)
//=============================================================================

/**
 * Pattern data markers and special values.
 *
 * These constants define the encoding used in compressed pattern data.
 * The pattern decoder (in sequencer.c) interprets these values to
 * extract notes, instruments, and effects from the pattern stream.
 */
typedef enum {
    PATTERN_END         = 0x00,  // End of pattern marker
    PATTERN_INSTR_MAX   = 0x3F,  // Maximum instrument number (1-based, so 63 instruments)
    PATTERN_FX          = 0x40,  // Effect command (note follows)
    PATTERN_FXONLY      = 0x50,  // Effect command (no note follows)
    PATTERN_NOTE        = 0x60,  // Note value base (actual note = value - PATTERN_NOTE)
    PATTERN_REST        = 0xBD,  // Rest (no sound)
    PATTERN_KEYOFF      = 0xBE,  // Gate off without changing note
    PATTERN_KEYON       = 0xBF,  // Gate on without changing note
    PATTERN_PACKED_REST = 0xC0,  // Packed rest base (count = value - PACKED_REST + 1)
} PatternMarker;

//=============================================================================
// EFFECT DEFINITIONS
//=============================================================================
// GoatTracker supports 16 effects (0-F). Effects are commands that modify
// how notes are played - changing pitch, volume, timbre, or triggering
// special actions like filter changes or tempo adjustments.
//
// Effects are divided into two categories:
// 1. Tick 0 effects: Execute once when the note starts
// 2. Continuous effects: Execute every tick while the note plays
//=============================================================================

/**
 * Effect numbers (0x0-0xF).
 *
 * Each effect modifies playback in a specific way. Some effects execute
 * only on tick 0 (when the note starts), while others execute continuously
 * on every tick for smooth modulation.
 */
typedef enum {
    FX_ARPEGGIO     = 0x0,  // Arpeggio/Vibrato (continuous - from instrument)
    FX_PORTAUP      = 0x1,  // Portamento up (continuous)
    FX_PORTADOWN    = 0x2,  // Portamento down (continuous)
    FX_TONEPORTA    = 0x3,  // Tone portamento - slide to note (continuous)
    FX_VIBRATO      = 0x4,  // Vibrato - oscillating pitch (continuous)
    FX_SETAD        = 0x5,  // Set Attack/Decay (tick 0)
    FX_SETSR        = 0x6,  // Set Sustain/Release (tick 0)
    FX_SETWAVE      = 0x7,  // Set waveform (tick 0)
    FX_SETWAVEPTR   = 0x8,  // Set wavetable pointer (tick 0)
    FX_SETPULSEPTR  = 0x9,  // Set pulse table pointer (tick 0)
    FX_SETFILTPTR   = 0xA,  // Set filter table pointer (tick 0)
    FX_SETFILTCTRL  = 0xB,  // Set filter control (tick 0)
    FX_SETFILTCUTOFF= 0xC,  // Set filter cutoff (tick 0)
    FX_SETMASTERVOL = 0xD,  // Set master volume or timing mark (tick 0)
    FX_SETFUNKTEMPO = 0xE,  // Set funk tempo (tick 0)
    FX_SETTEMPO     = 0xF,  // Set tempo (tick 0)
} EffectNumber;

//=============================================================================
// ORDER LIST COMMANDS
//=============================================================================
// The order list defines the song structure - which patterns play in what
// order. Special commands in the order list control looping, transposition,
// and repetition.
//=============================================================================

/**
 * Order list special commands.
 *
 * These commands appear in the order list (song sequence) to control
 * playback flow. Normal order list entries are pattern numbers (0-$CF).
 * Values $D0 and above are special commands.
 */
typedef enum {
    ORDER_REPEAT    = 0xD0,  // Repeat pattern N times (value = $D0 + count-1)
    ORDER_TRANSDOWN = 0xE0,  // Transpose down (value = $E0 + semitones)
    ORDER_TRANS     = 0xF0,  // Transpose (value = $F0 + semitones for up)
    ORDER_TRANSUP   = 0xF0,  // Alias for ORDER_TRANS
    ORDER_LOOP      = 0xFF,  // Loop to position (next byte is target position)
} OrderCommand;

//=============================================================================
// TABLE COMMANDS
//=============================================================================
// Wavetable, pulse table, and filter table commands control modulation
// over time. Tables are sequences of commands that change parameters
// automatically as the note plays.
//=============================================================================

/**
 * Wavetable commands.
 *
 * Wavetables control waveform and pitch changes over time. Values 0-15
 * are delay counts, $E0-$FE are special commands, and $FF is loop.
 */
typedef enum {
    WAVE_DELAY_MAX  = 0x0F,  // Maximum delay value (0-15 frames)
    WAVE_CMD_MIN    = 0xE0,  // Minimum command value
    WAVE_LOOP       = 0xFF,  // Loop to position (next byte is target)
} WaveCommand;

/**
 * Pulse table commands.
 *
 * Pulse tables control pulse width modulation. Values 1-$7F are modulation
 * durations, $80+ sets pulse directly, $FF is loop.
 */
typedef enum {
    PULSE_MOD_MAX   = 0x7F,  // Maximum modulation duration
    PULSE_SET       = 0x80,  // Set pulse directly (value >= $80)
    PULSE_LOOP      = 0xFF,  // Loop to position (next byte is target)
} PulseCommand;

/**
 * Filter table commands.
 *
 * Filter tables control filter modulation. Same format as pulse table,
 * plus special $00 command for cutoff-only sets.
 */
typedef enum {
    FILTER_CUTOFF   = 0x00,  // Set cutoff only (special case)
    FILTER_MOD_MAX  = 0x7F,  // Maximum modulation duration
    FILTER_SET      = 0x80,  // Set filter parameters (value >= $80)
    FILTER_LOOP     = 0xFF,  // Loop to position (next byte is target)
} FilterCommand;

//=============================================================================
// MUSIC DATA STRUCTURES
//=============================================================================
// The MusicData structure contains pointers to all the read-only music
// data: frequency tables, patterns, instruments, and modulation tables.
// This data is generated by the gt2reloc relocator tool and typically
// lives in ROM (flash memory) on the C64.
//
// All pointers are const because this data never changes during playback.
//=============================================================================

/**
 * Music data structure (read-only, lives in ROM).
 *
 * This structure holds pointers to all the music data for a song.
 * The data is organized as parallel arrays (structure of arrays pattern)
 * for memory efficiency and compatibility with the relocator output format.
 *
 * The relocator (gt2reloc.c) generates this data from a .sng file.
 */
typedef struct {
    //-------------------------------------------------------------------------
    // Frequency Table
    //-------------------------------------------------------------------------
    // Maps note numbers to SID frequency values
    // Size: 96 entries (FIRST_NOTE to LAST_NOTE)
    const uint8_t* freq_table_lo;       // Frequency low bytes
    const uint8_t* freq_table_hi;       // Frequency high bytes

    //-------------------------------------------------------------------------
    // Order Lists (Song Structure)
    //-------------------------------------------------------------------------
    // Defines which patterns play in what order for each channel
    // For player3: 3 order lists (one per channel)
    const uint8_t* const* order_lists;  // Array of 3 pointers to order lists

    //-------------------------------------------------------------------------
    // Pattern Data
    //-------------------------------------------------------------------------
    // Compressed pattern data (notes, effects, instruments)
    // Patterns are variable-length, so we have a table of pointers
    const uint8_t* pattern_table_lo;    // Pattern pointer low bytes
    const uint8_t* pattern_table_hi;    // Pattern pointer high bytes

    //-------------------------------------------------------------------------
    // Instrument Definitions
    //-------------------------------------------------------------------------
    // Each instrument has 9 parameters (parallel arrays, SoA pattern)
    // Index 0 is unused; instruments are 1-based (1-63)
    const uint8_t* instr_gatetimer;     // Gate-off timer (when to release)
    const uint8_t* instr_firstwave;     // First frame waveform
    const uint8_t* instr_pulseptr;      // Pulse table index (0=none)
    const uint8_t* instr_filterptr;     // Filter table index (0=none)
    const uint8_t* instr_waveptr;       // Wave table index (0=none)
    const uint8_t* instr_ad;            // Attack/Decay envelope
    const uint8_t* instr_sr;            // Sustain/Release envelope
    const uint8_t* instr_vibdelay;      // Vibrato delay (frames)
    const uint8_t* instr_vibparam;      // Vibrato parameter (speed)

    //-------------------------------------------------------------------------
    // Wavetable
    //-------------------------------------------------------------------------
    // Controls waveform and pitch changes over time
    // Two parallel tables: waveform values and note offsets
    const uint8_t* wave_table;          // Waveform/command sequences
    const int8_t* note_table;           // Note offsets (signed, relative/absolute)

    //-------------------------------------------------------------------------
    // Pulse Modulation Table
    //-------------------------------------------------------------------------
    // Controls pulse width modulation (PWM) over time
    const uint8_t* pulse_time_table;    // Step durations
    const uint8_t* pulse_speed_table;   // Step values/speeds

    //-------------------------------------------------------------------------
    // Filter Modulation Table
    //-------------------------------------------------------------------------
    // Controls filter sweeps and modulation over time
    const uint8_t* filter_time_table;   // Step durations
    const uint8_t* filter_speed_table;  // Step values/speeds

    //-------------------------------------------------------------------------
    // Effect Speed Tables
    //-------------------------------------------------------------------------
    // Used for portamento and vibrato effects
    // Supports both fixed speeds and note-relative "calculated" speeds
    const uint8_t* speed_left_table;    // High byte or shift count
    const uint8_t* speed_right_table;   // Low byte or flags
} MusicData;

//=============================================================================
// CHANNEL STATE
//=============================================================================
// Each channel (voice) has its own independent state for playback.
// This includes the current position in the song, active effects, timing,
// and the current SID register values.
//
// The channel state is organized into logical groups for clarity.
//=============================================================================

/**
 * Per-channel player state.
 *
 * This structure holds all the runtime state for one channel (voice).
 * The player maintains 3 of these (one per SID voice).
 *
 * Fields are grouped logically: sequencer, timing, effects, tables, SID state.
 */
typedef struct {
    //-------------------------------------------------------------------------
    // Sequencer State
    //-------------------------------------------------------------------------
    // Tracks position in the song structure (order list and patterns)

    uint8_t song_ptr;        // Position in order list (current pattern index)
    uint8_t pattern_ptr;     // Position in pattern data (0 = fetch new pattern)
    uint8_t pattern_num;     // Current pattern number being played

    int8_t transpose;        // Transpose offset (signed, -16 to +15 semitones)
    uint8_t repeat_count;    // Repeat counter for ORDER_REPEAT command
    uint8_t packed_rest;     // Packed rest counter (counts down to zero)

    //-------------------------------------------------------------------------
    // Instrument and Note State
    //-------------------------------------------------------------------------
    // What note is currently playing and with what instrument

    uint8_t instrument;      // Current instrument number (1-based, 0=invalid)
    uint8_t note;            // Current note value (0-95, FIRST_NOTE to LAST_NOTE)
    uint8_t gate;            // Gate flag: 0xFF=on (note playing), 0xFE=off (note released)
    uint8_t new_note;        // Pending note to initialize (0=none, else PATTERN_NOTE+value)

    //-------------------------------------------------------------------------
    // Effect State
    //-------------------------------------------------------------------------
    // Current and pending effects

    uint8_t effect;          // Active effect number (0-15, from EffectNumber enum)
    uint8_t effect_param;    // Active effect parameter (0-255, meaning depends on effect)
    uint8_t new_effect;      // Pending effect to apply on tick 0
    uint8_t new_param;       // Pending effect parameter to apply on tick 0

    //-------------------------------------------------------------------------
    // Timing
    //-------------------------------------------------------------------------
    // Controls when notes are fetched and processed

    uint8_t tempo;           // Tempo: ticks between note fetches (higher = slower)
    int8_t tick_counter;     // Tick counter (counts down; can go negative for funk tempo)
    uint8_t gate_timer;      // Gate-off timer: when to release note (from instrument)

    //-------------------------------------------------------------------------
    // Wavetable State
    //-------------------------------------------------------------------------
    // Automatic waveform and pitch changes over time

    uint8_t wave_ptr;        // Current wavetable position (0 = wavetable stopped)
    uint8_t wave_timer;      // Wavetable delay counter (for WAVE_DELAY)
    uint8_t waveform;        // Current waveform value (SID_WAVE_* bits)

    //-------------------------------------------------------------------------
    // Vibrato/Portamento State
    //-------------------------------------------------------------------------
    // Pitch modulation effects

    int8_t vibrato_phase;    // Vibrato phase position (signed, for oscillation)
    uint8_t vibrato_delay;   // Vibrato delay counter (counts down before vibrato starts)
    uint8_t last_note;       // Last note played (for calculated speed effects)

    //-------------------------------------------------------------------------
    // Current SID Register Values
    //-------------------------------------------------------------------------
    // Shadow registers - current values we want the SID to have
    // These are written to the SID hardware every frame

    sid_freq_t frequency;    // Frequency (16-bit, written to freq_lo/hi)
    sid_pulse_t pulse_width; // Pulse width (12-bit in 16-bit, written to pulse_lo/hi)
    uint8_t attack_decay;    // Attack/Decay envelope
    uint8_t sustain_release; // Sustain/Release envelope

    //-------------------------------------------------------------------------
    // Pulse Table State
    //-------------------------------------------------------------------------
    // Automatic pulse width modulation over time

    uint8_t pulse_ptr;       // Current pulse table position (0 = table stopped)
    uint8_t pulse_timer;     // Pulse modulation timer (counts down)

    //-------------------------------------------------------------------------
    // Sound Effect State
    //-------------------------------------------------------------------------
    // Sound effects override normal music playback

    uint8_t sfx_frame;       // SFX frame counter (0 = no SFX playing)
    const uint8_t* sfx_data; // Pointer to SFX data (NULL = no SFX)
} Channel;

//=============================================================================
// FILTER STATE
//=============================================================================
// The SID chip has one filter shared by all three voices. The filter state
// is global (not per-channel) and can be modulated over time using filter
// tables.
//=============================================================================

/**
 * Global filter state.
 *
 * The SID chip has only one filter, shared by all channels. The filter
 * can be modulated over time using filter tables (similar to wavetables
 * and pulse tables).
 */
typedef struct {
    uint8_t step_ptr;        // Current filter table position (0 = filter stopped)
    uint8_t mod_timer;       // Filter modulation timer (counts down)
    uint8_t cutoff;          // Current cutoff frequency (0-255, scaled to 11-bit)
    uint8_t control;         // Current control byte (routing and resonance)
    uint8_t type;            // Current filter type (LP/BP/HP bits)
} FilterState;

//=============================================================================
// PLAYER STATE
//=============================================================================
// The top-level player state contains all three channels, the filter,
// and global playback parameters like master volume and tempo.
//=============================================================================

/**
 * Top-level player state.
 *
 * This structure contains the complete runtime state of the player.
 * One instance of this structure is needed per player instance.
 *
 * For typical usage, allocate this in RAM (not ROM):
 *   Player player;
 *   player_init(&player, &music_data, 0);
 */
typedef struct {
    //-------------------------------------------------------------------------
    // Channel State
    //-------------------------------------------------------------------------
    // Three independent channels (one per SID voice)
    Channel channels[NUM_CHANNELS];

    //-------------------------------------------------------------------------
    // Global Filter State
    //-------------------------------------------------------------------------
    // Shared by all channels
    FilterState filter;

    //-------------------------------------------------------------------------
    // Global Playback Parameters
    //-------------------------------------------------------------------------

    uint8_t master_volume;   // Master volume (0-15, affects all voices)
    uint8_t funk_tempo[2];   // Funk tempo table (alternating tempo values for swing)

    //-------------------------------------------------------------------------
    // Initialization State
    //-------------------------------------------------------------------------

    int8_t init_song_num;    // Song number to initialize (negative = init pending)
                              // When negative, player_play() will initialize on next call
                              // After init, this is set to 0
    uint8_t song_num;        // Current song number (for multi-song files, 0-based)
} Player;

#endif // PLAYER3_TYPES_H
