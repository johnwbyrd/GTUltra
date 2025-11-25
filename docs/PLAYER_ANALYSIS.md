# Player.s Architecture Analysis

## Overview
Player.s is a 1-SID (3-channel) music player for the Commodore 64. It plays pattern-based music with effects, supporting wavetables, pulse modulation, filters, and sound effects.

---

## I. DATA STRUCTURES

### A. Constants & Defines (lines 12-82)

#### Zero Page Variables (lines 14-31)
```
mt_temp1, mt_temp2          - Temporary variables for calculations
ghostfreqlo/hi[x]           - Shadow registers for SID frequency (optional)
ghostpulselo/hi[x]          - Shadow registers for pulse width (optional)
ghostwave[x]                - Shadow register for waveform (optional)
ghostad[x], ghostsr[x]      - Shadow registers for ADSR envelope (optional)
ghostfiltcutlow/off[x]      - Shadow registers for filter (optional)
ghostfiltctrl[x]            - Shadow register for filter control (optional)
ghostfilttype[x]            - Shadow register for filter type (optional)
```

**Purpose**: Ghost registers allow optional buffering of SID writes. Can be in zero page (ZPGHOSTREGS) or regular memory.

#### Pattern Data Constants (lines 36-46)
```
ENDPATT = $00              - End of pattern marker
INS = $00                  - Instrument change command
FX = $40                   - Effect command
FXONLY = $50               - Effect only (no note follows)
NOTE = $60                 - Note data follows
REST = $bd                 - Rest (no sound)
KEYOFF = $be               - Gate off
KEYON = $bf                - Gate on
FIRSTPACKEDREST = $c0      - Packed rest encoding (saves space)
PACKEDREST = $00           - Packed rest value
```

#### Effect Numbers (lines 49-65)
```
00 - DONOTHING       - Instrument vibrato (automatic)
01 - PORTAUP         - Pitch slide up
02 - PORTADOWN       - Pitch slide down
03 - TONEPORTA       - Slide to target note
04 - VIBRATO         - Oscillating pitch
05 - SETAD           - Set Attack/Decay envelope
06 - SETSR           - Set Sustain/Release envelope
07 - SETWAVE         - Set waveform immediately
08 - SETWAVEPTR      - Set wavetable pointer
09 - SETPULSEPTR     - Set pulse table pointer
0a - SETFILTPTR      - Set filter table pointer
0b - SETFILTCTRL     - Set filter control (routing/resonance)
0c - SETFILTCUTOFF   - Set filter cutoff frequency
0d - SETMASTERVOL    - Set master volume / timing mark
0e - SETFUNKTEMPO    - Set funk tempo (alternating speeds)
0f - SETTEMPO        - Set global or per-channel tempo
```

#### Order List Commands (lines 68-73)
```
REPEAT = $d0         - Repeat section N times
TRANSDOWN = $e0      - Transpose down
TRANS = $f0          - Transpose (up)
TRANSUP = $f0        - Same as TRANS
LOOPSONG = $ff       - Loop back to beginning
```

#### Table Commands (lines 76-82)
```
LOOPWAVE = $ff       - Loop in wavetable
LOOPPULSE = $ff      - Loop in pulse table
LOOPFILT = $ff       - Loop in filter table
SETPULSE = $80       - Set pulse width directly
SETFILTER = $80      - Set filter parameters directly
SETCUTOFF = $00      - Set cutoff directly
```

---

### B. Jump Table (lines 87-95)
```
+0: jmp mt_init           - Initialize player with song number
+3: jmp mt_play           - Main play routine (call once per frame)
+6: jmp mt_playsfx        - Play sound effect (optional)
+9: jmp mt_setmastervol   - Set master volume (optional)
```

**Purpose**: Provides stable entry points for calling code.

---

### C. Channel Variables (lines 1643-1873)

Each channel has 14 bytes of state. With 3 channels = 42 bytes total.

#### Per-Channel Layout (stride = 7 for first section):
```
Offset 0-6 (first variable block):
+0  mt_chnsongptr       - Current position in song order list
+1  mt_chntrans         - Transpose amount (signed)
+2  mt_chnrepeat        - Repeat counter
+3  mt_chnpattptr       - Current position in pattern data
+4  mt_chnpackedrest    - Packed rest counter
+5  mt_chnnewfx         - New effect number to apply
+6  mt_chnnewparam      - New effect parameter to apply

Offset 7-13 (second variable block):
+0  mt_chnfx            - Current active effect
+1  mt_chnparam         - Current effect parameter
+2  mt_chnnewnote       - New note to initialize
+3  mt_chnwaveptr       - Current wavetable position
+4  mt_chnwave          - Current waveform value
+5  mt_chnpulseptr      - Current pulse table position
+6  mt_chnpulsetime     - Pulse table timer

Offset 14-20 (third variable block):
+0  mt_chnsongnum       - Song number (for multi-song files)
+1  mt_chnpattnum       - Current pattern number
+2  mt_chntempo         - Tempo (ticks between notes)
+3  mt_chncounter       - Tick counter (counts down)
+4  mt_chnnote          - Current note value
+5  mt_chninstr         - Current instrument number
+6  mt_chngate          - Gate flag ($ff=on, $fe=off)

Offset 21-27 (fourth variable block):
+0  mt_chnvibtime       - Vibrato phase position
+1  mt_chnvibdelay      - Vibrato delay counter
+2  mt_chnwavetime      - Wavetable delay counter
+3  mt_chnfreqlo        - Current frequency low byte
+4  mt_chnfreqhi        - Current frequency high byte
+5  mt_chnpulselo       - Current pulse width low byte
+6  mt_chnpulsehi       - Current pulse width high byte

Offset 28-34 (fifth variable block):
+0  mt_chnad            - Attack/Decay value
+1  mt_chnsr            - Sustain/Release value
+2  mt_chnsfx           - Sound effect active flag
+3  mt_chnsfxlo         - Sound effect data pointer low
+4  mt_chnsfxhi         - Sound effect data pointer high
+5  mt_chngatetimer     - Gate-off timer from instrument
+6  mt_chnlastnote      - Last note (for calculated speed effects)
```

