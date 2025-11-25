//=============================================================================
// TEST_DATA.C - Test Data for Player3 Sequencer Validation
//=============================================================================
// This file contains test data to validate the sequencer implementation.
// It includes test patterns with all encoding formats and order lists
// with LOOP, REPEAT, and TRANSPOSE commands.
//
// USAGE:
//   Include this file in test programs to get access to test_music_data
//=============================================================================

#include "../include/player3.h"
#include "../include/player3_types.h"

//=============================================================================
// FREQUENCY TABLES
//=============================================================================

// Simple C-3 major scale frequency table (for testing)
// Note indices: 0=C-3, 1=D-3, 2=E-3, 3=F-3, 4=G-3, 5=A-3, 6=B-3, 7=C-4
const uint8_t test_freq_table_lo[128] = {
    // C-3 = 269 Hz ($010D)
    0x0D, 0x1C, 0x2D, 0x40, 0x55, 0x6C, 0x85, 0xA1,
    // Rest are zeros for now
    [8 ... 127] = 0x00
};

const uint8_t test_freq_table_hi[128] = {
    // High bytes for C major scale
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    // Rest are zeros
    [8 ... 127] = 0x00
};

//=============================================================================
// INSTRUMENT TABLES (Structure of Arrays format)
//=============================================================================
// Each instrument parameter is in its own array. Index 0 is unused (1-based).

// Gate timer: when to release the note
const uint8_t test_instr_gatetimer[] = {
    0x00,  // [0] Unused
    0x01,  // [1] Gate timer 1
    0x02,  // [2] Gate timer 2
    [3 ... 63] = 0x00
};

// First waveform on note trigger
const uint8_t test_instr_firstwave[] = {
    0x00,  // [0] Unused
    0x41,  // [1] Pulse + gate
    0x21,  // [2] Sawtooth + gate
    [3 ... 63] = 0x00
};

// Pulse table pointer (0 = none)
const uint8_t test_instr_pulseptr[] = {
    0x00,  // [0] Unused
    0x01,  // [1] Use pulse table 1
    0x00,  // [2] No pulse table
    [3 ... 63] = 0x00
};

// Filter table pointer (0 = none)
const uint8_t test_instr_filterptr[] = {
    0x00,  // [0] Unused
    0x00,  // [1] No filter
    0x00,  // [2] No filter
    [3 ... 63] = 0x00
};

// Wave table pointer
const uint8_t test_instr_waveptr[] = {
    0x00,  // [0] Unused
    0x01,  // [1] Wave table 1
    0x02,  // [2] Wave table 2
    [3 ... 63] = 0x00
};

// Attack/Decay
const uint8_t test_instr_ad[] = {
    0x00,  // [0] Unused
    0x09,  // [1] Fast attack
    0x08,  // [2] Fast attack
    [3 ... 63] = 0x00
};

// Sustain/Release
const uint8_t test_instr_sr[] = {
    0x00,  // [0] Unused
    0xF0,  // [1] High sustain
    0xE0,  // [2] High sustain
    [3 ... 63] = 0x00
};

// Vibrato delay
const uint8_t test_instr_vibdelay[] = {
    0x00,  // [0] Unused
    0x00,  // [1] No delay
    0x00,  // [2] No delay
    [3 ... 63] = 0x00
};

// Vibrato parameter
const uint8_t test_instr_vibparam[] = {
    0x00,  // [0] Unused
    0x00,  // [1] No vibrato
    0x00,  // [2] No vibrato
    [3 ... 63] = 0x00
};

//=============================================================================
// WAVETABLE DATA
//=============================================================================

const uint8_t test_wave_table[] = {
    // Table 0: Empty (end immediately)
    0x00,

    // Table 1: Simple pulse wave hold (starts at index 1)
    0x11,        // Delay 1 frame + waveform byte follows (0x10 + 1)
    0x41,        // Waveform: gate + pulse
    0xFF,        // Loop command
    0x01,        // Loop to index 1

    // Table 2: Sawtooth (starts at index 5)
    0x11,        // Delay 1 frame + waveform
    0x21,        // Waveform: gate + sawtooth
    0xFF,        // Loop command
    0x05,        // Loop to index 5
};

