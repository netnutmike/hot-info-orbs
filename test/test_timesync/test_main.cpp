// Host tests for TimeSyncWatch.h:  pio test -e native -f test_timesync
#include "TimeSyncWatch.h"
#include <string>
#include <unity.h>

void setUp() {}
void tearDown() {}

using devlog::kFirstSyncWarnS;
using devlog::kSyncOverdueS;
using devlog::TimeSyncWatch;

void test_a_first_sync_on_time_is_not_news() {
    TimeSyncWatch w;
    TEST_ASSERT_EQUAL_STRING("", w.step(5, 0, 0).c_str());
    TEST_ASSERT_EQUAL_STRING("", w.step(6, 1, 6).c_str()); // "clock set" already says it
}

void test_no_first_sync_is_news_once() {
    TimeSyncWatch w;
    TEST_ASSERT_EQUAL_STRING("", w.step(kFirstSyncWarnS - 1, 0, 0).c_str());
    TEST_ASSERT_EQUAL_STRING("no time sync yet, 120 s after start",
                             w.step(kFirstSyncWarnS, 0, 0).c_str());
    TEST_ASSERT_EQUAL_STRING("", w.step(kFirstSyncWarnS + 60, 0, 0).c_str());
}

void test_a_late_first_sync_says_how_late() {
    TimeSyncWatch w;
    w.step(kFirstSyncWarnS, 0, 0);
    TEST_ASSERT_EQUAL_STRING("time sync ok, first one 300 s after start", w.step(300, 1, 300).c_str());
}

void test_regular_syncs_are_not_news() {
    TimeSyncWatch w;
    w.step(10, 1, 10);
    TEST_ASSERT_EQUAL_STRING("", w.step(910, 2, 910).c_str());
    TEST_ASSERT_EQUAL_STRING("", w.step(910 + kSyncOverdueS - 1, 2, 910).c_str());
}

void test_an_overdue_sync_is_news_once() {
    TimeSyncWatch w;
    w.step(10, 1, 10);
    TEST_ASSERT_EQUAL_STRING("no time sync for 35 min", w.step(10 + kSyncOverdueS, 1, 10).c_str());
    TEST_ASSERT_EQUAL_STRING("", w.step(10 + kSyncOverdueS + 600, 1, 10).c_str());
}

void test_the_sync_after_an_overdue_one_says_how_long_it_was_out() {
    TimeSyncWatch w;
    w.step(10, 1, 10);
    w.step(10 + kSyncOverdueS, 1, 10);
    TEST_ASSERT_EQUAL_STRING("time sync ok again after 50 min without one", w.step(3010, 2, 3010).c_str());
    TEST_ASSERT_EQUAL_STRING("", w.step(3011, 2, 3010).c_str());
}

void test_overdue_again_after_recovering_is_news_again() {
    TimeSyncWatch w;
    w.step(10, 1, 10);
    w.step(10 + kSyncOverdueS, 1, 10);
    w.step(3010, 2, 3010);
    TEST_ASSERT_EQUAL_STRING("no time sync for 35 min", w.step(3010 + kSyncOverdueS, 2, 3010).c_str());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_a_first_sync_on_time_is_not_news);
    RUN_TEST(test_no_first_sync_is_news_once);
    RUN_TEST(test_a_late_first_sync_says_how_late);
    RUN_TEST(test_regular_syncs_are_not_news);
    RUN_TEST(test_an_overdue_sync_is_news_once);
    RUN_TEST(test_the_sync_after_an_overdue_one_says_how_long_it_was_out);
    RUN_TEST(test_overdue_again_after_recovering_is_news_again);
    return UNITY_END();
}
