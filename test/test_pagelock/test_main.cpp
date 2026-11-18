// Host tests for PageLock.h (issue #58):  pio test -e native -f test_pagelock
#include "PageLock.h"
#include <unity.h>

void setUp() {}
void tearDown() {}

using pagelock::Event;
using pagelock::kHoldMs;
using pagelock::kNoteMs;
using pagelock::PageLock;
using pagelock::Step;

// Both up once, as on any orb that has been running a moment
static PageLock armed() {
    PageLock p;
    p.step(0, false, false);
    return p;
}

// --- the hold ---

void test_both_held_2_s_locks() {
    PageLock p = armed();
    TEST_ASSERT_TRUE(p.step(100, true, true).event == Event::None);
    TEST_ASSERT_TRUE(p.step(100 + kHoldMs - 1, true, true).event == Event::None);
    TEST_ASSERT_TRUE(p.step(100 + kHoldMs, true, true).event == Event::Locked);
    TEST_ASSERT_TRUE(p.locked());
}

void test_the_same_hold_again_unlocks() {
    PageLock p = armed();
    p.step(100, true, true);
    p.step(100 + kHoldMs, true, true);
    p.step(3000, false, false);
    p.step(4000, true, true);
    TEST_ASSERT_TRUE(p.step(4000 + kHoldMs, true, true).event == Event::Unlocked);
    TEST_ASSERT_FALSE(p.locked());
}

void test_one_hold_gives_one_event_however_long() {
    PageLock p = armed();
    p.step(100, true, true);
    TEST_ASSERT_TRUE(p.step(100 + kHoldMs, true, true).event == Event::Locked);
    TEST_ASSERT_TRUE(p.step(100 + 3 * kHoldMs, true, true).event == Event::None);
    TEST_ASSERT_TRUE(p.locked());
}

void test_the_clock_starts_when_the_second_button_goes_down() {
    PageLock p = armed();
    p.step(100, true, false);
    p.step(1500, true, true);
    TEST_ASSERT_TRUE(p.step(100 + kHoldMs, true, true).event == Event::None);
    TEST_ASSERT_TRUE(p.step(1500 + kHoldMs, true, true).event == Event::Locked);
}

void test_letting_one_go_early_does_nothing() {
    PageLock p = armed();
    p.step(100, true, true);
    p.step(1000, true, false);
    TEST_ASSERT_TRUE(p.step(100 + kHoldMs, true, false).event == Event::None);
    TEST_ASSERT_FALSE(p.locked());
}

void test_pressing_the_second_again_restarts_the_clock() {
    PageLock p = armed();
    p.step(100, true, true);
    p.step(1000, true, false);
    p.step(1200, true, true);
    TEST_ASSERT_TRUE(p.step(100 + kHoldMs, true, true).event == Event::None);
    TEST_ASSERT_TRUE(p.step(1200 + kHoldMs, true, true).event == Event::Locked);
}

// --- power-up: holding right at power-up wipes WiFi; a hold carried over
// from power-up must never lock ---

void test_a_hold_from_power_up_never_locks() {
    PageLock p;
    p.step(0, true, true);
    TEST_ASSERT_TRUE(p.step(5000, true, true).event == Event::None);
    TEST_ASSERT_FALSE(p.locked());
}

void test_a_hold_from_power_up_is_not_swallowed() {
    PageLock p;
    TEST_ASSERT_FALSE(p.step(0, true, true).swallow);
    TEST_ASSERT_FALSE(p.step(5000, false, false).swallow);
}

void test_after_letting_go_a_power_up_hold_the_next_hold_works() {
    PageLock p;
    p.step(0, true, true);
    p.step(5000, false, false);
    p.step(6000, true, true);
    TEST_ASSERT_TRUE(p.step(6000 + kHoldMs, true, true).event == Event::Locked);
}

