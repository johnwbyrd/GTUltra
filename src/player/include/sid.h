//=============================================================================
// SID.H - Commodore 64 SID Chip Register Definitions
//=============================================================================
// This file defines the hardware interface to the MOS 6581/8580 SID
// (Sound Interface Device) chip. The SID has 3 independent voices, each
// with its own oscillator, envelope generator, and waveform controls, plus
// a shared programmable filter and master volume control.
//
// The registers are accessed as memory-mapped I/O at $D400-$D418.
//=============================================================================

#ifndef PLAYER_SID_H
#define PLAYER_SID_H

#include <stdint.h>

//-----------------------------------------------------------------------------
// SID CHIP BASE ADDRESS
//-----------------------------------------------------------------------------

// SID chip registers are memory-mapped starting at $D400
#define SID_BASE_ADDRESS 0xD400

//-----------------------------------------------------------------------------
// TYPE DEFINITIONS
//-----------------------------------------------------------------------------

// SID frequency value (16-bit)
// The SID oscillator frequency is calculated as:
//   Frequency = (Register Value × Clock) / 16777216
// For PAL C64: Clock = 985248 Hz
// For NTSC C64: Clock = 1022727 Hz
typedef uint16_t sid_freq_t;

// SID pulse width value (12-bit, stored in 16-bit)
// Only the lower 12 bits are used. Pulse width controls the duty cycle
// of the pulse waveform. Range: 0-4095
// Typical musical values: 2048 (50% square wave), 1024 (25%), 3072 (75%)
typedef uint16_t sid_pulse_t;

//-----------------------------------------------------------------------------
// SID VOICE REGISTER STRUCTURE
//-----------------------------------------------------------------------------

// Each of the 3 voices has 7 registers (offsets $00-$06, $07-$0D, $0E-$14)
typedef struct {
    uint8_t freq_lo;        // +$00 - Frequency low byte
    uint8_t freq_hi;        // +$01 - Frequency high byte
    uint8_t pulse_lo;       // +$02 - Pulse width low byte
    uint8_t pulse_hi;       // +$03 - Pulse width high byte (bits 0-3 only)
    uint8_t control;        // +$04 - Waveform and gate control
    uint8_t attack_decay;   // +$05 - Envelope attack/decay
    uint8_t sustain_release;// +$06 - Envelope sustain/release
} SID_Voice;

//-----------------------------------------------------------------------------
// SID CHIP REGISTER STRUCTURE
//-----------------------------------------------------------------------------

// Complete SID chip register layout (25 bytes: $D400-$D418)
typedef struct {
    SID_Voice voice[3];         // $00-$14 - Three independent voices
    uint8_t filter_cutoff_lo;   // $15 - Filter cutoff frequency low byte
    uint8_t filter_cutoff_hi;   // $16 - Filter cutoff frequency high byte (bits 0-2)
    uint8_t filter_control;     // $17 - Filter voice routing and resonance
    uint8_t filter_mode_volume; // $18 - Filter mode select and master volume
} SID_Chip;

// Global pointer to SID chip registers
// This is marked 'volatile' because the hardware can change these values
// (though in practice, we only write to the SID, never read from it)
extern volatile SID_Chip* const sid;

//-----------------------------------------------------------------------------
// WAVEFORM CONTROL BITS (for voice[n].control register)
//-----------------------------------------------------------------------------

// Gate bit (bit 0): Triggers the envelope generator
// 0 = Release phase, 1 = Attack/Decay/Sustain phases
// The envelope generator starts when gate goes from 0→1
#define SID_WAVE_GATE     0x01

// Sync bit (bit 1): Synchronize oscillator with previous voice
// Voice 1 syncs to voice 3, voice 2 to voice 1, voice 3 to voice 2
// When set, resets this oscillator when the previous oscillator wraps
#define SID_WAVE_SYNC     0x02

