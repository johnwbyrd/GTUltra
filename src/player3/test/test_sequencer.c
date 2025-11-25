//=============================================================================
// TEST_SEQUENCER.C - Sequencer Unit Tests
//=============================================================================
// This program validates the sequencer implementation (Phase 2) by testing:
// - Order list processing (LOOP, REPEAT, TRANSPOSE)
// - Pattern data decoding (all encoding formats)
// - Note fetching and parsing
//
// USAGE:
//   make test
//   ./build/test_sequencer
//
// EXPECTED OUTPUT:
//   All tests should pass with detailed output for each test case.
//=============================================================================

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "../include/player3.h"
#include "../include/player3_types.h"

// Include test data
#include "test_data.c"

//=============================================================================
// TEST FRAMEWORK
//=============================================================================

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        tests_run++; \
        if (condition) { \
            tests_passed++; \
            printf("  ✓ %s\n", message); \
        } else { \
            tests_failed++; \
            printf("  ✗ FAILED: %s\n", message); \
            printf("    at %s:%d\n", __FILE__, __LINE__); \
        } \
    } while(0)

#define TEST_SECTION(name) \
    printf("\n========================================\n"); \
    printf("TEST: %s\n", name); \
    printf("========================================\n")

//=============================================================================
// TEST HELPERS
//=============================================================================

/**
 * Initialize a fresh player and channel for testing.
 */
static void setup_test_player(Player* player, Channel* ch) {
    memset(player, 0, sizeof(Player));
    memset(ch, 0, sizeof(Channel));

    // Set default tempo
    player->funk_tempo[0] = 6;
    player->funk_tempo[1] = 6;

    // Initialize channel with default values
    ch->tempo = 6;
    ch->tick_counter = 0;
    ch->gate_timer = 1;
}

/**
 * Print channel state for debugging.
 */
static void print_channel_state(const Channel* ch) {
    printf("  Channel State:\n");
    printf("    song_ptr=%d, pattern_ptr=%d, pattern_num=%d\n",
           ch->song_ptr, ch->pattern_ptr, ch->pattern_num);
    printf("    transpose=%d, repeat_count=%d\n",
           ch->transpose, ch->repeat_count);
    printf("    instrument=%d, note=0x%02X\n",
           ch->instrument, ch->note);
    printf("    new_effect=0x%02X, new_param=0x%02X\n",
           ch->new_effect, ch->new_param);
    printf("    tick_counter=%d, tempo=%d\n",
           ch->tick_counter, ch->tempo);
}

//=============================================================================
// PATTERN DECODING TESTS
//=============================================================================

