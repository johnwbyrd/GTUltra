//=============================================================================
// PULSETABLE.C - Pulse Table Interpreter
//=============================================================================
// Pulse tables control automatic pulse width modulation (PWM) over time.
//
// PULSE TABLE FORMAT:
// - Values 1-127: Modulation duration (run for N frames)
// - Values 128+: Set pulse directly (high byte of pulse width)
// - Value 255: Loop to position (next byte is target position)
//
// ASSOCIATED PULSE SPEED TABLE:
// - For modulation: Speed value to add/subtract each frame
// - For set command: Low byte of pulse width
//
// Pulse width modulation creates rich, evolving timbres by automatically
// changing the duty cycle of the pulse waveform over time.
//=============================================================================

#include "../include/player.h"
#include "../include/player_types.h"

/**
 * Execute pulse table for one frame.
 *
 * Processes one step of the pulse table sequence, potentially modulating
 * the pulse width or setting it to a new value.
 */
void execute_pulsetable(Channel* ch, const MusicData* music) {
    // Check if pulse table is active
    if (ch->pulse_ptr == 0) {
        return;  // Pulse table stopped
    }

    // Check if currently modulating
    if (ch->pulse_timer > 0) {
        // Continue modulation
        // Get modulation speed from speed table
        int8_t speed = (int8_t)music->pulse_speed_table[ch->pulse_ptr - 1];

        // Apply modulation to pulse width
        // Pulse width is 12-bit (0-4095), but we work with 16-bit for overflow
        int32_t new_pulse = (int32_t)ch->pulse_width + speed;

        // Clamp to 12-bit range
        if (new_pulse < 0) new_pulse = 0;
        if (new_pulse > 4095) new_pulse = 4095;

        ch->pulse_width = (sid_pulse_t)new_pulse;

        // Decrement timer
        ch->pulse_timer--;

        // If timer expired, advance to next step
        if (ch->pulse_timer == 0) {
            ch->pulse_ptr++;
        }

        return;
    }

    // Not currently modulating - read new command
    uint8_t pulse_cmd = music->pulse_time_table[ch->pulse_ptr - 1];

    // Check for loop command
    if (pulse_cmd == PULSE_LOOP) {
        // Loop - jump to target position
        uint8_t target = music->pulse_speed_table[ch->pulse_ptr - 1];
        ch->pulse_ptr = target;
        return;
    }

    // Check for set command (value >= 128)
    if (pulse_cmd >= PULSE_SET) {
        // Set pulse width directly
        // High byte from time table, low byte from speed table
        uint8_t pulse_hi = pulse_cmd;
        uint8_t pulse_lo = music->pulse_speed_table[ch->pulse_ptr - 1];

        ch->pulse_width = pulse_lo | ((sid_pulse_t)pulse_hi << 8);
        ch->pulse_width &= 0x0FFF;  // Clamp to 12-bit

        // Advance to next step
        ch->pulse_ptr++;
        return;
    }

    // Start new modulation
    // Duration is in time table, speed is in speed table
    ch->pulse_timer = pulse_cmd;

    // Don't advance ptr yet - we'll do that when modulation completes
}

//=============================================================================
// End of pulsetable.c
//=============================================================================
