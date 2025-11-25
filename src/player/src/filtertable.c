//=============================================================================
// FILTERTABLE.C - Filter Table Interpreter
//=============================================================================
// Filter tables control automatic filter sweeps and modulation over time.
//
// The SID chip has one shared filter that can be routed to any combination
// of the three voices. The filter table automates changes to filter cutoff
// frequency, resonance, and routing over time.
//
// FILTER TABLE FORMAT:
// - Value 0: Set cutoff only (next byte in speed table is cutoff value)
// - Values 1-127: Modulation duration (run for N frames)
// - Values 128+: Set filter parameters (passband type)
// - Value 255: Loop to position (next byte is target position)
//
// ASSOCIATED FILTER SPEED TABLE:
// - For value 0: Cutoff value
// - For modulation: Speed value to add/subtract each frame
// - For set command: Filter control byte (routing and resonance)
//
// Filter sweeps are commonly used for dramatic effects like rising/falling
// tones, wah-wah effects, and evolving timbres.
//=============================================================================

#include "../include/player.h"
#include "../include/player_types.h"
#include "../include/sid.h"

/**
 * Execute filter table for one frame.
 *
 * Processes one step of the filter table sequence, potentially modulating
 * the filter cutoff or setting new filter parameters.
 *
 * Note: The filter is global (shared by all channels), so this function
 * operates on the Player's FilterState, not a channel.
 */
void execute_filtertable(FilterState* filter, const MusicData* music) {
    // Check if filter table is active
    if (filter->step_ptr == 0) {
        // Filter stopped - write current values to SID anyway
        sid_write_filter_cutoff(filter->cutoff);
        sid_write_filter_control(filter->control);
        sid_write_filter_mode_volume(filter->type | 0x0F);  // TODO: Use actual master volume
        return;
    }

    // Check if currently modulating
    if (filter->mod_timer > 0) {
        // Continue modulation
        // Get modulation speed from speed table
        int8_t speed = (int8_t)music->filter_speed_table[filter->step_ptr - 1];

        // Apply modulation to cutoff
        int16_t new_cutoff = (int16_t)filter->cutoff + speed;

        // Clamp to 8-bit range (actual filter is 11-bit, but we work with 8-bit here)
        if (new_cutoff < 0) new_cutoff = 0;
        if (new_cutoff > 255) new_cutoff = 255;

        filter->cutoff = (uint8_t)new_cutoff;

        // Decrement timer
        filter->mod_timer--;

        // If timer expired, advance to next step
        if (filter->mod_timer == 0) {
            filter->step_ptr++;
        }

        // Write to SID
        sid_write_filter_cutoff(filter->cutoff);
        return;
    }

    // Not currently modulating - read new command
    uint8_t filter_cmd = music->filter_time_table[filter->step_ptr - 1];

    // Check for cutoff-only set (value 0)
    if (filter_cmd == FILTER_CUTOFF) {
        // Set cutoff directly from speed table
        filter->cutoff = music->filter_speed_table[filter->step_ptr - 1];

        // Advance to next step
        filter->step_ptr++;

        // Write to SID
        sid_write_filter_cutoff(filter->cutoff);
        return;
    }

    // Check for loop command
    if (filter_cmd == FILTER_LOOP) {
        // Loop - jump to target position
        uint8_t target = music->filter_speed_table[filter->step_ptr - 1];
        filter->step_ptr = target;
        return;
    }

    // Check for set command (value >= 128)
    if (filter_cmd >= FILTER_SET) {
        // Set filter parameters
        // Extract passband type from command (shift left 1 to get bit position)
        filter->type = (filter_cmd << 1);

        // Get control byte from speed table (routing and resonance)
        filter->control = music->filter_speed_table[filter->step_ptr - 1];

        // Check if cutoff set follows immediately
        filter->step_ptr++;
        uint8_t next_cmd = music->filter_time_table[filter->step_ptr - 1];

        if (next_cmd == FILTER_CUTOFF) {
            // Cutoff set follows - read it
            filter->cutoff = music->filter_speed_table[filter->step_ptr - 1];
            filter->step_ptr++;
        }

        // Write to SID
        sid_write_filter_cutoff(filter->cutoff);
        sid_write_filter_control(filter->control);
        sid_write_filter_mode_volume(filter->type | 0x0F);  // TODO: Use actual master volume
        return;
    }

    // Start new modulation
    // Duration is in time table, speed is in speed table
    filter->mod_timer = filter_cmd;

    // Don't advance ptr yet - we'll do that when modulation completes
}

//=============================================================================
// End of filtertable.c
//=============================================================================