// Ring modulation bit (bit 2): Enable ring modulation
// Voice 1 uses voice 3, voice 2 uses voice 1, voice 3 uses voice 2
// Creates bell-like metallic sounds by multiplying waveforms
#define SID_WAVE_RINGMOD  0x04

// Test bit (bit 3): Disables oscillator and resets to zero
// Used for hard restart effects. Should be set then cleared quickly.
#define SID_WAVE_TEST     0x08

// Waveform select bits (bits 4-7): Choose oscillator waveform
// Multiple waveforms can be combined (though some combinations are uncommon)
#define SID_WAVE_TRIANGLE 0x10  // Triangle wave (soft, flute-like)
#define SID_WAVE_SAWTOOTH 0x20  // Sawtooth wave (bright, buzzy)
#define SID_WAVE_PULSE    0x40  // Pulse/square wave (hollow, reedy)
#define SID_WAVE_NOISE    0x80  // Noise (for drums, effects)

//-----------------------------------------------------------------------------
// FILTER MODE BITS (for filter_mode_volume register)
//-----------------------------------------------------------------------------

// Filter mode bits (bits 4-6): Select filter type
// Multiple modes can be combined for band-reject or other responses
#define SID_FILTER_LP     0x10  // Low-pass filter (removes highs)
#define SID_FILTER_BP     0x20  // Band-pass filter (removes highs and lows)
#define SID_FILTER_HP     0x40  // High-pass filter (removes lows)

// Voice 3 off bit (bit 7): Disconnect voice 3 from audio output
// Voice 3 can still modulate other voices (sync, ring mod) but won't be heard
// Useful for using voice 3 as an LFO (low-frequency oscillator)
#define SID_FILTER_OFF    0x80

// Master volume mask (bits 0-3): Overall output volume
// Range: 0 (silence) to 15 (maximum)
#define SID_VOLUME_MASK   0x0F

//-----------------------------------------------------------------------------
// FILTER ROUTING BITS (for filter_control register)
//-----------------------------------------------------------------------------

// Filter voice routing (bits 0-2): Which voices go through filter
#define SID_FILTER_VOICE1 0x01  // Route voice 1 through filter
#define SID_FILTER_VOICE2 0x02  // Route voice 2 through filter
#define SID_FILTER_VOICE3 0x04  // Route voice 3 through filter

// External input routing (bit 3): Route external audio input through filter
// The C64 has an external audio input pin that can be filtered
#define SID_FILTER_EXT    0x08

// Filter resonance (bits 4-7): Filter resonance/Q factor
// Higher values create more pronounced filter peak
// Value is in high nibble: (resonance << 4)
#define SID_FILTER_RESONANCE_SHIFT 4

//-----------------------------------------------------------------------------
// INLINE ACCESSOR FUNCTIONS
//-----------------------------------------------------------------------------
// These functions provide a clean interface for writing to SID registers.
// They are marked 'static inline' so the compiler will inline them at the
// call site, avoiding function call overhead while keeping the code readable.
//-----------------------------------------------------------------------------

/**
 * Write frequency value to a voice.
 *
 * The frequency determines the pitch of the note. Higher values = higher pitch.
 * This writes both the low and high bytes of the 16-bit frequency register.
 *
 * @param voice Voice number (0-2)
 * @param freq Frequency value (0-65535)
 */
static inline void sid_write_freq(uint8_t voice, sid_freq_t freq) {
    sid->voice[voice].freq_lo = freq & 0xFF;
    sid->voice[voice].freq_hi = freq >> 8;
}

/**
 * Write pulse width value to a voice.
 *
 * The pulse width controls the duty cycle of the pulse waveform.
 * Only the lower 12 bits are significant (0-4095).
 * 2048 = 50% square wave, 1024 = 25% pulse, 3072 = 75% pulse
 *
 * @param voice Voice number (0-2)
 * @param pulse Pulse width value (0-4095)
 */