---

## II. MUSIC DATA FORMAT

### A. Song Structure
```
Song Data (generated by gt2reloc):
1. Frequency Table (FIRSTNOTE..LASTNOTE)
   - mt_freqtbllo[], mt_freqtblhi[]

2. Order Lists (one per channel)
   - mt_songtbllo[], mt_songtblhi[]
   - Contains sequence of: pattern numbers, LOOP, TRANS, REPEAT

3. Pattern Data
   - mt_patttbllo[], mt_patttblhi[]
   - Compressed note/effect data

4. Instrument Definitions
   - mt_insgatetimer[]    - Gate timing
   - mt_insfirstwave[]    - First frame waveform
   - mt_inspulseptr[]     - Pulse table index
   - mt_insfiltptr[]      - Filter table index
   - mt_inswaveptr[]      - Wave table index
   - mt_insad[]           - Attack/Decay
   - mt_inssr[]           - Sustain/Release
   - mt_insvibdelay[]     - Vibrato delay
   - mt_insvibparam[]     - Vibrato parameters

5. Wavetables
   - mt_wavetbl[]         - Waveform sequences
   - mt_notetbl[]         - Relative note offsets

6. Pulse Tables
   - mt_pulsetimetbl[]    - Pulse step durations
   - mt_pulsespdtbl[]     - Pulse step values/speeds

7. Filter Tables
   - mt_filttimetbl[]     - Filter step durations
   - mt_filtspdtbl[]      - Filter step values/speeds

8. Speed Tables (for calculated speed effects)
   - mt_speedlefttbl[]    - High byte of speed
   - mt_speedrighttbl[]   - Low byte of speed
```

---

## III. PATTERN DATA ENCODING

### Format
Pattern data is a compressed stream of commands:

```
Byte < $40:           Instrument change (00-3F)
                      Next byte is either FX or NOTE

$40-$4F:              FX command ($40 + effect number 0-F)
                      If effect != 0: next byte is parameter
                      Next byte is NOTE

$50-$5F:              FXONLY command ($50 + effect number 0-F)
                      If effect != 0: next byte is parameter
                      No NOTE follows (just effect change)

$60-$BC:              NOTE ($60 + note value)

$BD:                  REST
$BE:                  KEYOFF (gate off without note)
$BF:                  KEYON (gate on without note)
$C0-$FE:              PACKED REST (saves space for multiple rests)
                      Value = number of rest rows remaining

$00:                  ENDPATT (end of pattern)
```

---

## IV. MAIN EXECUTION FLOW

### A. Initialization: mt_init (lines 516-524)

**Purpose**: Initialize player with a song number.

**Pseudo-code**:
```c
void mt_init(uint8_t song_number) {
    if (NUMSONGS > 1) {
        // Song number × 3 gives offset into song table
        mt_initsongnum = song_number * 3;
    } else {
        mt_initsongnum = 0;
    }
}
```

**Called once** before playing music. The song_number parameter is multiplied by 3 because each song has 3 order lists (one per channel).

---

### B. Main Playback: mt_play (lines 560-1401)

**Called once per frame** (50Hz PAL or 60Hz NTSC).

#### Phase 1: Ghost Register Copy (lines 561-567)
**Optional**: If using ghost registers in RAM, copy them to SID at frame start.

```c
if (ZPGHOSTREGS == 0 && GHOSTREGS != 0) {
    // Copy all 25 shadow registers to SID in one loop
    for (int i = 0; i < 25; i++) {
        SIDBASE[i] = ghostregs[i];
    }
}
```

#### Phase 2: Song Initialization Check (lines 577-627)
**Runs once** when mt_initsongnum is set (negative value).

