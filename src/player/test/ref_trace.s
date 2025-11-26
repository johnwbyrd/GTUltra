;===============================================================================
; Reference Trace Harness - Assembly Player Test
;===============================================================================
; This program runs the assembly player (player.s) with minimal test data
; and outputs SID register states tick-by-tick for comparison with C version.
;===============================================================================

; Basic setup for C64
.ORG $0801

; BASIC stub: SYS 2064
.BYTE $0B, $08, $0A, $00, $9E, $20, $32, $30, $36, $34, $00, $00, $00

; Program starts at $0810
.ORG $0810

; Constants
SID_BASE    = $D400
NUM_TICKS   = 50          ; Number of ticks to capture
TRACE_SIZE  = 25          ; SID has 25 registers ($D400-$D418)

;===============================================================================
; Main Program
;===============================================================================

start:
        ; Initialize player with song 0
        lda #$00
        jsr player_init

        ; Initialize trace buffer pointer
        lda #<trace_buffer
        sta trace_ptr
        lda #>trace_buffer
        sta trace_ptr+1

        ; Tick counter
        lda #NUM_TICKS
        sta tick_count

main_loop:
        ; Call player
        jsr player_play

        ; Copy SID registers to trace buffer
        jsr capture_sid_state

        ; Decrement tick counter
        dec tick_count
        bne main_loop

        ; Output trace
        jsr output_trace

        ; Halt
        jmp *

;===============================================================================
; Capture SID State
;===============================================================================
; Copies all 25 SID registers ($D400-$D418) to trace buffer

capture_sid_state:
        ldx #$00
copy_loop:
        lda SID_BASE,x
        ldy #$00
        sta (trace_ptr),y

        ; Increment buffer pointer
        inc trace_ptr
        bne :+
        inc trace_ptr+1
:
        inx
        cpx #TRACE_SIZE
        bne copy_loop
        rts

;===============================================================================
; Output Trace
;===============================================================================
; Outputs trace buffer to screen in hex format
; Format: TICK:nnnn D400:xx D401:xx ... D418:xx

output_trace:
        ; Reset buffer pointer
        lda #<trace_buffer
        sta trace_ptr
        lda #>trace_buffer
        sta trace_ptr+1

        ; Initialize tick number
        lda #$00
        sta current_tick

output_loop:
        ; Check if done
        lda current_tick
        cmp #NUM_TICKS
        beq output_done

        ; Output "TICK:"
        lda #<msg_tick
        sta $FB
        lda #>msg_tick
        sta $FC
        jsr print_string

        ; Output tick number (2 hex digits)
        lda current_tick
        jsr print_hex_byte

        ; Output space
        lda #$20
        jsr $FFD2

        ; Output 25 register values
        ldx #$00
reg_loop:
        ; Output register name "Dxxx:"
        txa
        clc
        adc #$00        ; Base is $D400
        jsr print_reg_name

        ; Output register value
        ldy #$00
        lda (trace_ptr),y
        jsr print_hex_byte

        ; Space between registers
        lda #$20
        jsr $FFD2

        ; Increment buffer pointer
        inc trace_ptr
        bne :+
        inc trace_ptr+1
:
        inx
        cpx #TRACE_SIZE
        bne reg_loop

        ; Newline
        lda #$0D
        jsr $FFD2

        ; Next tick
        inc current_tick
        jmp output_loop

output_done:
        rts

;===============================================================================
; Print Register Name
;===============================================================================
; Input: A = register offset (0-24)
; Prints "D4xx:" where xx is the hex offset

print_reg_name:
        pha
        lda #$44        ; 'D'
        jsr $FFD2
        lda #$34        ; '4'
        jsr $FFD2
        pla
        jsr print_hex_byte
        lda #$3A        ; ':'
        jsr $FFD2
        rts

;===============================================================================
; Print Hex Byte
;===============================================================================
; Input: A = byte to print
; Prints as 2-digit hex

print_hex_byte:
        pha
        lsr
        lsr
        lsr
        lsr
        jsr print_hex_nybble
        pla
        and #$0F
        jsr print_hex_nybble
        rts

print_hex_nybble:
        cmp #$0A
        bcc is_digit
        ; A-F
        clc
        adc #$07
is_digit:
        clc
        adc #$30
        jmp $FFD2

;===============================================================================
; Print String
;===============================================================================
; Input: $FB/$FC = pointer to null-terminated string

print_string:
        ldy #$00
:       lda ($FB),y
        beq :+
        jsr $FFD2
        iny
        bne :-
:       rts

;===============================================================================
; Data
;===============================================================================

msg_tick:
        .TEXT "TICK:"
        .BYTE $00

; Variables
trace_ptr:      .WORD $0000
tick_count:     .BYTE $00
current_tick:   .BYTE $00

;===============================================================================
; Minimal Player Data
;===============================================================================
; This is minimal test data: single note pattern, simple instrument

; Frequency table (just C-4 for testing)
freq_table_lo:
        .BYTE $7E, $00  ; C-4 = $107E
freq_table_hi:
        .BYTE $10, $00

; Order list for channel 0 (play pattern 0, then loop)
order_list_0:
        .BYTE $00       ; Pattern 0
        .BYTE $FF, $00  ; Loop to start

; Order list pointers
order_list_ptrs_lo:
        .BYTE <order_list_0, <order_list_0, <order_list_0
order_list_ptrs_hi:
        .BYTE >order_list_0, >order_list_0, >order_list_0

; Pattern 0: Single note (C-4 with instrument 1)
pattern_0:
        .BYTE $01       ; Instrument 1
        .BYTE $60+48    ; Note C-4
        .BYTE $BD       ; Rest
        .BYTE $00       ; End of pattern

; Pattern table
pattern_ptrs_lo:
        .BYTE <pattern_0
pattern_ptrs_hi:
        .BYTE >pattern_0

; Instrument 1 (simple test instrument)
instr_ad:        .BYTE $09          ; Attack=0, Decay=9
instr_sr:        .BYTE $F0          ; Sustain=F, Release=0
instr_waveptr:   .BYTE $00          ; No wavetable
instr_pulseptr:  .BYTE $00          ; No pulse table
instr_filtptr:   .BYTE $00          ; No filter table
instr_gatetimer: .BYTE $08          ; Gate timer
instr_firstwave: .BYTE $41          ; Triangle waveform
instr_vibdelay:  .BYTE $00          ; No vibrato
instr_vibparam:  .BYTE $00

; Empty tables (required but unused)
wave_table:      .BYTE $00
note_table:      .BYTE $00
pulse_time_tbl:  .BYTE $00
pulse_speed_tbl: .BYTE $00
filt_time_tbl:   .BYTE $00
filt_speed_tbl:  .BYTE $00
speed_left_tbl:  .BYTE $00
speed_right_tbl: .BYTE $00

;===============================================================================
; Player Include
;===============================================================================
; TODO: Include player.s here
; This requires setting up proper defines first
; For now, this is a placeholder

player_init:
        ; Placeholder - will include actual player code
        rts

player_play:
        ; Placeholder - will include actual player code
        rts

;===============================================================================
; Trace Buffer (25 bytes * 50 ticks = 1250 bytes)
;===============================================================================

trace_buffer:
        .FILL (NUM_TICKS * TRACE_SIZE), $00
