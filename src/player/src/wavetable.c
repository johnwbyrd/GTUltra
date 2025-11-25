//=============================================================================
// WAVETABLE.C - Wavetable Interpreter
//=============================================================================
// Wavetables control automatic waveform and pitch changes over time.
//
// WAVETABLE FORMAT:
// - Values 0-15: Delay (wait N frames before changing waveform)
// - Values 16+: Waveform to set (subtract 16 to get actual waveform)
// - Values 0xE0-0xEF: Wavetable commands (not yet implemented)
// - Value 0xFF: Loop to position (next byte is target position)
//
// ASSOCIATED NOTE TABLE:
// Each wavetable entry has a corresponding note table entry:
// - Value 0: No frequency change
// - Value 1-127: Absolute note number
// - Value 128-255: Relative note offset (signed, subtract 128)
//
// This allows wavetables to create arpeggios, pitch sweeps, and
// complex melodic patterns automatically.
//=============================================================================

#include "../include/player.h"
#include "../include/player_types.h"

/**
 * Execute wavetable for one frame.
 *
 * Processes one step of the wavetable sequence, potentially changing
 * the waveform and/or pitch. Handles delays, loops, and note offsets.
 */
void execute_wavetable(Channel* ch, const MusicData* music) {
    // Check if wavetable is active
    if (ch->wave_ptr == 0) {
        return;  // Wavetable stopped
    }

    // Get wavetable command at current position
    // Subtract 1 because wave_ptr is 1-based
    uint8_t wave_cmd = music->wave_table[ch->wave_ptr - 1];

    // Check for delay (values 0-15)
    // Delays pause waveform changes for N frames
    if (wave_cmd < 16) {
        // Check if delay is complete
        if (wave_cmd == ch->wave_timer) {
            // Delay complete - advance to next wavetable entry
            ch->wave_ptr++;
            ch->wave_timer = 0;
        } else {
            // Still delaying - increment timer and return
            ch->wave_timer++;
        }
        return;
    }

    // Check for wavetable commands (0xE0-0xEF)
    // These are special commands that can trigger effects
    if (wave_cmd >= 0xE0 && wave_cmd < 0xFF) {
        // TODO: Implement wavetable commands
        // For now, just skip them
        ch->wave_ptr++;
        return;
    }

    // Normal waveform value - subtract 16 to get actual waveform
    if (wave_cmd < 0xE0) {
        ch->waveform = wave_cmd - 16;
    }

    // Get corresponding note offset from note table
    int8_t note_offset = music->note_table[ch->wave_ptr - 1];

    // Check for loop command in next position
    uint8_t next_cmd = music->wave_table[ch->wave_ptr];
    if (next_cmd == WAVE_LOOP) {
        // Loop - jump to target position
        uint8_t target = music->note_table[ch->wave_ptr];
        ch->wave_ptr = target;
    } else {
        // Advance to next entry
        ch->wave_ptr++;
    }

    // Reset delay timer
    ch->wave_timer = 0;

    // Apply note offset if non-zero
    if (note_offset != 0) {
        uint8_t new_note;

        if (note_offset < 0) {
            // Relative offset - add to current note
            int16_t result = ch->note + note_offset;
            if (result < FIRST_NOTE) result = FIRST_NOTE;
            if (result > LAST_NOTE) result = LAST_NOTE;
            new_note = (uint8_t)result;
        } else {
            // Absolute note number
            new_note = note_offset;
            if (new_note > LAST_NOTE) new_note = LAST_NOTE;
        }

        // Update note and frequency
        ch->note = new_note;
        ch->frequency = music->freq_table_lo[new_note] |
                       ((sid_freq_t)music->freq_table_hi[new_note] << 8);

        // Reset vibrato phase when changing frequency
        ch->vibrato_phase = 0;
    }
}

//=============================================================================
// End of wavetable.c
//=============================================================================