```c
if (mt_initsongnum < 0) {
    // Reset all channel variables to 0
    for (int i = 0; i < NUMCHANNELS * 14; i++) {
        channel_vars[i] = 0;
    }

    // Reset SID filter
    SIDBASE[0x15] = 0;  // Filter cutoff low
    SIDBASE[0x16] = 0;  // Filter cutoff high
    SIDBASE[0x17] = 0;  // Filter control
    SIDBASE[0x18] = 0;  // Filter type/volume

    mt_filtctrl = 0;
    mt_filtstep = 0;

    // Initialize each channel
    for each channel (x = 0, 7, 14) {
        mt_chnsongnum[x] = song_index++;
        mt_chntempo[x] = DEFAULTTEMPO;
        mt_chncounter[x] = 1;
        mt_chninstr[x] = 1;
        load_waveform_only(x);  // Initialize waveform
    }

    mt_initsongnum = 0;  // Clear init flag
}
```

#### Phase 3: Filter Execution (lines 630-705)
**Runs every frame** for the global filter.

```c
void execute_filter() {
    uint8_t step = mt_filtstep;
    if (step == 0) return;  // Filter stopped

    // Check if modulation time remaining
    if (mt_filttime > 0) {
        // Continue modulation
        mt_filtcutoff += mt_filtspdtbl[step - 1];
        mt_filttime--;
        if (mt_filttime > 0) {
            SIDBASE[0x16] = mt_filtcutoff;
            return;
        }
    }

    // Process next filter step
    uint8_t cmd = mt_filttimetbl[step - 1];

    if (cmd == 0) {
        // Set cutoff directly
        mt_filtcutoff = mt_filtspdtbl[step - 1];
    }
    else if (cmd >= 0x80) {
        // Set filter parameters
        mt_filttype = (cmd << 1);  // Passband type
        mt_filtctrl = mt_filtspdtbl[step - 1];  // Routing/resonance

        // Check if cutoff follows immediately
        if (mt_filttimetbl[step] == 0) {
            step++;
            mt_filtcutoff = mt_filtspdtbl[step - 1];
        }
    }
    else {
        // Start new modulation
        mt_filttime = cmd;
        // Speed applied on next iteration
    }

    // Check for loop
    step++;
    if (mt_filttimetbl[step - 1] == LOOPFILT) {
        step = mt_filtspdtbl[step - 1];  // Jump
    }

    mt_filtstep = step;
    SIDBASE[0x16] = mt_filtcutoff;
    SIDBASE[0x17] = mt_filtctrl;
    SIDBASE[0x18] = mt_filttype | mt_masterfader;  // Volume in low nibble
}
```

#### Phase 4: Channel Execution (lines 706-1401)
**Main music logic**. Executes for each channel (x = 0, 7, 14).

This is the most complex part - I'll continue in next section...

---

## V. CHANNEL EXECUTION DETAILS

### A. Tick Counter (lines 720-743)

```c
void execute_channel(uint8_t x) {
    // Decrement tick counter
    mt_chncounter[x]--;

    if (mt_chncounter[x] == 0) {
        goto tick_0;  // Time for new note
    }

    if (mt_chncounter[x] > 0) {
        goto execute_effects;  // Continue effects
    }

    // Counter went negative - reload tempo
    uint8_t tempo = mt_chntempo[x];

    // Special "funktempo" mode for swing timing
    if (tempo < 2) {
        tempo = mt_funktempotbl[tempo];
        mt_chntempo[x] ^= 1;  // Alternate 0<->1
    }

    mt_chncounter[x] = tempo;
    goto execute_effects;
```

**Purpose**: Implements timing. Tempo = ticks between note fetches. Funktempo allows alternating speeds for swing feel.

---

### B. Tick 0: Sequencer (lines 760-827)

**Runs when counter reaches 0** - time to fetch next note.

```c
tick_0:
    // Setup effect jump tables for tick 0
    uint8_t fx = mt_chnnewfx[x];
    mt_tick0jump = mt_tick0jumptbl[fx];

    // Check if need to fetch new pattern
    if (mt_chnpattptr[x] == 0) {
        // Sequencer: get next pattern from order list
        uint8_t songnum = mt_chnsongnum[x];
        uint16_t songlist_ptr = (mt_songtblhi[songnum] << 8) | mt_songtbllo[songnum];
        uint8_t songpos = mt_chnsongptr[x];
        uint8_t cmd = songlist_ptr[songpos];

        // Check for special commands
        if (cmd == LOOPSONG) {
            songpos = songlist_ptr[songpos + 1];  // Jump
            cmd = songlist_ptr[songpos];
        }

        if (cmd >= TRANSDOWN) {
            // Transpose command
            mt_chntrans[x] = cmd - TRANS;  // Signed offset
            songpos++;
            cmd = songlist_ptr[songpos];
        }

        if (cmd >= REPEAT) {
            // Repeat command
            uint8_t repeat_count = cmd - REPEAT;
            mt_chnrepeat[x]++;
            if (mt_chnrepeat[x] < repeat_count) {
                goto nonewpatt;  // Continue current pattern
            }
            mt_chnrepeat[x] = 0;  // Reset repeat
        }

        mt_chnpattnum[x] = cmd;  // Store pattern number
        songpos++;
        mt_chnsongptr[x] = songpos;
    }
```

---

### C. Note Initialization (lines 809-917)