// Note table (signed offsets for wavetable)
const int8_t test_note_table[] = {
    0x00,        // [0] No offset
    0x00,        // [1] No offset
    0x00,        // [2] No offset
    0x00,        // [3] No offset
    0x00,        // [4] No offset
    0x00,        // [5] No offset
    0x00,        // [6] No offset
    0x00,        // [7] No offset
    0x00,        // [8] No offset
};

//=============================================================================
// PULSE TABLE DATA
//=============================================================================

const uint8_t test_pulse_time_table[] = {
    // Table 0: Empty
    0x00,

    // Table 1: Simple pulse (starts at index 1)
    0x80,        // Set pulse width (high bit = set)
    0x01,        // Modulation step
    0xFF,        // Loop command
    0x01,        // Loop to index 1
};

const uint8_t test_pulse_speed_table[] = {
    // Table 0: Empty
    0x00,

    // Table 1: Matches time table
    0x80,        // Pulse width value
    0x10,        // Modulation speed
    0x00,        // Jump target (loop)
    0x00,        // Unused
};

//=============================================================================
// FILTER TABLE DATA
//=============================================================================

const uint8_t test_filter_time_table[] = {
    // No filter table used
    0x00,
};

const uint8_t test_filter_speed_table[] = {
    // No filter table used
    0x00,
};

//=============================================================================
// EFFECT SPEED TABLES
//=============================================================================

const uint8_t test_speed_left_table[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    [8 ... 255] = 0x00
};

const uint8_t test_speed_right_table[] = {
    0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
    [8 ... 255] = 0x00
};

//=============================================================================
// PATTERN DATA
//=============================================================================

// Pattern pointer tables (low and high bytes)
// For simplicity, pattern data starts at offset 0 in test_pattern_data
const uint8_t test_pattern_table_lo[] = {
    0x00,   // Pattern 0 at offset 0
    0x20,   // Pattern 1 at offset 32
    0x40,   // Pattern 2 at offset 64
    0x60,   // Pattern 3 at offset 96
    0x80,   // Pattern 4 at offset 128
    0xA0,   // Pattern 5 at offset 160
};