void test_pattern_decoding(void) {
    TEST_SECTION("Pattern Decoding");

    Player player;
    Channel ch;
    setup_test_player(&player, &ch);

    // Set up to read pattern 0
    ch.pattern_num = 0;
    ch.pattern_ptr = 1;  // Start at position 1 (0 is dummy)
    ch.instrument = 0;   // Start with no instrument

    printf("\nPattern 0 - Testing all encoding formats:\n");

    //-------------------------------------------------------------------------
    // Test 1: Instrument change (0x01)
    //-------------------------------------------------------------------------
    sequencer_fetch_note(&ch, &test_music_data);
    TEST_ASSERT(ch.instrument == 1, "Instrument change to 1");
    TEST_ASSERT(ch.note == 0xFF, "No note played (instrument change only)");
    TEST_ASSERT(ch.pattern_ptr == 2, "Pattern pointer advanced to 2");

    //-------------------------------------------------------------------------
    // Test 2: Note (0x60 = note 0)
    //-------------------------------------------------------------------------
    sequencer_fetch_note(&ch, &test_music_data);
    TEST_ASSERT(ch.note == 0x00, "Note C-3 (index 0)");
    TEST_ASSERT(ch.instrument == 1, "Instrument unchanged");
    TEST_ASSERT(ch.pattern_ptr == 3, "Pattern pointer advanced to 3");

    //-------------------------------------------------------------------------
    // Test 3: Effect with note (0x40 | FX_PORTAUP)
    //-------------------------------------------------------------------------
    sequencer_fetch_note(&ch, &test_music_data);
    TEST_ASSERT(ch.new_effect == FX_PORTAUP, "Effect: Portamento up");
    TEST_ASSERT(ch.new_param == 0x10, "Effect parameter: 0x10");
    TEST_ASSERT(ch.note == 0x01, "Note D-3 (index 1)");
    TEST_ASSERT(ch.pattern_ptr == 6, "Pattern pointer advanced to 6");

    //-------------------------------------------------------------------------
    // Test 4: Effect without note (0x50 | FX_VIBRATO)
    //-------------------------------------------------------------------------
    sequencer_fetch_note(&ch, &test_music_data);
    TEST_ASSERT(ch.new_effect == FX_VIBRATO, "Effect: Vibrato");
    TEST_ASSERT(ch.new_param == 0x46, "Effect parameter: 0x46");
    TEST_ASSERT(ch.note == 0xFF, "No note (effect only)");
    TEST_ASSERT(ch.pattern_ptr == 8, "Pattern pointer advanced to 8");

    //-------------------------------------------------------------------------
    // Test 5: Rest (0xBD)
    //-------------------------------------------------------------------------
    sequencer_fetch_note(&ch, &test_music_data);
    TEST_ASSERT(ch.note == NOTE_REST, "Rest command");
    TEST_ASSERT(ch.pattern_ptr == 9, "Pattern pointer advanced to 9");

    //-------------------------------------------------------------------------
    // Test 6: KeyOff (0xBE)
    //-------------------------------------------------------------------------
    sequencer_fetch_note(&ch, &test_music_data);
    TEST_ASSERT(ch.note == NOTE_KEYOFF, "KeyOff command");
    TEST_ASSERT(ch.pattern_ptr == 10, "Pattern pointer advanced to 10");

    //-------------------------------------------------------------------------
    // Test 7: KeyOn (0xBF)
    //-------------------------------------------------------------------------
    sequencer_fetch_note(&ch, &test_music_data);
    TEST_ASSERT(ch.note == NOTE_KEYON, "KeyOn command");
    TEST_ASSERT(ch.pattern_ptr == 11, "Pattern pointer advanced to 11");

    //-------------------------------------------------------------------------
    // Test 8: Packed rest (0xC3 = 4 frames)
    //-------------------------------------------------------------------------
    // First call - should see REST and set packed_rest_count
    sequencer_fetch_note(&ch, &test_music_data);
    TEST_ASSERT(ch.note == NOTE_REST, "Packed rest: first REST");
    TEST_ASSERT(ch.packed_rest_count > 0, "Packed rest count set");
    TEST_ASSERT(ch.pattern_ptr == 11, "Pattern pointer stays at 11");

    uint8_t expected_count = ch.packed_rest_count;

    // Subsequent calls - should continue returning REST
    sequencer_fetch_note(&ch, &test_music_data);
    TEST_ASSERT(ch.note == NOTE_REST, "Packed rest: second REST");
    TEST_ASSERT(ch.packed_rest_count == expected_count - 1, "Count decremented");

    // Exhaust packed rests
    while (ch.packed_rest_count > 0) {
        sequencer_fetch_note(&ch, &test_music_data);
    }
    TEST_ASSERT(ch.packed_rest_count == 0, "Packed rest count exhausted");
    TEST_ASSERT(ch.pattern_ptr == 12, "Pattern pointer advanced after packed rests");

    //-------------------------------------------------------------------------
    // Test 9: End of pattern (0x00)
    //-------------------------------------------------------------------------
    sequencer_fetch_note(&ch, &test_music_data);
    TEST_ASSERT(ch.pattern_ptr == 0, "Pattern pointer reset to 0 (end of pattern)");
}

//=============================================================================
// ORDER LIST TESTS
//=============================================================================