```c
nonewpatt:
    // Get instrument parameters
    uint8_t instr = mt_chninstr[x];
    mt_chngatetimer[x] = mt_insgatetimer[instr - 1];

    // Check if new note pending
    uint8_t newnote = mt_chnnewnote[x];
    if (newnote == 0) {
        goto no_new_note;  // Just effect change
    }

    // Initialize new note
    mt_chnnote[x] = newnote - NOTE;
    mt_chnnewnote[x] = 0;
    mt_chnfx[x] = 0;  // Reset effect

    // Load instrument vibrato
    mt_chnvibdelay[x] = mt_insvibdelay[instr - 1];
    mt_chnparam[x] = mt_insvibparam[instr - 1];

    // Check for toneporta - skip most init
    if (mt_chnnewfx[x] == TONEPORTA) {
        goto no_new_note;
    }

    // Load first frame waveform
    uint8_t wave = mt_insfirstwave[instr - 1];
    if (wave == 0) goto skip_wave;
    if (wave >= 0xfe) goto skip_wave2;  // Special: skip but load gate

    mt_chnwave[x] = wave;
    mt_chngate[x] = 0xff;  // Gate on

skip_wave2:
skip_wave:

    // Load pulse table pointer
    uint8_t pulse_ptr = mt_inspulseptr[instr - 1];
    if (pulse_ptr != 0) {
        mt_chnpulseptr[x] = pulse_ptr;
        mt_chnpulsetime[x] = 0;
    }

    // Load filter table pointer
    uint8_t filt_ptr = mt_insfiltptr[instr - 1];
    if (filt_ptr != 0) {
        mt_filtstep = filt_ptr;
        mt_filttime = 0;
    }

    // Load wave table pointer
    mt_chnwaveptr[x] = mt_inswaveptr[instr - 1];

    // Load ADSR
    mt_chnsr[x] = mt_inssr[instr - 1];
    mt_chnad[x] = mt_insad[instr - 1];

    // Execute tick 0 effect
    tick0_effect(mt_chnnewparam[x]);

    goto load_registers;
```

**Purpose**: Initializes a new note with instrument parameters.

---

### D. Wavetable Execution (lines 934-1056)

```c
no_new_note:
    // Execute tick 0 effect
    tick0_effect(mt_chnnewparam[x]);

execute_wavetable:
    uint8_t waveptr = mt_chnwaveptr[x];
    if (waveptr == 0) goto wavedone;  // No wavetable

    uint8_t wave_cmd = mt_wavetbl[waveptr - 1];

    // Check for delay (0-15)
    if (wave_cmd < 0x10) {
        if (wave_cmd == mt_chnwavetime[x]) {
            mt_chnwavetime[x]++;
            goto wavedone;  // Still waiting
        }
        goto nowavechange;
    }

    wave_cmd -= 0x10;

    // Check for wavetable command ($E0-$EF)
    if (wave_cmd >= 0xe0) {
        goto execute_wave_command;
    }

    // Normal waveform
    mt_chnwave[x] = wave_cmd;

nowavechange:
    // Check for loop
    uint8_t next_cmd = mt_wavetbl[waveptr];
    if (next_cmd == LOOPWAVE) {
        waveptr = mt_notetbl[waveptr];  // Jump
    } else {
        waveptr++;
    }

    mt_chnwaveptr[x] = waveptr;
    mt_chnwavetime[x] = 0;

    // Get note offset from table
    int8_t note_offset = mt_notetbl[waveptr - 2];

    if (note_offset == 0) goto wavedone;  // No frequency change

    // Calculate note
    if (note_offset < 0) {
        // Relative note
        note = mt_chnnote[x] + note_offset;
    } else {
        // Absolute note
        note = note_offset;
    }

    // Reset vibrato
    mt_chnvibtime[x] = 0;

    // Load frequency from table
    mt_chnfreqlo[x] = mt_freqtbllo[note - FIRSTNOTE];
    mt_chnfreqhi[x] = mt_freqtblhi[note - FIRSTNOTE];

    goto check_gate_timer;
```

**Purpose**: Executes wavetable sequences - changes waveform and pitch over time.

---

### E. Continuous Effects (lines 984-1056)

