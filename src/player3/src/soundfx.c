//=============================================================================
// SOUNDFX.C - Sound Effect Player
//=============================================================================
// Sound effects temporarily override normal music playback on a channel.
//
// SFX FORMAT:
//   Byte 0: Attack/Decay value
//   Byte 1: Sustain/Release value
//   Byte 2: Pulse width (8-bit, used for both lo and hi bytes)
//   Byte 3+: Note values ($80-$FF) or waveforms ($00-$81)
//   End: $00
//
// PRIORITY SYSTEM:
// Sound effects use address-based priority. A SFX will only start if its
// address is higher than any currently playing SFX on that channel. This
// allows important sounds (explosions) to override less important sounds
// (footsteps) when placed at higher addresses in memory.
//
// EXECUTION:
// - Frame 0: Hard restart (set AD=0, SR=0 to reset envelope)
// - Frame 1: Load ADSR and pulse, set test bit
// - Frame 2+: Read notes and waveforms until $00 terminator
// - On end: Return to normal music playback
//=============================================================================

#include "../include/player3.h"
#include "../include/player3_types.h"
#include "../include/sid.h"

/**
 * Play a sound effect on a channel.
 *
 * Initializes SFX playback with priority checking. The SFX will only
 * start if its address is higher than any currently playing SFX.
 */
void player_play_sfx(Player* player, const MusicData* music,
                     uint8_t channel, const uint8_t* sfx_data) {
    Channel* ch = &player->channels[channel];

    // Priority check - only play if address is higher
    if (ch->sfx_frame != 0) {
        // SFX already playing - check priority
        if (sfx_data <= ch->sfx_data) {
            // Lower or equal priority - don't play
            return;
        }
    }

    // Start new SFX
    ch->sfx_frame = 1;  // Frame counter starts at 1
    ch->sfx_data = sfx_data;
}

/**
 * Execute sound effect for one frame.
 *
 * Advances the SFX playback, reading notes and waveforms from the
 * SFX data stream. Handles hard restart and envelope setup.
 */
void execute_soundfx(Channel* ch, const MusicData* music, uint8_t channel_num) {
    const uint8_t* sfx = ch->sfx_data;
    uint8_t frame = ch->sfx_frame;

    // Turn gate off and stop wavetable
    ch->gate = 0xFE;
    ch->wave_ptr = 0;

    // Increment frame counter for next time
    ch->sfx_frame++;

    // Handle different frames
    if (frame == 1) {
        // Frame 0: Hard restart
        // Set AD=0, SR=0 to reset envelope generator
        sid_write_adsr(channel_num, 0, 0);

        // Also load frequency (will be written in frame 1)
        return;
    }

    if (frame == 2) {
        // Frame 1: Load ADSR and pulse
        uint8_t ad = sfx[0];
        uint8_t sr = sfx[1];
        uint8_t pulse = sfx[2];

        // Write ADSR
        ch->attack_decay = ad;
        ch->sustain_release = sr;
        sid_write_adsr(channel_num, ad, sr);

        // Write pulse (use same value for lo and hi bytes)
        ch->pulse_width = pulse | ((sid_pulse_t)pulse << 8);
        sid_write_pulse(channel_num, ch->pulse_width);

        // Set test bit (waveform $09 = gate + test)
        ch->waveform = SID_WAVE_TEST | SID_WAVE_GATE;
        sid_write_control(channel_num, ch->waveform, ch->gate);

        return;
    }

    // Frame 2+: Read notes and waveforms
    uint8_t note = sfx[frame - 1];

    // Check for end of SFX ($00)
    if (note == 0) {
        // End SFX - return to normal music
        ch->sfx_frame = 0;
        ch->sfx_data = NULL;

        // Set test bit to stop oscillator
        ch->waveform = SID_WAVE_TEST | SID_WAVE_GATE;
        sid_write_control(channel_num, ch->waveform, ch->gate);

        return;
    }

    // Read note (values $80-$FF)
    // Note values are offset by $80, so subtract to get index into freq table
    uint8_t note_idx = note - 0x80;
    ch->frequency = music->freq_table_lo[note_idx] |
                   ((sid_freq_t)music->freq_table_hi[note_idx] << 8);
    sid_write_freq(channel_num, ch->frequency);

    // Check if waveform follows
    uint8_t next = sfx[frame];

    if (next == 0 || next >= 0x82) {
        // No waveform follows (either end or another note)
        return;
    }

    // Waveform follows - read it
    ch->waveform = next;
    ch->sfx_frame++;  // Skip waveform byte next time
    sid_write_control(channel_num, ch->waveform, ch->gate);
}

//=============================================================================
// End of soundfx.c
//=============================================================================