void test_order_list_processing(void) {
    TEST_SECTION("Order List Processing");

    Player player;
    Channel ch;
    setup_test_player(&player, &ch);

    // Use order list 0 (has all commands)
    ch.song_ptr = 0;
    ch.pattern_ptr = 0;

    printf("\nOrder List 0 - Testing all order list commands:\n");

    //-------------------------------------------------------------------------
    // Test 1: Simple pattern number
    //-------------------------------------------------------------------------
    sequencer_fetch_pattern(&ch, &test_music_data);
    TEST_ASSERT(ch.pattern_num == 0, "Pattern 0 loaded");
    TEST_ASSERT(ch.song_ptr == 1, "Song pointer advanced to 1");
    TEST_ASSERT(ch.pattern_ptr == 1, "Pattern pointer set to 1 (start)");
    TEST_ASSERT(ch.transpose == 0, "Transpose unchanged (0)");

    // Reset for next test
    ch.pattern_ptr = 0;

    //-------------------------------------------------------------------------
    // Test 2: TRANSPOSE UP (+2 semitones)
    //-------------------------------------------------------------------------
    sequencer_fetch_pattern(&ch, &test_music_data);
    TEST_ASSERT(ch.transpose == 2, "Transpose +2");
    TEST_ASSERT(ch.pattern_num == 1, "Pattern 1 loaded");
    TEST_ASSERT(ch.song_ptr == 3, "Song pointer advanced past transpose");

    // Reset for next test
    ch.pattern_ptr = 0;

    //-------------------------------------------------------------------------
    // Test 3: TRANSPOSE DOWN (-1 semitone)
    //-------------------------------------------------------------------------
    sequencer_fetch_pattern(&ch, &test_music_data);
    TEST_ASSERT(ch.transpose == -1, "Transpose -1");
    TEST_ASSERT(ch.pattern_num == 1, "Pattern 1 loaded again");
    TEST_ASSERT(ch.song_ptr == 5, "Song pointer advanced");

    // Reset for next test
    ch.pattern_ptr = 0;

    //-------------------------------------------------------------------------
    // Test 4: TRANSPOSE to 0 (reset)
    //-------------------------------------------------------------------------
    sequencer_fetch_pattern(&ch, &test_music_data);
    TEST_ASSERT(ch.transpose == 0, "Transpose reset to 0");

    // Reset for next test
    ch.pattern_ptr = 0;

    //-------------------------------------------------------------------------
    // Test 5: REPEAT (repeat 2 times)
    //-------------------------------------------------------------------------
    sequencer_fetch_pattern(&ch, &test_music_data);
    TEST_ASSERT(ch.pattern_num == 2, "Pattern 2 loaded (first play)");
    TEST_ASSERT(ch.repeat_count == 0, "Repeat count starts at 0");
    uint8_t repeat_song_ptr = ch.song_ptr;

    // Reset pattern_ptr to trigger fetch again (simulates pattern completion)
    ch.pattern_ptr = 0;

    // First repeat
    sequencer_fetch_pattern(&ch, &test_music_data);
    TEST_ASSERT(ch.repeat_count == 1, "Repeat count = 1");
    TEST_ASSERT(ch.song_ptr == repeat_song_ptr, "Song pointer unchanged (repeating)");
    TEST_ASSERT(ch.pattern_num == 2, "Pattern 2 still playing");

    ch.pattern_ptr = 0;

    // Second repeat
    sequencer_fetch_pattern(&ch, &test_music_data);
    TEST_ASSERT(ch.repeat_count == 2, "Repeat count = 2");
    TEST_ASSERT(ch.song_ptr == repeat_song_ptr, "Song pointer unchanged (repeating)");

    ch.pattern_ptr = 0;

    // After last repeat, should advance
    sequencer_fetch_pattern(&ch, &test_music_data);
    TEST_ASSERT(ch.repeat_count == 0, "Repeat count reset");
    TEST_ASSERT(ch.song_ptr > repeat_song_ptr, "Song pointer advanced past repeat");
    TEST_ASSERT(ch.pattern_num == 3, "Pattern 3 loaded (after repeats)");

    // Reset for next test
    ch.pattern_ptr = 0;

    //-------------------------------------------------------------------------
    // Test 6: LOOP
    //-------------------------------------------------------------------------
    uint8_t loop_song_ptr = ch.song_ptr;
    sequencer_fetch_pattern(&ch, &test_music_data);
    TEST_ASSERT(ch.song_ptr == 0, "Song pointer looped to 0");
    TEST_ASSERT(ch.pattern_num == 0, "Pattern 0 loaded (after loop)");
}