```c
wavedone:
    // Skip continuous effects on tick 0
    if (mt_chncounter[x] == 0) goto check_gate_timer;

    // Get current effect
    uint8_t fx = mt_chnfx[x];
    uint8_t param = mt_chnparam[x];

    // Calculate speed from parameter
    uint8_t speed_hi, speed_lo;

    if (mt_speedlefttbl[param - 1] >= 0x80) {
        // Calculated speed (note-relative)
        uint8_t last_note = mt_chnlastnote[x];
        uint16_t freq_diff = mt_freqtbllo[last_note + 1 - FIRSTNOTE]
                           - mt_freqtbllo[last_note - FIRSTNOTE];
        // Shift right by param count
        for (int i = 0; i < mt_speedrighttbl[param - 1]; i++) {
            freq_diff >>= 1;
        }
        speed_lo = freq_diff & 0xff;
        speed_hi = freq_diff >> 8;
    } else {
        // Normal speed
        speed_lo = mt_speedrighttbl[param - 1];
        speed_hi = mt_speedlefttbl[param - 1];
    }

    // Execute effect
    switch(fx) {
        case 0:  // Instrument vibrato
            if (param == 0) break;
            if (mt_chnvibdelay[x] > 0) {
                mt_chnvibdelay[x]--;
                break;
            }
            // Fall through to vibrato

        case 4:  // Vibrato
            int8_t vib_time = mt_chnvibtime[x];
            if (vib_time >= 0) {
                if (vib_time >= speed_lo) {
                    vib_time = -vib_time;
                }
            }
            vib_time += 2;
            mt_chnvibtime[x] = vib_time;

            if (vib_time & 1) {
                goto freq_subtract;
            } else {
                goto freq_add;
            }

        case 1:  // Portamento up
        case 2:  // Portamento down
            if (fx == 1) goto freq_add;
            else goto freq_subtract;

        case 3:  // Tone portamento
            if (param == 0) break;  // Speed 0 = tie note

            // Calculate distance to target
            uint8_t target_note = mt_chnnote[x];
            int16_t offset = mt_chnfreqlo[x] - mt_freqtbllo[target_note - FIRSTNOTE];
            offset |= (mt_chnfreqhi[x] - mt_freqtblhi[target_note - FIRSTNOTE]) << 8;

            if (offset < 0) {
                // Need to go up
                offset += speed_lo;
                if (offset >= 0) goto porta_done;  // Reached target
                goto freq_add;
            } else {
                // Need to go down
                offset -= speed_lo;
                if (offset <= 0) goto porta_done;
                goto freq_subtract;
            }

porta_done:
            // Snap to exact target frequency
            mt_chnfreqlo[x] = mt_freqtbllo[target_note - FIRSTNOTE];
            mt_chnfreqhi[x] = mt_freqtblhi[target_note - FIRSTNOTE];
            goto check_gate_timer;
    }
    goto check_gate_timer;

freq_add:
    uint16_t freq = mt_chnfreqlo[x] | (mt_chnfreqhi[x] << 8);
    freq += speed_lo | (speed_hi << 8);
    mt_chnfreqlo[x] = freq & 0xff;
    mt_chnfreqhi[x] = freq >> 8;
    goto check_gate_timer;

freq_subtract:
    uint16_t freq = mt_chnfreqlo[x] | (mt_chnfreqhi[x] << 8);
    freq -= speed_lo | (speed_hi << 8);
    mt_chnfreqlo[x] = freq & 0xff;
    mt_chnfreqhi[x] = freq >> 8;
    goto check_gate_timer;
```

**Purpose**: Implements continuous pitch effects (vibrato, portamento) that run every tick.

---

### F. Pulse Table Execution (lines 1106-1206)

```c
check_gate_timer:
    // Check if time to fetch new note
    if (mt_chncounter[x] == mt_chngatetimer[x]) {
        goto fetch_new_note;
    }

execute_pulse:
    uint8_t pulseptr = mt_chnpulseptr[x];
    if (pulseptr == 0) goto pulse_done;

    // Skip if sequencer just ran
    if (mt_chnpattptr[x] == 0 && mt_chncounter[x] == 0) {
        goto pulse_done;
    }

    if (mt_chnpulsetime[x] > 0) {
        // Continue modulation
        int8_t speed = mt_pulsespdtbl[pulseptr - 1];
        uint16_t pulse = mt_chnpulselo[x] | (mt_chnpulsehi[x] << 8);

        if (speed < 0) {
            pulse += speed;  // Add negative = subtract
        } else {
            pulse += speed;
        }

        mt_chnpulselo[x] = pulse & 0xff;
        mt_chnpulsehi[x] = pulse >> 8;

        mt_chnpulsetime[x]--;
        if (mt_chnpulsetime[x] == 0) {
            goto next_pulse_step;
        }
        goto pulse_done;
    }

next_pulse_step:
    uint8_t cmd = mt_pulsetimetbl[pulseptr - 1];

    if (cmd >= 0x80) {
        // Set pulse directly
        mt_chnpulsehi[x] = cmd;
        mt_chnpulselo[x] = mt_pulsespdtbl[pulseptr - 1];
    } else {
        // Start modulation
        mt_chnpulsetime[x] = cmd;
    }

    // Check for loop
    pulseptr++;
    if (mt_pulsetimetbl[pulseptr - 1] == LOOPPULSE) {
        pulseptr = mt_pulsespdtbl[pulseptr - 1];
    }

    mt_chnpulseptr[x] = pulseptr;

pulse_done:
    // Write pulse to SID (if not using buffered writes)
    SIDBASE[0x02 + x] = mt_chnpulselo[x];
    SIDBASE[0x03 + x] = mt_chnpulsehi[x];

    goto load_registers;
```

**Purpose**: Modulates pulse width for PWM effects.

---

### G. Pattern Data Fetch (lines 1224-1353)

