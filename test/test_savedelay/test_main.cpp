// Host tests for SaveDelay.h (issue #60):  pio test -e native -f test_savedelay
#include "SaveDelay.h"
#include <unity.h>

void setUp() {}
void tearDown() {}

using savedelay::kDelayMs;
using savedelay::SaveDelay;

void test_nothing_changed_nothing_due() {
    SaveDelay d;
    TEST_ASSERT_FALSE(d.owed());
    TEST_ASSERT_FALSE(d.due(0));
    TEST_ASSERT_FALSE(d.due(100000));
}

void test_due_after_the_delay_once() {
    SaveDelay d;
    d.changed(1000);
    TEST_ASSERT_TRUE(d.owed());
    TEST_ASSERT_FALSE(d.due(1000 + kDelayMs - 1));
    TEST_ASSERT_TRUE(d.due(1000 + kDelayMs));
    TEST_ASSERT_FALSE(d.owed());
    TEST_ASSERT_FALSE(d.due(1000 + kDelayMs + 1));
}

// Tapping through the faces: each tap moves the write on, so it lands once,
// kDelayMs after the last tap
void test_a_run_of_taps_writes_once_after_the_last() {
    SaveDelay d;
    int writes = 0;
    for (uint32_t t = 0; t <= 4000; t += 1000) {
        d.changed(t);
        writes += d.due(t + 500) ? 1 : 0;
    }
    TEST_ASSERT_EQUAL(0, writes);
    TEST_ASSERT_FALSE(d.due(4000 + kDelayMs - 1));
    TEST_ASSERT_TRUE(d.due(4000 + kDelayMs));
}

void test_a_change_after_a_write_owes_another() {
    SaveDelay d;
    d.changed(0);
    TEST_ASSERT_TRUE(d.due(kDelayMs));
    d.changed(20000);
    TEST_ASSERT_TRUE(d.due(20000 + kDelayMs));
}

void test_holds_across_the_counter_wrapping() {
    SaveDelay d;
    const uint32_t t = 0xFFFFFF00u;
    d.changed(t);
    TEST_ASSERT_FALSE(d.due(t + 1000)); // past the wrap, not yet due
    TEST_ASSERT_TRUE(d.due(t + kDelayMs));
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_nothing_changed_nothing_due);
    RUN_TEST(test_due_after_the_delay_once);
    RUN_TEST(test_a_run_of_taps_writes_once_after_the_last);
    RUN_TEST(test_a_change_after_a_write_owes_another);
    RUN_TEST(test_holds_across_the_counter_wrapping);
    return UNITY_END();
}
