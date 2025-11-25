//=============================================================================
// PLAYER3_INTERNAL.H - Internal Functions for Testing and Debugging
//=============================================================================
// This header exposes internal player functions that are not part of the
// public API. These functions are used for:
// - Unit testing individual components
// - Debugging
//
// DO NOT include this header in normal application code. Use player3.h instead.
//=============================================================================

#ifndef PLAYER3_INTERNAL_H
#define PLAYER3_INTERNAL_H

#include "player3_types.h"

//=============================================================================
// SEQUENCER FUNCTIONS
//=============================================================================

/**
 * Fetch the next pattern from the order list.
 *
 * Called when the pattern pointer is 0 (end of pattern or initial state).
 * Handles order list commands: TRANSPOSE, REPEAT, LOOP.
 *
 * @param ch Channel state to update
 * @param music Music data containing order lists
 */
void sequencer_fetch_pattern(Channel* ch, const MusicData* music);

/**
 * Fetch the next note from the current pattern.
 *
 * Called when the tick counter reaches the gate timer threshold.
 * Parses pattern data and updates channel state with:
 * - New instrument (if specified)
 * - New note (if specified)
 * - New effect and parameter (if specified)
 *
 * @param ch Channel state to update
 * @param music Music data containing pattern data
 */
void sequencer_fetch_note(Channel* ch, const MusicData* music);

//=============================================================================
// EFFECT FUNCTIONS
//=============================================================================

/**
 * Execute tick 0 effects.
 *
 * Tick 0 effects are one-shot effects that execute when a note starts.
 * Examples: set ADSR, set waveform, set table pointers.
 *
 * @param ch Channel state
 * @param music Music data
 */
void execute_tick0_effect(Channel* ch, const MusicData* music);

/**
 * Execute continuous effects.
 *
 * Continuous effects execute every tick (except tick 0) to modify
 * pitch or other parameters over time.
 * Examples: portamento, vibrato, tone portamento.
 *
 * @param ch Channel state
 * @param music Music data
 */
void execute_continuous_effect(Channel* ch, const MusicData* music);

//=============================================================================
// TABLE EXECUTION FUNCTIONS
//=============================================================================

/**
 * Execute wavetable for a channel.
 *
 * Updates waveform and applies pitch offsets based on wavetable data.
 *
 * @param ch Channel state
 * @param music Music data containing wavetable
 */
void execute_wavetable(Channel* ch, const MusicData* music);

/**
 * Execute pulse table for a channel.
 *
 * Updates pulse width based on pulse table data.
 *
 * @param ch Channel state
 * @param music Music data containing pulse table
 */
void execute_pulsetable(Channel* ch, const MusicData* music);

/**
 * Execute filter table (global filter modulation).
 *
 * @param player Player state containing filter state
 * @param music Music data containing filter table
 */
void execute_filtertable(Player* player, const MusicData* music);

//=============================================================================
// SOUND EFFECT FUNCTIONS
//=============================================================================

/**
 * Execute sound effect for a channel.
 *
 * Called when a sound effect is active on a channel.
 * Updates the channel's SID registers based on SFX data.
 *
 * @param ch Channel state
 * @param music Music data (for frequency table)
 * @param channel_num Channel number (0-2) for SID writes
 */
void execute_soundfx(Channel* ch, const MusicData* music, uint8_t channel_num);

#endif // PLAYER3_INTERNAL_H
