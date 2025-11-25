//=============================================================================
// SEQUENCER.C - Pattern and Order List Processing
//=============================================================================
// This file handles the song structure sequencing:
// - Order list processing (which patterns play in what order)
// - Pattern data decoding (compressed note/effect data)
// - Note fetching and parsing
//
// The sequencer is responsible for stepping through the song structure
// and extracting musical information (notes, instruments, effects) from
// the compressed pattern data.
//
// PATTERN DATA FORMAT:
// Patterns are compressed byte streams with variable-length commands:
// - 0x00-0x3F: Instrument change (followed by FX or NOTE)
// - 0x40-0x4F: Effect + parameter + note
// - 0x50-0x5F: Effect + parameter only (no note)
// - 0x60-0xBC: Note value
// - 0xBD: Rest
// - 0xBE: Key off
// - 0xBF: Key on
// - 0xC0-0xFE: Packed rest (rest for N frames)
// - 0x00: End of pattern (when pattern_ptr != 0)
//=============================================================================

#include "../include/player3.h"
#include "../include/player3_types.h"

//=============================================================================
// HELPER FUNCTIONS
//=============================================================================

/**
 * Read a 16-bit pattern pointer from the pattern table.
 *
 * Pattern pointers are stored as two separate tables (low and high bytes).
 * This function combines them into a 16-bit address.
 */
static inline const uint8_t* read_pattern_pointer(const MusicData* music, uint8_t pattern_num) {
    uint16_t addr = music->pattern_table_lo[pattern_num] |
                    ((uint16_t)music->pattern_table_hi[pattern_num] << 8);
    return (const uint8_t*)addr;
}

/**
 * Get note frequency from frequency table.
 *
 * Combines low and high bytes from the frequency tables.
 */
static inline sid_freq_t read_frequency(const MusicData* music, uint8_t note) {
    return music->freq_table_lo[note] |
           ((sid_freq_t)music->freq_table_hi[note] << 8);
}

//=============================================================================
// ORDER LIST PROCESSING
//=============================================================================

/**
 * Fetch next pattern from order list.
 *
 * The order list defines which patterns play in what order. It can contain:
 * - Pattern numbers (0x00-0xCF): Play this pattern
 * - REPEAT commands (0xD0+): Repeat pattern N times
 * - TRANSPOSE commands (0xE0-0xFE): Transpose following patterns
 * - LOOP command (0xFF): Jump to position in order list
 *
 * This function advances through the order list and sets up the channel
 * to play the next pattern.
 */
void sequencer_fetch_pattern(Channel* ch, const MusicData* music) {
    // Get the order list for this channel
    // Note: For multi-song files, each song has 3 order lists (one per channel)
    const uint8_t* order_list = music->order_lists[ch->song_ptr];

    // Read current position in order list
    uint8_t pos = ch->song_ptr;
    uint8_t command = order_list[pos];

    // Handle LOOP command
    // Format: 0xFF <position>
    // Jumps to the specified position in the order list
    if (command == ORDER_LOOP) {
        pos++;  // Skip loop command
        uint8_t loop_target = order_list[pos];  // Get target position
        pos = loop_target;  // Jump to target
        command = order_list[pos];  // Read command at new position
    }

    // Handle TRANSPOSE commands
    // Format: 0xE0-0xFE <pattern>
    // Transposes all following notes by a signed offset
    // 0xE0-0xEF: Transpose down (-16 to -1 semitones)
    // 0xF0-0xFE: Transpose up (0 to +14 semitones)
    if (command >= ORDER_TRANSDOWN) {
        // Extract transpose amount
        // For TRANSDOWN (0xE0-0xEF): command - 0xF0 = -16 to -1
        // For TRANSUP (0xF0-0xFE): command - 0xF0 = 0 to +14
        int8_t transpose = (int8_t)(command - ORDER_TRANS);
        ch->transpose = transpose;

        // Move to next byte (the pattern number)
        pos++;
        command = order_list[pos];
    }

    // Handle REPEAT commands
    // Format: 0xD0-0xDF
    // Repeats the current pattern N times before advancing
    // The repeat count is stored in the channel's repeat_count variable
    if (command >= ORDER_REPEAT) {
        // Extract repeat count from command byte
        // 0xD0 = repeat once (play 2 times total)
        // 0xD1 = repeat twice (play 3 times total)
        // etc.
        uint8_t repeat_max = command - ORDER_REPEAT;

        // Increment our repeat counter
        ch->repeat_count++;

        // Check if we've repeated enough times
        if (ch->repeat_count <= repeat_max) {
            // Not done repeating yet - stay on same pattern
            // Don't advance pos, don't change pattern_num
            return;
        }

        // Done repeating - reset counter and move to next pattern
        ch->repeat_count = 0;
        pos++;
        command = order_list[pos];
    }

    // At this point, command should be a pattern number (0x00-0xCF)
    ch->pattern_num = command;

    // Advance order list position for next time
    pos++;
    ch->song_ptr = pos;

    // Reset pattern pointer to start of pattern
    // Setting this to 0 signals that we need to start reading pattern data
    ch->pattern_ptr = 1;  // Start at position 1 (position 0 is read first)
}

//=============================================================================
// PATTERN DATA DECODING
//=============================================================================

/**
 * Fetch next note from pattern data.
 *
 * This decodes the compressed pattern data stream and extracts:
 * - Instrument changes
 * - Effect commands and parameters
 * - Note values
 * - Rest/gate commands
 *
 * The pattern data is a variable-length byte stream with different
 * command formats depending on the first byte value.
 */