```c
fetch_new_note:
    // Get pattern data pointer
    uint8_t pattnum = mt_chnpattnum[x];
    uint16_t patt_ptr = (mt_patttblhi[pattnum] << 8) | mt_patttbllo[pattnum];
    uint8_t pattpos = mt_chnpattptr[x];

    uint8_t data = patt_ptr[pattpos];

    // Decode pattern data
    if (data < FX) {
        // Instrument change
        mt_chninstr[x] = data;
        pattpos++;
        data = patt_ptr[pattpos];
    }

    if (data < NOTE) {
        // Effect
        uint8_t fx = data & 0x0f;
        bool note_follows = (data < FXONLY);

        mt_chnnewfx[x] = fx;

        if (fx != 0) {
            pattpos++;
            mt_chnnewparam[x] = patt_ptr[pattpos];
        }

        if (note_follows) {
            pattpos++;
            data = patt_ptr[pattpos];
        } else {
            goto rest;  // Effect only, no note
        }
    }

    if (data >= FIRSTPACKEDREST) {
        // Packed rest
        if (mt_chnpackedrest[x] == 0) {
            mt_chnpackedrest[x] = data;
        }
        mt_chnpackedrest[x]--;
        if (mt_chnpackedrest[x] == 0) {
            goto rest;
        }
        goto load_registers;
    }

    // Note data
    if (data == REST) {
        goto rest;
    }

    if (data == KEYOFF || data == KEYON) {
        mt_chngate[x] = (data == KEYON) ? 0xff : 0xfe;
        goto rest;
    }

    // Normal note
    mt_chnnewnote[x] = data;

    // Check for toneporta - skip gate off
    if (mt_chnnewfx[x] == TONEPORTA) {
        goto rest;
    }

    // Hard restart (if instrument has HR flag)
    uint8_t instr = mt_chninstr[x];
    if (instr < FIRSTNOHRINSTR) {
        // Hard restart: set AD=0, SR=0 briefly
        SIDBASE[0x05 + x] = ADPARAM;
        SIDBASE[0x06 + x] = SRPARAM;
    }

    mt_chngate[x] = 0xfe;  // Gate off for hard restart

rest:
    // Move to next byte in pattern
    pattpos++;
    if (patt_ptr[pattpos] == 0) {
        pattpos = 0;  // End of pattern - trigger sequencer
    }
    mt_chnpattptr[x] = pattpos;
```

**Purpose**: Decodes compressed pattern data and extracts notes/effects.

---

### H. Register Writing (lines 1356-1401)

```c
load_registers:
    // Check for sound effect override
    if (mt_chnsfx[x] != 0) {
        goto execute_sfx;
    }

    // Write all registers to SID
    SIDBASE[0x05 + x] = mt_chnad[x];      // Attack/Decay
    SIDBASE[0x06 + x] = mt_chnsr[x];      // Sustain/Release
    SIDBASE[0x02 + x] = mt_chnpulselo[x]; // Pulse low
    SIDBASE[0x03 + x] = mt_chnpulsehi[x]; // Pulse high
    SIDBASE[0x00 + x] = mt_chnfreqlo[x];  // Frequency low
    SIDBASE[0x01 + x] = mt_chnfreqhi[x];  // Frequency high

    // Waveform with gate
    uint8_t wave = mt_chnwave[x] & mt_chngate[x];
    SIDBASE[0x04 + x] = wave;

    return;
```

**Purpose**: Writes channel state to SID registers.

---

## VI. EFFECT HANDLERS (Tick 0)

These execute on the first tick when an effect is triggered:

### Effect 0: Instrument Vibrato (lines 156-168)
```c
// Automatic vibrato from instrument
// Just sets up parameters - actual vibrato runs continuously
```

### Effect 1-2: Portamento (lines 172-179)
```c
// Reset vibrato when starting portamento
mt_chnvibtime[x] = 0;
mt_chnparam[x] = param;
mt_chnfx[x] = fx;
```

### Effect 3-4: Tone Porta / Vibrato (lines 183-190)
```c
// Store parameters for continuous execution
mt_chnparam[x] = param;
mt_chnfx[x] = fx;
```

### Effect 5: Set Attack/Decay (lines 194-206)
```c
// Write directly to SID
SIDBASE[0x05 + x] = param;
```

### Effect 6: Set Sustain/Release (lines 210-222)
```c
// Write directly to SID
SIDBASE[0x06 + x] = param;
```

### Effect 7: Set Waveform (lines 226-230)
```c
mt_chnwave[x] = param;
```

### Effect 8: Set Wave Pointer (lines 234-242)
```c
mt_chnwaveptr[x] = param;
mt_chnwavetime[x] = 0;  // Reset delay
```

### Effect 9: Set Pulse Pointer (lines 246-252)
```c
mt_chnpulseptr[x] = param;
mt_chnpulsetime[x] = 0;
```

### Effect A: Set Filter Pointer (lines 256-265)
```c
mt_filtstep = param;
mt_filttime = 0;
```

### Effect B: Set Filter Control (lines 269-280)
```c
mt_filtctrl = param;
if (param == 0) {
    mt_filtstep = 0;  // Stop filter programming
}
```

### Effect C: Set Filter Cutoff (lines 284-288)
```c
mt_filtcutoff = param;
```

### Effect D: Set Master Volume (lines 292-306)
```c
if (param < 0x10) {
    mt_masterfader = param;  // Volume 0-15
} else {
    mt_author[31] = param;   // Timing mark for sync
}
```