const uint8_t test_pattern_table_hi[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const uint8_t test_pattern_data[] = {
    //-------------------------------------------------------------------------
    // Pattern 0: Test all basic encoding formats
    //-------------------------------------------------------------------------
    0x00,        // [0] Dummy byte (patterns are 1-indexed)

    // Test: Instrument change
    0x01,        // [1] Instrument 1

    // Test: Note (0x60 = note 0 = C-3)
    0x60,        // [2] Note C-3 (no instrument change)

    // Test: Effect with note (0x40-0x4F range)
    0x40 | FX_PORTAUP,  // [3] Effect: Portamento up with note
    0x10,        // [4] Effect parameter
    0x61,        // [5] Note D-3

    // Test: Effect without note (0x50-0x5F range)
    0x50 | FX_VIBRATO,  // [6] Effect: Vibrato (no note)
    0x46,        // [7] Effect parameter

    // Test: Rest
    0xBD,        // [8] REST

    // Test: KeyOff
    0xBE,        // [9] KEYOFF

    // Test: KeyOn
    0xBF,        // [10] KEYON

    // Test: Packed rest (0xC0 + count)
    0xC3,        // [11] Rest for 4 frames (0xC3 - 0xC0 + 1 = 4)

    // End of pattern
    0x00,        // [12] End

    // Padding to offset 32
    [13 ... 31] = 0x00,

    //-------------------------------------------------------------------------
    // Pattern 1: Test instrument + note combination
    //-------------------------------------------------------------------------
    0x00,        // [32] Dummy byte

    // Instrument change followed by note
    0x02,        // [33] Instrument 2
    0x62,        // [34] Note E-3

    // Same instrument, new note
    0x63,        // [35] Note F-3

    // Note with transpose (will be applied by order list)
    0x64,        // [36] Note G-3

    0x00,        // [37] End
    [38 ... 63] = 0x00,

    //-------------------------------------------------------------------------
    // Pattern 2: Test effect combinations
    //-------------------------------------------------------------------------
    0x00,        // [64] Dummy

    // Set ADSR values
    0x50 | FX_SETAD,   // [65] Set Attack/Decay (no note)
    0x09,              // [66] Parameter

    0x50 | FX_SETSR,   // [67] Set Sustain/Release (no note)
    0xF0,              // [68] Parameter

    // Set waveform
    0x50 | FX_SETWAVE, // [69] Set waveform (no note)
    0x41,              // [70] Pulse + gate

    // Note with vibrato effect
    0x40 | FX_VIBRATO, // [71] Vibrato with note
    0x46,              // [72] Parameter
    0x65,              // [73] Note A-3

    0x00,              // [74] End
    [75 ... 95] = 0x00,

    //-------------------------------------------------------------------------
    // Pattern 3: Test long packed rest sequence
    //-------------------------------------------------------------------------
    0x00,        // [96] Dummy

    0x01,        // [97] Instrument 1
    0x60,        // [98] Note C-3

    // Long rest (max single packed rest is 0xFE - 0xC0 + 1 = 63 frames)
    0xFE,        // [99] Rest for 63 frames

    0x00,        // [100] End
    [101 ... 127] = 0x00,

    //-------------------------------------------------------------------------
    // Pattern 4: Empty pattern
    //-------------------------------------------------------------------------
    0x00,        // [128] Dummy
    0x00,        // [129] End immediately
    [130 ... 159] = 0x00,

    //-------------------------------------------------------------------------
    // Pattern 5: Test note range
    //-------------------------------------------------------------------------
    0x00,        // [160] Dummy

    // Low note
    0x60,        // [161] Note 0 (C-3)

    // Mid note
    0x80,        // [162] Note 32 (middle range)

    // High note
    0xBC,        // [163] Note 92 (high range, 0xBC is max note value)

    0x00,        // [164] End
    [165 ... 191] = 0x00,
};

//=============================================================================
// ORDER LIST DATA
//=============================================================================

// Order list for channel 0: Test all order list commands
const uint8_t test_order_list_0[] = {
    // Play pattern 0 normally
    0x00,        // [0] Pattern 0

    // Test TRANSPOSE UP (+2 semitones)
    ORDER_TRANS + 2,  // [1] Transpose +2
    0x01,             // [2] Pattern 1 (notes will be +2)

    // Test TRANSPOSE DOWN (-1 semitone)
    ORDER_TRANSDOWN + 1,  // [3] Transpose -1
    0x01,                 // [4] Pattern 1 again (notes will be -1)

    // Reset transpose to 0
    ORDER_TRANS,     // [5] Transpose 0

    // Test REPEAT (repeat next pattern 2 times)
    ORDER_REPEAT + 2,  // [6] Repeat 2 times
    0x02,              // [7] Pattern 2 (will play 3 times total: 1 + 2 repeats)

    // Test LOOP
    0x03,              // [8] Pattern 3
    ORDER_LOOP,        // [9] Loop to...
    0x00,              // [10] ...position 0 (infinite loop)
};

// Order list for channel 1: Simple pattern sequence
const uint8_t test_order_list_1[] = {
    0x00,        // Pattern 0
    0x01,        // Pattern 1
    0x02,        // Pattern 2
    ORDER_LOOP,  // Loop to...
    0x00,        // ...start
};

// Order list for channel 2: Empty (just pattern 4 looping)
const uint8_t test_order_list_2[] = {
    0x04,        // Pattern 4 (empty)
    ORDER_LOOP,  // Loop to...
    0x00,        // ...start
};

//=============================================================================
// SONG DATA
//=============================================================================

// Pointers to order lists for each channel
const uint8_t* test_order_list_pointers[NUM_CHANNELS] = {
    test_order_list_0,
    test_order_list_1,
    test_order_list_2,
};

//=============================================================================
// MAIN MUSIC DATA STRUCTURE
//=============================================================================

const MusicData test_music_data = {
    // Frequency tables
    .freq_table_lo = test_freq_table_lo,
    .freq_table_hi = test_freq_table_hi,

    // Order lists
    .order_lists = test_order_list_pointers,

    // Pattern tables
    .pattern_table_lo = test_pattern_table_lo,
    .pattern_table_hi = test_pattern_table_hi,

    // Instrument parameters (Structure of Arrays)
    .instr_gatetimer = test_instr_gatetimer,
    .instr_firstwave = test_instr_firstwave,
    .instr_pulseptr = test_instr_pulseptr,
    .instr_filterptr = test_instr_filterptr,
    .instr_waveptr = test_instr_waveptr,
    .instr_ad = test_instr_ad,
    .instr_sr = test_instr_sr,
    .instr_vibdelay = test_instr_vibdelay,
    .instr_vibparam = test_instr_vibparam,

    // Wavetable
    .wave_table = test_wave_table,
    .note_table = test_note_table,

    // Pulse table
    .pulse_time_table = test_pulse_time_table,
    .pulse_speed_table = test_pulse_speed_table,

    // Filter table
    .filter_time_table = test_filter_time_table,
    .filter_speed_table = test_filter_speed_table,

    // Effect speed tables
    .speed_left_table = test_speed_left_table,
    .speed_right_table = test_speed_right_table,
};

//=============================================================================
// TEST VALIDATION HELPERS
//=============================================================================

/**
 * Expected values for sequencer test validation.
 * These define what we expect to see when parsing each pattern.
 */

typedef struct {
    uint8_t pattern_num;
    uint8_t position;
    uint8_t expected_instrument;
    uint8_t expected_note;
    uint8_t expected_effect;
    uint8_t expected_param;
    const char* description;
} PatternTestCase;

// Pattern 0 test cases (all encoding formats)
const PatternTestCase pattern0_tests[] = {
    // Instrument change
    {0, 1, 1, 0xFF, 0, 0, "Instrument 1 change"},

    // Note
    {0, 2, 1, 0x00, 0, 0, "Note C-3 (index 0)"},

    // Effect with note
    {0, 3, 1, 0x01, FX_PORTAUP, 0x10, "Portamento up with D-3"},

    // Effect without note
    {0, 6, 1, 0xFF, FX_VIBRATO, 0x46, "Vibrato (no note)"},

    // Rest
    {0, 8, 1, PATTERN_REST, 0, 0, "Rest"},

    // KeyOff
    {0, 9, 1, PATTERN_KEYOFF, 0, 0, "KeyOff"},

    // KeyOn
    {0, 10, 1, PATTERN_KEYON, 0, 0, "KeyOn"},

    // Packed rest - this advances pattern_ptr by 4
    {0, 11, 1, PATTERN_REST, 0, 0, "Packed rest (4 frames)"},
};

// Order list test expectations
typedef struct {
    uint8_t song_position;
    uint8_t expected_pattern;
    int8_t expected_transpose;
    uint8_t expected_repeat_count;
    const char* description;
} OrderListTestCase;

const OrderListTestCase orderlist_tests[] = {
    {0, 0, 0, 0, "Pattern 0, no transpose"},
    {1, 1, 2, 0, "Pattern 1, transpose +2"},
    {3, 1, -1, 0, "Pattern 1, transpose -1"},
    {5, 2, 0, 0, "Pattern 2, transpose reset"},
    // Repeat test requires multiple calls to properly validate
    {6, 2, 0, 1, "Pattern 2, first repeat (count=1)"},
    {6, 2, 0, 2, "Pattern 2, second repeat (count=2)"},
    {8, 3, 0, 0, "Pattern 3, repeats done"},
    // Loop wraps back to position 0
    {9, 0, 0, 0, "Loop back to position 0"},
};

//=============================================================================
// End of test_data.c
//=============================================================================