void sequencer_fetch_note(Channel* ch, const MusicData* music) {
    // Get pointer to current pattern data
    const uint8_t* pattern = read_pattern_pointer(music, ch->pattern_num);

    // Get current position in pattern
    uint8_t pos = ch->pattern_ptr;

    // Read first byte of command
    uint8_t data = pattern[pos];

    // Check for INSTRUMENT CHANGE (0x00-0x3F)
    // Format: <instrument> <FX or NOTE>
    // Changes the current instrument, followed by either an effect or note
    if (data <= PATTERN_INSTR_MAX) {
        // Store new instrument number (1-based)
        ch->instrument = data;

        // Load instrument parameters
        // Subtract 1 because instrument arrays are 0-based but instruments are 1-based
        uint8_t instr_idx = data - 1;

        // Load gate timer (when to release the note)
        ch->gate_timer = music->instr_gatetimer[instr_idx];

        // Move to next byte (FX or NOTE)
        pos++;
        data = pattern[pos];
    }

    // Check for EFFECT COMMAND (0x40-0x5F)
    // Two formats:
    // - 0x40-0x4F: Effect with parameter, note follows
    // - 0x50-0x5F: Effect with parameter, no note (effect only)
    if (data >= PATTERN_FX && data < PATTERN_NOTE) {
        // Determine if note follows
        bool note_follows = (data < PATTERN_FXONLY);

        // Extract effect number (lower 4 bits)
        uint8_t effect_num = data & 0x0F;
        ch->new_effect = effect_num;

        // If effect is not 0, read parameter byte
        if (effect_num != 0) {
            pos++;
            ch->new_param = pattern[pos];
        } else {
            // Effect 0 (arpeggio/vibrato) has no parameter byte
            ch->new_param = 0;
        }

        // If note follows, read it
        if (note_follows) {
            pos++;
            data = pattern[pos];
            // Fall through to note handling below
        } else {
            // Effect only, no note - advance and return
            pos++;

            // Check for end of pattern
            if (pattern[pos] == PATTERN_END) {
                pos = 0;  // Signal sequencer to fetch new pattern
            }

            ch->pattern_ptr = pos;
            return;
        }
    }

    // Check for PACKED REST (0xC0-0xFE)
    // Format: <count>
    // Rests for multiple frames (saves space)
    // Count is stored and decremented each frame
    if (data >= PATTERN_PACKED_REST && data != 0xFF) {
        // Check if we're starting a new packed rest
        if (ch->packed_rest == 0) {
            // Start new packed rest
            // The value encodes how many frames to rest
            ch->packed_rest = data - PATTERN_PACKED_REST + 1;
        }

        // Decrement packed rest counter
        ch->packed_rest--;

        // If still resting, don't advance pattern pointer
        if (ch->packed_rest > 0) {
            return;
        }

        // Packed rest complete - advance to next command
        pos++;

        // Check for end of pattern
        if (pattern[pos] == PATTERN_END) {
            pos = 0;  // Signal sequencer to fetch new pattern
        }

        ch->pattern_ptr = pos;
        return;
    }

    // Check for NOTE (0x60-0xBC)
    // Format: <note_value>
    // Triggers a new note with the current instrument
    if (data >= PATTERN_NOTE && data < PATTERN_REST) {
        // Extract note value
        uint8_t note = data - PATTERN_NOTE;

        // Apply transpose
        int16_t transposed = note + ch->transpose;

        // Clamp to valid range
        if (transposed < FIRST_NOTE) transposed = FIRST_NOTE;
        if (transposed > LAST_NOTE) transposed = LAST_NOTE;

        // Store as new note to be initialized
        ch->new_note = PATTERN_NOTE + (uint8_t)transposed;
        ch->note = (uint8_t)transposed;

        // Load frequency from table
        ch->frequency = read_frequency(music, ch->note);

        // Check for hard restart
        // If instrument is in the "hard restart" range, we need to
        // briefly reset the SID envelope for a percussive sound
        // (This will be implemented in the note initialization code)

        // Turn gate off briefly for hard restart, then on
        ch->gate = 0xFE;  // Will be set to 0xFF in note init

        // Advance to next command
        pos++;

        // Check for end of pattern
        if (pattern[pos] == PATTERN_END) {
            pos = 0;  // Signal sequencer to fetch new pattern
        }

        ch->pattern_ptr = pos;
        return;
    }

    // Check for REST (0xBD)
    // Silence - no note plays
    if (data == PATTERN_REST) {
        // Rest - do nothing, just advance
        pos++;

        // Check for end of pattern
        if (pattern[pos] == PATTERN_END) {
            pos = 0;  // Signal sequencer to fetch new pattern
        }

        ch->pattern_ptr = pos;
        return;
    }

    // Check for KEY OFF (0xBE)
    // Turn gate off without changing note
    if (data == PATTERN_KEYOFF) {
        ch->gate = 0xFE;  // Gate off

        // Advance to next command
        pos++;

        // Check for end of pattern
        if (pattern[pos] == PATTERN_END) {
            pos = 0;  // Signal sequencer to fetch new pattern
        }

        ch->pattern_ptr = pos;
        return;
    }

    // Check for KEY ON (0xBF)
    // Turn gate on without changing note
    if (data == PATTERN_KEYON) {
        ch->gate = 0xFF;  // Gate on

        // Advance to next command
        pos++;

        // Check for end of pattern
        if (pattern[pos] == PATTERN_END) {
            pos = 0;  // Signal sequencer to fetch new pattern
        }

        ch->pattern_ptr = pos;
        return;
    }

    // If we get here, something unexpected happened
    // This might be end of pattern (0x00) or invalid data
    // In either case, signal to fetch a new pattern
    ch->pattern_ptr = 0;
}

//=============================================================================
// End of sequencer.c
//=============================================================================