### Effect E: Funk Tempo (lines 310-321)
```c
// Alternating tempo for swing feel
mt_funktempotbl[0] = mt_speedlefttbl[param - 1];
mt_funktempotbl[1] = mt_speedrighttbl[param - 1];
```

### Effect F: Set Tempo (lines 325-345)
```c
if (param < 0x80) {
    // Global tempo (all channels)
    for each channel {
        mt_chntempo[x] = param;
    }
} else {
    // Per-channel tempo
    mt_chntempo[x] = param & 0x7f;
}
```

---

## VII. SOUND EFFECTS (lines 1410-1530)

**Purpose**: Overrides music channel with simple sound effect.

```c
void play_sfx(uint8_t channel, uint16_t sfx_ptr) {
    // Priority check - only override with higher address
    if (mt_chnsfx[channel] != 0) {
        if (sfx_ptr <= current_sfx_ptr) {
            return;  // Lower priority - don't play
        }
    }

    mt_chnsfx[channel] = 1;  // Mark as active
    mt_chnsfxlo[channel] = sfx_ptr & 0xff;
    mt_chnsfxhi[channel] = sfx_ptr >> 8;
}

void execute_sfx(uint8_t channel) {
    uint16_t ptr = (mt_chnsfxhi[channel] << 8) | mt_chnsfxlo[channel];
    uint8_t frame = mt_chnsfx[channel];

    mt_chngate[channel] = 0xfe;
    mt_chnwaveptr[channel] = 0;  // Stop wavetable
    mt_chnsfx[channel]++;

    if (frame == 1) {
        // Frame 0: Hard restart
        SIDBASE[0x05 + channel] = 0;
        SIDBASE[0x06 + channel] = 0;
        goto load_freq;
    }

    if (frame == 2) {
        // Frame 1: Load ADSR and pulse
        SIDBASE[0x05 + channel] = ptr[0];  // AD
        SIDBASE[0x06 + channel] = ptr[1];  // SR
        SIDBASE[0x02 + channel] = ptr[2];  // Pulse
        SIDBASE[0x03 + channel] = ptr[2];  // Pulse (simplified)

        mt_chnwave[channel] = 0x09;  // Test bit
        SIDBASE[0x04 + channel] = 0x09;
        return;
    }

    // Frame 2+: Read note
    uint8_t note = ptr[frame - 1];
    if (note == 0) {
        // End of SFX
        mt_chnsfx[channel] = 0;
        mt_chnwave[channel] = 0x09;
        SIDBASE[0x04 + channel] = 0x09;
        return;
    }

    // Load frequency
    SIDBASE[0x00 + channel] = mt_freqtbllo[note - 0x80];
    SIDBASE[0x01 + channel] = mt_freqtblhi[note - 0x80];

    // Check if waveform follows
    note = ptr[frame];
    if (note == 0 || note >= 0x82) {
        return;  // No waveform or another note
    }

    // Load waveform
    mt_chnsfx[channel]++;
    mt_chnwave[channel] = note;
    SIDBASE[0x04 + channel] = note;
}
```

**SFX Format**:
```
Byte 0: Attack/Decay
Byte 1: Sustain/Release
Byte 2: Pulse width
Byte 3+: Note ($80-$FF) or waveform ($00-$81)
End with $00
```

---

## VIII. CRITICAL TIMING NOTES

### Frame Timing
- **MUST be called at 50Hz (PAL) or 60Hz (NTSC)**
- One call to `mt_play()` per frame
- Typical: IRQ or VBlank interrupt

### Gate Timing
- **Critical for percussion**: Gate must go 0→1 for note to trigger
- Hard restart: Set AD=0, SR=0 briefly to reset envelope
- `mt_chngatetimer` controls when gate goes off (from instrument)

### Effect Timing
- **Tick 0 effects**: Execute once when note starts
- **Continuous effects**: Execute every tick (portamento, vibrato)
- Speed parameters control effect intensity

### Filter Sharing
- **Only one filter** for all 3 channels
- Filter table can route different channels through filter
- Filter control byte: bits 0-2 = channel routing, bits 4-7 = resonance

---

## IX. OPTIMIZATION OPPORTUNITIES FOR C

### 1. Replace Indexed Addressing with Structures
```c
// Instead of: mt_chnfreqlo[x]
// Use:
struct Channel {
    uint16_t frequency;
    uint16_t pulse;
    uint8_t waveform;
    // ...
} channels[3];
```

### 2. Use Switch Statements for Effect Dispatch
```c
switch(effect_number) {
    case FX_PORTAUP: handle_portaup(); break;
    case FX_VIBRATO: handle_vibrato(); break;
    // ...
}
```

### 3. Use Bitfields for Flags
```c
struct ChannelFlags {
    uint8_t gate_on : 1;
    uint8_t sfx_active : 1;
    uint8_t new_note : 1;
};
```

### 4. Const Tables in Flash
```c
const uint8_t effect_jump_table[16] PROGMEM = {
    // ...
};
```

### 5. Inline Small Functions
```c
static inline void write_sid_frequency(uint8_t channel, uint16_t freq) {
    SID_BASE[channel * 7 + 0] = freq & 0xff;
    SID_BASE[channel * 7 + 1] = freq >> 8;
}
```