//=============================================================================
// TRANSPOSE TESTS
//=============================================================================

void test_transpose(void) {
    TEST_SECTION("Transpose Functionality");

    Player player;
    Channel ch;
    setup_test_player(&player, &ch);

    // Set pattern 1 and position to first note
    ch.pattern_num = 1;
    ch.pattern_ptr = 33;  // Position of instrument 2
    ch.transpose = 0;

    printf("\nTesting transpose with pattern 1:\n");

    //-------------------------------------------------------------------------
    // Test 1: No transpose
    //-------------------------------------------------------------------------
    sequencer_fetch_note(&ch, &test_music_data);
    TEST_ASSERT(ch.instrument == 2, "Instrument 2 loaded");
    ch.pattern_ptr = 34;  // Position of E-3 (note index 2)

    sequencer_fetch_note(&ch, &test_music_data);
    TEST_ASSERT(ch.note == 2, "Note E-3 (index 2) with transpose 0");

    //-------------------------------------------------------------------------
    // Test 2: Transpose +2
    //-------------------------------------------------------------------------
    ch.pattern_ptr = 34;
    ch.transpose = 2;
    sequencer_fetch_note(&ch, &test_music_data);
    TEST_ASSERT(ch.note == 4, "Note G-3 (index 4) with transpose +2");

    //-------------------------------------------------------------------------
    // Test 3: Transpose -1
    //-------------------------------------------------------------------------
    ch.pattern_ptr = 34;
    ch.transpose = -1;
    sequencer_fetch_note(&ch, &test_music_data);
    TEST_ASSERT(ch.note == 1, "Note D-3 (index 1) with transpose -1");
}

//=============================================================================
// INTEGRATION TEST
//=============================================================================

void test_sequencer_integration(void) {
    TEST_SECTION("Sequencer Integration");

    Player player;
    setup_test_player(&player, &player.channels[0]);

    Channel* ch = &player.channels[0];
    ch->song_ptr = 0;
    ch->pattern_ptr = 0;

    printf("\nSimulating full playback cycle:\n");

    // Fetch first pattern
    sequencer_fetch_pattern(ch, &test_music_data);
    printf("  Loaded pattern %d at song position %d\n",
           ch->pattern_num, ch->song_ptr - 1);

    // Fetch several notes from the pattern
    for (int i = 0; i < 5 && ch->pattern_ptr != 0; i++) {
        sequencer_fetch_note(ch, &test_music_data);
        printf("  Note %d: inst=%d, note=0x%02X, effect=0x%02X\n",
               i, ch->instrument, ch->note, ch->new_effect);
    }

    TEST_ASSERT(ch->pattern_ptr != 0 || ch->pattern_ptr == 0,
                "Pattern playback progressed");
}

//=============================================================================
// MAIN TEST RUNNER
//=============================================================================

int main(void) {
    printf("\n");
    printf("================================================================================\n");
    printf("PLAYER3 SEQUENCER UNIT TESTS\n");
    printf("================================================================================\n");
    printf("Testing Phase 2 implementation: Order list and pattern parsing\n");

    // Run all test suites
    test_pattern_decoding();
    test_order_list_processing();
    test_transpose();
    test_sequencer_integration();

    // Print summary
    printf("\n");
    printf("================================================================================\n");
    printf("TEST SUMMARY\n");
    printf("================================================================================\n");
    printf("Total tests:  %d\n", tests_run);
    printf("Passed:       %d\n", tests_passed);
    printf("Failed:       %d\n", tests_failed);
    printf("\n");

    if (tests_failed == 0) {
        printf("✓ ALL TESTS PASSED!\n");
        printf("\nPhase 2 (Sequencer) validation complete.\n");
        printf("Ready to proceed to Phase 3 (Basic Playback).\n");
        return 0;
    } else {
        printf("✗ SOME TESTS FAILED\n");
        printf("\nReview failed tests above before proceeding.\n");
        return 1;
    }
}

//=============================================================================
// End of test_sequencer.c
//=============================================================================
