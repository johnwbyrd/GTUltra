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
// INSTRUMENT TABLE
//=============================================================================

// Simple test instruments
// Format: AD, SR, Wavetable, Pulsetable, Filtertable, Vibrato, GateTimer
const uint8_t test_instrument_table[] = {
    // Instrument 0: Empty/default
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // Instrument 1: Basic pulse
    0x09, 0xF0,  // AD, SR (fast attack, high sustain)
    0x01,        // Wavetable 1
    0x01,        // Pulsetable 1
    0x00,        // No filter table
    0x00,        // No vibrato
    0x01,        // Gate timer 1

    // Instrument 2: Sawtooth lead
    0x08, 0xE0,  // AD, SR
    0x02,        // Wavetable 2
    0x00,        // No pulse table
    0x00,        // No filter
    0x00,        // No vibrato
    0x02,        // Gate timer 2

    // Rest are empty
    [21 ... 255] = 0x00
};

//=============================================================================
// WAVETABLE DATA
//=============================================================================

const uint8_t test_wavetable_data[] = {
    // Table 0: Empty (end immediately)
    0x00,

    // Table 1: Simple pulse wave hold
    0x01,        // Delay 1 frame
    0x41,        // Waveform: gate + pulse
    0xFF,        // Loop to start

    // Table 2: Sawtooth with vibrato
    0x01,        // Delay 1 frame
    0x21,        // Waveform: gate + sawtooth
    0xFF,        // Loop
};

//=============================================================================
// PULSE TABLE DATA
//=============================================================================

const uint8_t test_pulsetable_data[] = {
    // Table 0: Empty
    0x00,

    // Table 1: Simple pulse sweep
    0x00,        // Speed/delay
    0x80, 0x00,  // Start pulse width (lo, hi)
    0xFF,        // Set command
    0x10, 0x00,  // Modulation amount
    0x00,        // End
};

//=============================================================================
// FILTER TABLE DATA
//=============================================================================

const uint8_t test_filtertable_data[] = {
    // Global filter table: Simple hold
    0x00,        // Speed
    0x00, 0x00,  // Cutoff (lo, hi)
    0x00,        // Control
    0x00,        // End
};

//=============================================================================
// PATTERN DATA
//=============================================================================

// Pattern pointers (index into pattern_data)
const uint16_t test_pattern_pointers[] = {
    0,     // Pattern 0
    32,    // Pattern 1
    64,    // Pattern 2
    96,    // Pattern 3
    128,   // Pattern 4
    160,   // Pattern 5
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

// Song names (optional, for debugging)
const char* test_song_names[] = {
    "Test Song 0",
};

//=============================================================================
// MAIN MUSIC DATA STRUCTURE
//=============================================================================

const MusicData test_music_data = {
    // Frequency tables
    .freq_table_lo = test_freq_table_lo,
    .freq_table_hi = test_freq_table_hi,

    // Instrument table
    .instrument_table = test_instrument_table,

    // Wavetable
    .wavetable_lo = (const uint8_t*)((uintptr_t)test_wavetable_data & 0xFF),
    .wavetable_hi = (const uint8_t*)((uintptr_t)test_wavetable_data >> 8),

    // Pulse table
    .pulsetable_lo = (const uint8_t*)((uintptr_t)test_pulsetable_data & 0xFF),
    .pulsetable_hi = (const uint8_t*)((uintptr_t)test_pulsetable_data >> 8),

    // Filter table
    .filtertable_lo = (const uint8_t*)((uintptr_t)test_filtertable_data & 0xFF),
    .filtertable_hi = (const uint8_t*)((uintptr_t)test_filtertable_data >> 8),

    // Pattern data
    .pattern_lo = (const uint8_t*)((uintptr_t)test_pattern_pointers & 0xFF),
    .pattern_hi = (const uint8_t*)((uintptr_t)test_pattern_pointers >> 8),

    // Order lists
    .order_lists = test_order_list_pointers,

    // Song names (optional)
    .song_names = test_song_names,

    // Initial tempo (funk tempo values)
    .initial_tempo = {6, 6},  // Standard tempo (no funk)

    // Filter parameters
    .filter_params = {0, 0, 0},  // No filter initially
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
    {0, 8, 1, NOTE_REST, 0, 0, "Rest"},

    // KeyOff
    {0, 9, 1, NOTE_KEYOFF, 0, 0, "KeyOff"},

    // KeyOn
    {0, 10, 1, NOTE_KEYON, 0, 0, "KeyOn"},

    // Packed rest - this advances pattern_ptr by 4
    {0, 11, 1, NOTE_REST, 0, 0, "Packed rest (4 frames)"},
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
