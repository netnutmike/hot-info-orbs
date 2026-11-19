// Host tests for ForgetCountdown.h (issue #60):  pio test -e native -f test_forgetwifi
#include "ForgetCountdown.h"
#include <unity.h>

void setUp() {}
void tearDown() {}

using forgetwifi::Countdown;
using forgetwifi::kCountMs;
using forgetwifi::kLetGoMs;
using forgetwifi::State;

// Held the whole way, read every 10 ms as the firmware does
static State holdUntil(Countdown &c, uint32_t from, uint32_t to) {
    State s = State::Counting;
    for (uint32_t t = from; t <= to && s == State::Counting; t += 10) {
        s = c.step(t, true);
    }
    return s;
}

void test_held_to_the_end_forgets() {
    Countdown c(1000);
    TEST_ASSERT_TRUE(holdUntil(c, 1000, 1000 + kCountMs - 10) == State::Counting);
    TEST_ASSERT_TRUE(c.step(1000 + kCountMs, true) == State::Forget);
}

void test_let_go_early_keeps_the_wifi() {
    Countdown c(0);
    holdUntil(c, 0, 1500);
    TEST_ASSERT_TRUE(c.step(1510, false) == State::Counting); // could be bounce
    TEST_ASSERT_TRUE(c.step(1510 + kLetGoMs, false) == State::Kept);
}

// A finger still pressing, the contact chattering: not a let-go
void test_bounce_does_not_cancel() {
    Countdown c(0);
    holdUntil(c, 0, 1000);
    TEST_ASSERT_TRUE(c.step(1010, false) == State::Counting);
    TEST_ASSERT_TRUE(c.step(1020, true) == State::Counting);
    TEST_ASSERT_TRUE(c.step(1030, false) == State::Counting);
    TEST_ASSERT_TRUE(c.step(1030 + kLetGoMs - 1, false) == State::Counting);
    TEST_ASSERT_TRUE(c.step(1030 + kLetGoMs, true) == State::Counting);
    TEST_ASSERT_TRUE(holdUntil(c, 1100, kCountMs) == State::Forget);
}

// Bouncing up just as the time runs out: wait for the button to settle
void test_up_at_the_end_waits_to_see() {
    Countdown c(0);
    holdUntil(c, 0, kCountMs - 10);
    TEST_ASSERT_TRUE(c.step(kCountMs, false) == State::Counting);
    TEST_ASSERT_TRUE(c.step(kCountMs + 10, true) == State::Forget);
}

void test_let_go_right_at_the_end_keeps() {
    Countdown c(0);
    holdUntil(c, 0, kCountMs - 10);
    c.step(kCountMs, false);
    TEST_ASSERT_TRUE(c.step(kCountMs + kLetGoMs, false) == State::Kept);
}

void test_the_count_shows_3_2_1() {
    Countdown c(500);
    TEST_ASSERT_EQUAL(3, c.secondsLeft(500));
    TEST_ASSERT_EQUAL(3, c.secondsLeft(500 + 999));
    TEST_ASSERT_EQUAL(2, c.secondsLeft(500 + 1000));
    TEST_ASSERT_EQUAL(2, c.secondsLeft(500 + 1999));
    TEST_ASSERT_EQUAL(1, c.secondsLeft(500 + 2000));
    TEST_ASSERT_EQUAL(1, c.secondsLeft(500 + 2999));
    TEST_ASSERT_EQUAL(0, c.secondsLeft(500 + 3000));
}

void test_holds_across_the_counter_wrapping() {
    const uint32_t start = 0xFFFFF800u; // about 2 s before the wrap
    Countdown c(start);
    TEST_ASSERT_TRUE(c.step(start + 2000, true) == State::Counting);
    TEST_ASSERT_EQUAL(1, c.secondsLeft(start + 2000));
    TEST_ASSERT_TRUE(c.step(start + kCountMs, true) == State::Forget);
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_held_to_the_end_forgets);
    RUN_TEST(test_let_go_early_keeps_the_wifi);
    RUN_TEST(test_bounce_does_not_cancel);
    RUN_TEST(test_up_at_the_end_waits_to_see);
    RUN_TEST(test_let_go_right_at_the_end_keeps);
    RUN_TEST(test_the_count_shows_3_2_1);
    RUN_TEST(test_holds_across_the_counter_wrapping);
    return UNITY_END();
}
