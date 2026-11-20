// Host tests for PressPolicy.h (issue #60):  pio test -e native -f test_presspolicy
#include "PressPolicy.h"
#include <unity.h>

void setUp() {}
void tearDown() {}

using press::bounce;
using press::classify;
using press::Kind;

static const uint32_t kHold = 2000; // BUTTON_LONG_PRESS_TIME
static const uint32_t kDebounce = 35; // BUTTON_DEBOUNCE_TIME

// --- tap or hold ---

void test_a_quick_press_is_a_tap() {
    TEST_ASSERT_TRUE(classify(35, kHold) == Kind::Tap);
    TEST_ASSERT_TRUE(classify(120, kHold) == Kind::Tap);
}

// The bug #60 fixes: 0.5 to 2 s used to be "medium", and Left, Right, Weather,
// Stocks and MQTT ignored it
void test_a_slow_press_is_still_a_tap() {
    TEST_ASSERT_TRUE(classify(499, kHold) == Kind::Tap);
    TEST_ASSERT_TRUE(classify(500, kHold) == Kind::Tap);
    TEST_ASSERT_TRUE(classify(600, kHold) == Kind::Tap);
    TEST_ASSERT_TRUE(classify(1999, kHold) == Kind::Tap);
}

void test_2_s_or_more_is_a_hold() {
    TEST_ASSERT_TRUE(classify(2000, kHold) == Kind::Hold);
    TEST_ASSERT_TRUE(classify(2001, kHold) == Kind::Hold);
    TEST_ASSERT_TRUE(classify(60000, kHold) == Kind::Hold);
}

// --- debounce ---

void test_an_edge_inside_the_debounce_is_bounce() {
    TEST_ASSERT_TRUE(bounce(1000, 1000, kDebounce));
    TEST_ASSERT_TRUE(bounce(1034, 1000, kDebounce));
}

void test_an_edge_at_the_debounce_counts() {
    TEST_ASSERT_FALSE(bounce(1035, 1000, kDebounce));
    TEST_ASSERT_FALSE(bounce(5000, 1000, kDebounce));
}

void test_the_debounce_holds_across_the_counter_wrapping() {
    const uint32_t last = 0xFFFFFFF0u; // 16 ms before the wrap
    TEST_ASSERT_TRUE(bounce(last + 20, last, kDebounce)); // 4 ms after it
    TEST_ASSERT_FALSE(bounce(last + 35, last, kDebounce));
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_a_quick_press_is_a_tap);
    RUN_TEST(test_a_slow_press_is_still_a_tap);
    RUN_TEST(test_2_s_or_more_is_a_hold);
    RUN_TEST(test_an_edge_inside_the_debounce_is_bounce);
    RUN_TEST(test_an_edge_at_the_debounce_counts);
    RUN_TEST(test_the_debounce_holds_across_the_counter_wrapping);
    return UNITY_END();
}