void test_one_button_held_from_power_up_still_blocks_until_both_are_up() {
    PageLock p;
    p.step(0, false, true);
    p.step(500, true, true);
    TEST_ASSERT_TRUE(p.step(500 + kHoldMs, true, true).event == Event::None);
}

// --- swallowing: the two buttons in a hold must do nothing else, the
// release included ---

void test_both_down_is_swallowed() {
    PageLock p = armed();
    TEST_ASSERT_TRUE(p.step(100, true, true).swallow);
}

void test_the_release_pass_is_swallowed_then_it_stops() {
    PageLock p = armed();
    p.step(100, true, true);
    p.step(100 + kHoldMs, true, true);
    TEST_ASSERT_TRUE(p.step(2500, true, false).swallow);
    TEST_ASSERT_TRUE(p.step(2600, false, false).swallow);
    TEST_ASSERT_FALSE(p.step(2700, false, false).swallow);
}

void test_an_early_let_go_is_swallowed_too() {
    PageLock p = armed();
    p.step(100, true, true);
    TEST_ASSERT_TRUE(p.step(500, false, true).swallow);
    TEST_ASSERT_TRUE(p.step(600, false, false).swallow);
    TEST_ASSERT_FALSE(p.step(700, false, false).swallow);
}

void test_one_button_alone_is_not_swallowed() {
    PageLock p = armed();
    TEST_ASSERT_FALSE(p.step(100, true, false).swallow);
    TEST_ASSERT_FALSE(p.step(200, false, false).swallow);
    TEST_ASSERT_FALSE(p.step(300, false, true).swallow);
}

// --- the Locked / Unlocked note ---

void test_the_note_shows_after_an_event_and_ends_once() {
    PageLock p = armed();
    p.step(100, true, true);
    p.step(100 + kHoldMs, true, true);
    const uint32_t at = 100 + kHoldMs;
    TEST_ASSERT_TRUE(p.noteShowing());
    TEST_ASSERT_FALSE(p.noteEnded(at + kNoteMs - 1));
    TEST_ASSERT_TRUE(p.noteEnded(at + kNoteMs));
    TEST_ASSERT_FALSE(p.noteShowing());
    TEST_ASSERT_FALSE(p.noteEnded(at + kNoteMs + 1));
}

void test_no_note_before_any_event() {
    PageLock p = armed();
    TEST_ASSERT_FALSE(p.noteShowing());
    TEST_ASSERT_FALSE(p.noteEnded(100000));
}

void test_millis_wrap_does_not_break_the_hold() {
    PageLock p;
    const uint32_t start = 0xFFFFFF00u;
    p.step(start, false, false);
    p.step(start + 10, true, true);
    TEST_ASSERT_TRUE(p.step(start + 10 + kHoldMs, true, true).event == Event::Locked);
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_both_held_2_s_locks);
    RUN_TEST(test_the_same_hold_again_unlocks);
    RUN_TEST(test_one_hold_gives_one_event_however_long);
    RUN_TEST(test_the_clock_starts_when_the_second_button_goes_down);
    RUN_TEST(test_letting_one_go_early_does_nothing);
    RUN_TEST(test_pressing_the_second_again_restarts_the_clock);
    RUN_TEST(test_a_hold_from_power_up_never_locks);
    RUN_TEST(test_a_hold_from_power_up_is_not_swallowed);
    RUN_TEST(test_after_letting_go_a_power_up_hold_the_next_hold_works);
    RUN_TEST(test_one_button_held_from_power_up_still_blocks_until_both_are_up);
    RUN_TEST(test_both_down_is_swallowed);
    RUN_TEST(test_the_release_pass_is_swallowed_then_it_stops);
    RUN_TEST(test_an_early_let_go_is_swallowed_too);
    RUN_TEST(test_one_button_alone_is_not_swallowed);
    RUN_TEST(test_the_note_shows_after_an_event_and_ends_once);
    RUN_TEST(test_no_note_before_any_event);
    RUN_TEST(test_millis_wrap_does_not_break_the_hold);
    return UNITY_END();
}
