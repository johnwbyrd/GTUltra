//=============================================================================
// PLAYER3.C - Main Player Loop and Initialization
//=============================================================================
// This file contains the core player logic:
// - Initialization (player_init)
// - Main playback loop (player_play)
// - Channel execution
// - SID register writing
//
// The main loop is called once per frame (50Hz PAL / 60Hz NTSC) and:
// 1. Completes pending initialization if needed
// 2. Executes the global filter modulation
// 3. Executes all three channels independently
// 4. Writes final values to SID registers
//=============================================================================

#include "../include/player3.h"
#include "../include/player3_types.h"
#include "../include/sid.h"

//=============================================================================
// GLOBAL SID POINTER
//=============================================================================

// Pointer to SID chip registers at $D400
// This is the hardware interface - writing to this struct writes to the SID
volatile SID_Chip* const sid = (SID_Chip*)SID_BASE_ADDRESS;

//=============================================================================
// FORWARD DECLARATIONS
//=============================================================================

// Internal functions (defined in other modules)
extern void sequencer_fetch_pattern(Channel* ch, const MusicData* music);
extern void sequencer_fetch_note(Channel* ch, const MusicData* music);
extern void execute_tick0_effect(Channel* ch, const MusicData* music);
extern void execute_continuous_effect(Channel* ch, const MusicData* music);
extern void execute_wavetable(Channel* ch, const MusicData* music);
extern void execute_pulsetable(Channel* ch, const MusicData* music);
extern void execute_filtertable(FilterState* filter, const MusicData* music);
extern void execute_soundfx(Channel* ch, const MusicData* music, uint8_t channel_num);

// Internal functions (defined in this file)
static void complete_initialization(Player* player, const MusicData* music);
static void execute_channel(Player* player, const MusicData* music, uint8_t channel_num);
static void update_tick_counter(Channel* ch, const uint8_t* funk_tempo);
static void write_sid_registers(const Channel* ch, uint8_t channel_num);
static void init_channel(Channel* ch, const MusicData* music, uint8_t song_index);

//=============================================================================
// PUBLIC API IMPLEMENTATION
//=============================================================================

/**
 * Initialize the player with a song.
 *
 * This marks the song for initialization, which will be completed on the
 * next call to player_play(). This deferred initialization allows smooth
 * transitions between songs during playback.
 */
void player_init(Player* player, const MusicData* music, uint8_t song_num) {
    (void)music;  // Unused - music data accessed later in player_play()

    // Store the song number as a negative value to trigger initialization
    // in player_play(). Using negative values allows us to encode the song
    // number (0-31) while still having a clear "needs init" flag.
    player->init_song_num = -(int8_t)(song_num + 1);
    player->song_num = song_num;
}

/**
 * Main playback routine - call once per frame.
 *
 * This is the heart of the player. Must be called at exactly 50Hz (PAL)
 * or 60Hz (NTSC) for correct timing.
 */
void player_play(Player* player, const MusicData* music) {
    // Check if initialization is pending
    if (player->init_song_num < 0) {
        complete_initialization(player, music);
    }

    // Execute global filter modulation (shared by all channels)
    execute_filtertable(&player->filter, music);

    // Execute all three channels independently
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        execute_channel(player, music, i);
    }
}

/**
 * Set master volume.
 *
 * Updates the master volume, which affects all voices. The actual SID
 * write happens in player_play() when the filter mode/volume register
 * is written.
 */
void player_set_master_volume(Player* player, uint8_t volume) {
    // Clamp to 0-15 (only lower 4 bits are used)
    player->master_volume = volume & 0x0F;
}

//=============================================================================
// INITIALIZATION
//=============================================================================

/**
 * Complete pending initialization.
 *
 * This resets all player state and prepares for playback of the specified
 * song. Called automatically by player_play() when init_song_num is negative.
 */
static void complete_initialization(Player* player, const MusicData* music) {
    // Extract song number from negative init value
    uint8_t song_num = (uint8_t)(-(player->init_song_num + 1));

    // Reset all channel state to zero
    // This clears all runtime variables: counters, pointers, effects, etc.
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        Channel* ch = &player->channels[i];

        // Zero out the entire channel structure
        // Note: In C, this is cleaner than assembly's loop
        *ch = (Channel){0};
    }

    // Reset global filter state
    player->filter.step_ptr = 0;
    player->filter.mod_timer = 0;
    player->filter.cutoff = 0;
    player->filter.control = 0;
    player->filter.type = 0;

    // Reset SID filter registers
    // Writing 0 to these disables the filter and resets cutoff
    sid->filter_cutoff_lo = 0;
    sid->filter_cutoff_hi = 0;
    sid->filter_control = 0;
    sid->filter_mode_volume = player->master_volume;  // Volume only, no filter

    // Initialize default funk tempo table
    // These values (8, 5) create a shuffle/swing feel when funk tempo is used
    player->funk_tempo[0] = 8;
    player->funk_tempo[1] = 5;

    // Initialize each channel with song-specific data
    // For player3, we have 3 channels using 3 consecutive order lists
    uint8_t song_index = song_num * NUM_CHANNELS;  // Offset into order list table

    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        init_channel(&player->channels[i], music, song_index + i);
    }

    // Clear the init flag (set to 0, not negative)
    player->init_song_num = 0;
}

/**
 * Initialize a single channel.
 *
 * Sets up the channel with default values and loads the initial waveform
 * from instrument 1 (the default instrument).
 */