static inline void sid_write_pulse(uint8_t voice, sid_pulse_t pulse) {
    sid->voice[voice].pulse_lo = pulse & 0xFF;
    sid->voice[voice].pulse_hi = (pulse >> 8) & 0x0F;  // Only lower 4 bits
}

/**
 * Write waveform and gate to a voice.
 *
 * This combines the waveform selection with the gate bit. The gate parameter
 * should be 0xFF (gate on) or 0xFE (gate off). ANDing the waveform with the
 * gate clears bit 0 when gate is off, while preserving all other bits.
 *
 * This technique comes from the original assembly player and is efficient
 * because it avoids branching.
 *
 * @param voice Voice number (0-2)
 * @param waveform Waveform bits (SID_WAVE_* constants)
 * @param gate Gate flag (0xFF = on, 0xFE = off)
 */
static inline void sid_write_control(uint8_t voice, uint8_t waveform, uint8_t gate) {
    // ANDing with gate (0xFF or 0xFE) sets or clears the gate bit
    // 0xFF & waveform = waveform (gate on)
    // 0xFE & waveform = waveform with bit 0 cleared (gate off)
    sid->voice[voice].control = waveform & gate;
}

/**
 * Write ADSR envelope to a voice.
 *
 * The ADSR (Attack, Decay, Sustain, Release) envelope shapes how the volume
 * of a note changes over time:
 * - Attack: How quickly volume rises to maximum (0-15, faster to slower)
 * - Decay: How quickly volume falls to sustain level (0-15, faster to slower)
 * - Sustain: Volume level held while gate is on (0-15, quiet to loud)
 * - Release: How quickly volume falls to zero after gate off (0-15, faster to slower)
 *
 * @param voice Voice number (0-2)
 * @param attack_decay Attack (high nibble) and Decay (low nibble)
 * @param sustain_release Sustain (high nibble) and Release (low nibble)
 */
static inline void sid_write_adsr(uint8_t voice, uint8_t attack_decay, uint8_t sustain_release) {
    sid->voice[voice].attack_decay = attack_decay;
    sid->voice[voice].sustain_release = sustain_release;
}

/**
 * Write filter cutoff frequency.
 *
 * The cutoff frequency determines which frequencies are affected by the filter.
 * This is an 11-bit value (0-2047), with the low 3 bits in filter_cutoff_lo
 * and the high 8 bits in filter_cutoff_hi.
 *
 * Higher values = higher cutoff frequency.
 *
 * @param cutoff Cutoff frequency (0-2047)
 */
static inline void sid_write_filter_cutoff(uint16_t cutoff) {
    sid->filter_cutoff_lo = cutoff & 0x07;           // Lower 3 bits
    sid->filter_cutoff_hi = (cutoff >> 3) & 0xFF;    // Upper 8 bits
}

/**
 * Write filter control (voice routing and resonance).
 *
 * The filter control byte determines:
 * - Bits 0-2: Which voices are routed through the filter
 * - Bit 3: External input routing
 * - Bits 4-7: Filter resonance (Q factor)
 *
 * @param control Filter control byte
 */
static inline void sid_write_filter_control(uint8_t control) {
    sid->filter_control = control;
}

/**
 * Write filter mode and master volume.
 *
 * This register combines two functions:
 * - Bits 0-3: Master volume (0-15)
 * - Bits 4-7: Filter mode select (LP/BP/HP) and voice 3 off
 *
 * @param mode_volume Combined filter mode and volume byte
 */
static inline void sid_write_filter_mode_volume(uint8_t mode_volume) {
    sid->filter_mode_volume = mode_volume;
}

/**
 * Set master volume only (preserves filter mode).
 *
 * This is a convenience function that sets the master volume without
 * affecting the filter mode bits.
 *
 * @param volume Volume level (0-15)
 */
static inline void sid_set_volume(uint8_t volume) {
    // Preserve filter mode bits (4-7), update volume bits (0-3)
    sid->filter_mode_volume = (sid->filter_mode_volume & 0xF0) | (volume & 0x0F);
}

#endif // PLAYER_SID_H
