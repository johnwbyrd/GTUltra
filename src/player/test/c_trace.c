//=============================================================================
// C Trace Harness - C Player Test
//=============================================================================
// This program runs the C player with the same minimal test data as the
// assembly harness and outputs SID register states for comparison.
//
// The trace buffer is stored at a fixed memory address so VICE's monitor
// can dump it to a file after execution.
//=============================================================================

#include <stdint.h>
#include "../include/player.h"
#include "../include/sid.h"
#include "test_data.h"

//=============================================================================
// Constants
//=============================================================================

#define NUM_TICKS   50      // Number of ticks to capture (same as assembly)
#define TRACE_SIZE  25      // SID has 25 registers ($D400-$D418)

//=============================================================================
// Trace Buffer at fixed address
//=============================================================================
// Located at $4000 (16384) - safe area above BASIC, below screen memory
// Total size: 50 ticks * 25 registers = 1250 bytes
// VICE monitor can dump: save "trace.bin" 0 4000 44e1

#define TRACE_BUFFER_ADDR 0x4000
#define TRACE_HEADER_ADDR 0x3FF0  // Header with magic and tick count

// Header structure at $3FF0:
// $3FF0-$3FF3: Magic "TRC\0"
// $3FF4-$3FF5: Number of ticks captured (16-bit)
// $3FF6-$3FF7: Trace size per tick (25)
// Trace data starts at $4000

static uint8_t* trace_buffer = (uint8_t*)TRACE_BUFFER_ADDR;
static uint8_t* trace_header = (uint8_t*)TRACE_HEADER_ADDR;
static uint16_t trace_index = 0;

//=============================================================================
// SID State Capture
//=============================================================================
// Copies all 25 SID registers to trace buffer

void capture_sid_state(void) {
    volatile SID_Chip* sid_ptr = (volatile SID_Chip*)SID_BASE_ADDRESS;
    const uint8_t* sid_bytes = (const uint8_t*)sid_ptr;

    for (uint8_t i = 0; i < TRACE_SIZE; i++) {
        trace_buffer[trace_index++] = sid_bytes[i];
    }
}

//=============================================================================
// Write Header
//=============================================================================
// Writes header info so the dump tool knows the format

void write_header(uint16_t ticks) {
    trace_header[0] = 'T';
    trace_header[1] = 'R';
    trace_header[2] = 'C';
    trace_header[3] = 0;
    trace_header[4] = (uint8_t)(ticks & 0xFF);
    trace_header[5] = (uint8_t)(ticks >> 8);
    trace_header[6] = TRACE_SIZE;
    trace_header[7] = 0;
}

//=============================================================================
// Signal Completion
//=============================================================================
// Write a completion marker that VICE can detect

void signal_done(void) {
    // Write "DONE" at $3FFC so we can detect completion
    trace_header[12] = 'D';
    trace_header[13] = 'O';
    trace_header[14] = 'N';
    trace_header[15] = 'E';
}

//=============================================================================
// Main Program
//=============================================================================

int main(void) {
    // Initialize player state
    static Player player;

    // Clear trace buffer area
    for (uint16_t i = 0; i < NUM_TICKS * TRACE_SIZE; i++) {
        trace_buffer[i] = 0;
    }

    // Write header
    write_header(NUM_TICKS);

    // Initialize with test music data (song 0)
    player_init(&player, &test_music_data, 0);

    // Run for NUM_TICKS frames
    for (uint8_t tick = 0; tick < NUM_TICKS; tick++) {
        // Call player
        player_play(&player, &test_music_data);

        // Capture SID state
        capture_sid_state();
    }

    // Signal completion
    signal_done();

    // Halt CPU - triggers VICE monitor entry
    for (;;) {
        __asm__ volatile ("brk");
    }

    return 0;
}