static void init_channel(Channel* ch, const MusicData* music, uint8_t song_index) {
    // Set default tempo (6 ticks per note is typical)
    ch->tempo = DEFAULT_TEMPO;

    // Set tick counter to 1 so we immediately fetch a note on first play
    ch->tick_counter = 1;

    // Set default instrument to 1 (instruments are 1-based)
    ch->instrument = 1;

    // Gate starts in "off" state (0xFE)
    // When we trigger a note, it becomes 0xFF (on)
    ch->gate = 0xFE;

    // Store which order list this channel uses
    // This is used by the sequencer to fetch patterns
    ch->song_ptr = 0;  // Start at beginning of order list

    // Load initial waveform from instrument 1
    // This ensures we have a valid waveform even before the first note
    uint8_t first_wave = music->instr_firstwave[0];  // Instrument 1 (0-based array)

    if (first_wave != 0 && first_wave < 0xFE) {
        ch->waveform = first_wave;

        // Write waveform to SID immediately
        // We write with gate off (0xFE) so no sound yet
        sid_write_control(song_index % NUM_CHANNELS, ch->waveform, ch->gate);
    }
}

//=============================================================================
// CHANNEL EXECUTION
//=============================================================================

/**
 * Execute one channel for one frame.
 *
 * This is the main per-channel logic that runs every frame. It handles:
 * - Tick counting and tempo
 * - Fetching new notes
 * - Executing effects
 * - Modulating tables (wave/pulse)
 * - Sound effects
 * - Writing to SID
 */
static void execute_channel(Player* player, const MusicData* music, uint8_t channel_num) {
    Channel* ch = &player->channels[channel_num];

    // Update tick counter and check if it's time for a new note
    update_tick_counter(ch, player->funk_tempo);

    // If counter reached zero, it's "tick 0" - time to fetch a new note
    if (ch->tick_counter == 0) {
        // Check if we need to fetch a new pattern from the order list
        if (ch->pattern_ptr == 0) {
            sequencer_fetch_pattern(ch, music);
        }

        // Check if we have a new note to initialize
        if (ch->new_note != 0) {
            // Initialize the new note (load instrument parameters, etc.)
            // This is implemented in the next phase
            // For now, just clear the new_note flag
            ch->new_note = 0;
        }

        // Execute tick 0 effects (effects that run once when note starts)
        execute_tick0_effect(ch, music);
    }

    // Execute wavetable (automatic waveform/pitch changes)
    execute_wavetable(ch, music);

    // Execute continuous effects (portamento, vibrato - run every tick)
    execute_continuous_effect(ch, music);

    // Execute pulse table (pulse width modulation)
    execute_pulsetable(ch, music);

    // Check if it's time to fetch a new note from the pattern
    if (ch->tick_counter == ch->gate_timer) {
        sequencer_fetch_note(ch, music);
    }

    // Check if sound effect is active on this channel
    if (ch->sfx_frame != 0) {
        // Sound effect overrides normal music
        execute_soundfx(ch, music, channel_num);
    } else {
        // Write normal music to SID
        write_sid_registers(ch, channel_num);
    }
}

/**
 * Update tick counter and handle tempo reload.
 *
 * The tick counter counts down from tempo to 0. When it reaches 0, it's
 * time to fetch a new note. After reaching 0, it goes negative and then
 * reloads from tempo on the next frame.
 *
 * Funk tempo is a special mode where the tempo alternates between two
 * values (creating a shuffle/swing feel).
 */
static void update_tick_counter(Channel* ch, const uint8_t* funk_tempo) {
    // Decrement tick counter
    ch->tick_counter--;

    // If counter is still positive, nothing more to do
    if (ch->tick_counter > 0) {
        return;
    }

    // Counter reached 0 or went negative - time to reload

    // Check for funk tempo mode (tempo < 2)
    // Funk tempo alternates between two different speeds for swing feel
    if (ch->tempo < 2) {
        // Use tempo as index into funk tempo table (0 or 1)
        uint8_t funk_index = ch->tempo;
        uint8_t new_tempo = funk_tempo[funk_index];

        // Alternate between indexes 0 and 1 for next time
        ch->tempo ^= 1;  // Toggle bit 0: 0<->1

        // Reload counter with funk tempo value
        ch->tick_counter = new_tempo;
    } else {
        // Normal tempo - just reload from tempo value
        ch->tick_counter = ch->tempo;
    }
}

/**
 * Write channel state to SID registers.
 *
 * This writes the current channel state (frequency, pulse, waveform, ADSR)
 * to the SID chip hardware. The channel_num determines which voice (0-2)
 * to write to.
 */
static void write_sid_registers(const Channel* ch, uint8_t channel_num) {
    // Write all registers using our inline accessor functions
    // These functions handle the proper byte splitting and register offsets

    // Write frequency (16-bit value split across two registers)
    sid_write_freq(channel_num, ch->frequency);

    // Write pulse width (12-bit value split across two registers)
    sid_write_pulse(channel_num, ch->pulse_width);

    // Write waveform with gate bit
    // Gate is 0xFF (on) or 0xFE (off), ANDing clears bit 0 when off
    sid_write_control(channel_num, ch->waveform, ch->gate);

    // Write ADSR envelope
    sid_write_adsr(channel_num, ch->attack_decay, ch->sustain_release);
}

//=============================================================================
// End of player3.c
//=============================================================================
