//=============================================================================
// EFFECTS.C - Effect Handlers
//=============================================================================
// This file implements all 16 musical effects supported by the player:
//
// TICK 0 EFFECTS (execute once when note starts):
// - FX_SETAD (5): Set Attack/Decay
// - FX_SETSR (6): Set Sustain/Release
// - FX_SETWAVE (7): Set waveform
// - FX_SETWAVEPTR (8): Set wavetable pointer
// - FX_SETPULSEPTR (9): Set pulse table pointer
// - FX_SETFILTPTR (A): Set filter table pointer
// - FX_SETFILTCTRL (B): Set filter control
// - FX_SETFILTCUTOFF (C): Set filter cutoff
// - FX_SETMASTERVOL (D): Set master volume
// - FX_SETFUNKTEMPO (E): Set funk tempo
// - FX_SETTEMPO (F): Set tempo
//
// CONTINUOUS EFFECTS (execute every tick):
// - FX_ARPEGGIO (0): Instrument vibrato/arpeggio
// - FX_PORTAUP (1): Pitch slide up
// - FX_PORTADOWN (2): Pitch slide down
// - FX_TONEPORTA (3): Slide to target note
// - FX_VIBRATO (4): Oscillating pitch
//=============================================================================

#include "../include/player3.h"
#include "../include/player3_types.h"
#include "../include/sid.h"

//=============================================================================
// TICK 0 EFFECT HANDLERS
//=============================================================================

/**
 * Execute tick 0 effects.
 *
 * Tick 0 effects are executed once when a note starts. They typically
 * set up parameters (waveform, ADSR, table pointers, etc.) that affect
 * how the note will sound.
 */
void execute_tick0_effect(Channel* ch, const MusicData* music) {
    (void)music;  // Unused for now - will be needed for speed table lookups

    uint8_t effect = ch->new_effect;
    uint8_t param = ch->new_param;

    // Dispatch to appropriate handler based on effect number
    switch (effect) {
        case FX_ARPEGGIO:
            // Instrument vibrato - handled in continuous effects
            // Just store the parameters here
            ch->effect = effect;
            ch->effect_param = param;
            break;

        case FX_PORTAUP:
        case FX_PORTADOWN:
            // Portamento - reset vibrato when starting portamento
            ch->vibrato_phase = 0;
            ch->effect = effect;
            ch->effect_param = param;
            break;

        case FX_TONEPORTA:
        case FX_VIBRATO:
            // Store effect and parameter for continuous execution
            ch->effect = effect;
            ch->effect_param = param;
            break;

        case FX_SETAD:
            // Set Attack/Decay envelope
            ch->attack_decay = param;
            break;

        case FX_SETSR:
            // Set Sustain/Release envelope
            ch->sustain_release = param;
            break;

        case FX_SETWAVE:
            // Set waveform immediately
            ch->waveform = param;
            break;

        case FX_SETWAVEPTR:
            // Set wavetable pointer
            ch->wave_ptr = param;
            ch->wave_timer = 0;  // Reset delay timer
            break;

        case FX_SETPULSEPTR:
            // Set pulse table pointer
            ch->pulse_ptr = param;
            ch->pulse_timer = 0;  // Reset modulation timer
            break;

        case FX_SETFILTPTR:
            // Set filter table pointer (stub - will be implemented)
            // TODO: Implement filter table pointer setting
            break;

        case FX_SETFILTCTRL:
            // Set filter control (stub - will be implemented)
            // TODO: Implement filter control setting
            break;

        case FX_SETFILTCUTOFF:
            // Set filter cutoff (stub - will be implemented)
            // TODO: Implement filter cutoff setting
            break;

        case FX_SETMASTERVOL:
            // Set master volume or timing mark
            // Values 0-15: Set master volume
            // Values 16+: Timing mark for synchronization (not implemented)
            if (param < 16) {
                // Set master volume (will be written to SID in filter execution)
                // TODO: Implement master volume setting via filter
            }
            break;

        case FX_SETFUNKTEMPO:
            // Set funk tempo (stub - will be implemented)
            // TODO: Implement funk tempo setting
            break;

        case FX_SETTEMPO:
            // Set tempo
            if (param < 0x80) {
                // Global tempo - set same tempo for all channels
                // TODO: Access to all channels needed - implement in player3.c
            } else {
                // Per-channel tempo
                ch->tempo = param & 0x7F;
            }
            break;

        default:
            // Unknown effect - do nothing
            break;
    }
}

//=============================================================================
// CONTINUOUS EFFECT HANDLERS
//=============================================================================

/**
 * Execute continuous effects.
 *
 * Continuous effects run every tick (not just tick 0) and modify the
 * channel's frequency or other parameters in real-time. This creates
 * smooth pitch changes like vibrato or portamento.
 */
void execute_continuous_effect(Channel* ch, const MusicData* music) {
    (void)music;  // Unused for now - will be needed for speed table lookups

    // Continuous effects only run on ticks > 0
    // (Tick 0 is reserved for fetching new notes)
    if (ch->tick_counter == 0) {
        return;
    }

    uint8_t effect = ch->effect;

    // Dispatch to appropriate handler
    switch (effect) {
        case FX_ARPEGGIO:
            // Instrument vibrato/arpeggio
            // TODO: Implement instrument vibrato
            break;

        case FX_PORTAUP:
            // Portamento up - increase frequency every tick
            // TODO: Implement portamento up with speed calculation
            break;

        case FX_PORTADOWN:
            // Portamento down - decrease frequency every tick
            // TODO: Implement portamento down with speed calculation
            break;

        case FX_TONEPORTA:
            // Tone portamento - slide to target note
            // TODO: Implement tone portamento
            break;

        case FX_VIBRATO:
            // Vibrato - oscillating pitch
            // TODO: Implement vibrato
            break;

        default:
            // No continuous effect - do nothing
            break;
    }
}

//=============================================================================
// End of effects.c
//=============================================================================