---

## X. NEXT STEPS FOR REWRITE

1. **Define C structures** for all data types
2. **Create parser** for music data format (or use existing)
3. **Write initialization** function
4. **Implement tick counter** and tempo system
5. **Implement sequencer** (order list processing)
6. **Implement pattern decoder**
7. **Implement wavetable engine**
8. **Implement pulse table engine**
9. **Implement filter engine**
10. **Implement each effect handler**
11. **Implement SFX system**
12. **Test and optimize**

---

## XI. DATA STRUCTURE DEFINITIONS

Here are proposed C structures for the rewrite:

```c
// Music data pointers (generated by relocator)
typedef struct {
    const uint8_t* freq_table_lo;
    const uint8_t* freq_table_hi;
    const uint8_t* order_lists[3];      // One per channel
    const uint8_t* pattern_ptrs_lo;
    const uint8_t* pattern_ptrs_hi;
    const uint8_t* instrument_data;
    const uint8_t* wave_table;
    const uint8_t* note_table;
    const uint8_t* pulse_table_time;
    const uint8_t* pulse_table_speed;
    const uint8_t* filter_table_time;
    const uint8_t* filter_table_speed;
    const uint8_t* speed_table_left;
    const uint8_t* speed_table_right;
} MusicData;

// Per-instrument data
typedef struct {
    uint8_t gate_timer;
    uint8_t first_wave;
    uint8_t pulse_ptr;
    uint8_t filter_ptr;
    uint8_t wave_ptr;
    uint8_t attack_decay;
    uint8_t sustain_release;
    uint8_t vib_delay;
    uint8_t vib_param;
} Instrument;

// Per-channel state
typedef struct {
    // Sequencer state
    uint8_t song_ptr;
    int8_t transpose;
    uint8_t repeat_count;
    uint8_t pattern_ptr;
    uint8_t packed_rest;

    // Current note state
    uint8_t pattern_num;
    uint8_t instrument;
    uint8_t note;
    uint8_t gate;           // 0xff = on, 0xfe = off

    // Effect state
    uint8_t effect_num;
    uint8_t effect_param;
    uint8_t new_effect;
    uint8_t new_param;
    uint8_t new_note;

    // Timing
    uint8_t tempo;
    int8_t counter;         // Can go negative for funk tempo
    uint8_t gate_timer;

    // Wavetable
    uint8_t wave_ptr;
    uint8_t wave_time;
    uint8_t waveform;

    // Vibrato
    int8_t vib_time;
    uint8_t vib_delay;
    uint8_t last_note;      // For calculated speed

    // Current SID state
    uint16_t frequency;
    uint16_t pulse;
    uint8_t attack_decay;
    uint8_t sustain_release;

    // Pulse table
    uint8_t pulse_ptr;
    uint8_t pulse_time;

    // Sound effects
    uint8_t sfx_active;
    const uint8_t* sfx_ptr;
} Channel;

// Global filter state
typedef struct {
    uint8_t step_ptr;
    uint8_t mod_time;
    uint8_t cutoff;
    uint8_t control;
    uint8_t type;
} Filter;

// Global player state
typedef struct {
    Channel channels[3];
    Filter filter;
    uint8_t master_volume;
    uint8_t funk_tempo[2];
    int8_t init_song_num;   // Negative = init pending
} Player;
```

---

## XII. PSEUDO-CODE MAIN LOOP

```c
void player_play(Player* player, const MusicData* music) {
    // Handle initialization
    if (player->init_song_num < 0) {
        player_initialize(player, music, -player->init_song_num);
        player->init_song_num = 0;
    }

    // Execute filter
    execute_filter(&player->filter, music);

    // Execute all channels
    for (int ch = 0; ch < 3; ch++) {
        execute_channel(&player->channels[ch], music, ch);
    }
}

void execute_channel(Channel* chan, const MusicData* music, int ch_num) {
    // Tick countdown
    chan->counter--;

    if (chan->counter > 0) {
        goto execute_effects;
    }

    if (chan->counter < 0) {
        // Reload tempo
        uint8_t tempo = chan->tempo;
        if (tempo < 2) {
            tempo = player->funk_tempo[tempo];
            chan->tempo ^= 1;  // Alternate
        }
        chan->counter = tempo;
    }

    // Tick 0 - fetch new note
    if (chan->counter == 0) {
        if (chan->pattern_ptr == 0) {
            execute_sequencer(chan, music);
        }

        if (chan->new_note != 0) {
            initialize_note(chan, music);
        } else {
            execute_tick0_effect(chan, music);
        }
    }

execute_effects:
    execute_wavetable(chan, music);
    execute_continuous_effect(chan, music);
    execute_pulse_table(chan, music);

    // Check gate timer
    if (chan->counter == chan->gate_timer) {
        fetch_pattern_data(chan, music);
    }

    // Check for sound effect override
    if (chan->sfx_active) {
        execute_sound_effect(chan, music, ch_num);
    } else {
        write_sid_registers(chan, ch_num);
    }
}
```

---

This documentation should provide a solid foundation for the C rewrite!
