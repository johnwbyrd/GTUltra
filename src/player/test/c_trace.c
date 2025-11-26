//=============================================================================
// C Trace Harness - C Player Test
//=============================================================================
// This program runs the C player with the same minimal test data as the
// assembly harness and outputs SID register states for comparison.
//=============================================================================

#include <stdint.h>
#include <stdio.h>
#include "../include/player.h"
#include "../include/sid.h"
#include "test_data.h"

//=============================================================================
// Constants
//=============================================================================

#define NUM_TICKS   50      // Number of ticks to capture (same as assembly)
#define TRACE_SIZE  25      // SID has 25 registers ($D400-$D418)

//=============================================================================
// Trace Buffer
//=============================================================================

static uint8_t trace_buffer[NUM_TICKS * TRACE_SIZE];
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
// Output Trace
//=============================================================================
// Outputs trace buffer in same format as assembly version
// Format: TICK:nn D400:xx D401:xx ... D418:xx

void output_trace(void) {
    uint16_t buf_index = 0;

    for (uint8_t tick = 0; tick < NUM_TICKS; tick++) {
        // Output tick number
        printf("TICK:%02X ", tick);

        // Output 25 register values
        for (uint8_t reg = 0; reg < TRACE_SIZE; reg++) {
            printf("D4%02X:%02X ", reg, trace_buffer[buf_index++]);
        }

        printf("\n");
    }
}

//=============================================================================
// Print Hex Byte (compatibility function for direct screen output on C64)
//=============================================================================

void print_hex_byte(uint8_t value) {
    // On C64, this would use KERNAL routines
    // For now, we'll use the trace buffer approach
    (void)value;  // Unused in this version
}

//=============================================================================
// Main Program
//=============================================================================

int main(void) {
    // Initialize player state
    static Player player;

    // Initialize with test music data (song 0)
    player_init(&player, &test_music_data, 0);

    // Run for NUM_TICKS frames
    for (uint8_t tick = 0; tick < NUM_TICKS; tick++) {
        // Call player
        player_play(&player, &test_music_data);

        // Capture SID state
        capture_sid_state();
    }

    // Output trace
    output_trace();

    // On C64, program would halt here
    // For simulation, we just return
    return 0;
}

//=============================================================================
// C64-specific BASIC stub (for .prg format)
//=============================================================================
// This would normally be in a linker script or startup code
// llvm-mos handles this automatically for c64 target

#ifdef __mos_c64__
// The compiler will handle BASIC stub and startup code
#endif
