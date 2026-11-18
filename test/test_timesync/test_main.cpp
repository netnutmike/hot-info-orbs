// Host tests for TimeSyncWatch.h:  pio test -e native -f test_timesync
#include "NoData.h"
#include "TimeSyncWatch.h"
#include <string>
#include <unity.h>

void setUp() {}
void tearDown() {}

using devlog::kFirstSyncWarnS;
using devlog::kSyncOverdueS;
using devlog::TimeSyncWatch;
using devlog::kKickMs;
using devlog::SyncKick;
using devlog::timeValid;
using devlog::syncCurrent;

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

// --- No fake time, and a quick first sync (Scott on the rig, 2026-11-18) ---

void test_a_clock_never_synced_is_not_valid() {
    TEST_ASSERT_FALSE(timeValid(0));
    TEST_ASSERT_FALSE(timeValid(7 * 3600 + 26 * 60)); // 1970, what the rig showed as 2:26
    TEST_ASSERT_TRUE(timeValid(1795000000)); // November 2026
}

void test_no_kick_while_the_sync_is_current() {
    SyncKick k;
    k.step(0, true, true);
    TEST_ASSERT_FALSE(k.step(kKickMs * 10, true, true));
}

void test_no_current_sync_kicks_every_interval_not_every_pass() {
    SyncKick k;
    TEST_ASSERT_FALSE(k.step(1000, false, true)); // start: SNTP was just started
    TEST_ASSERT_FALSE(k.step(1000 + kKickMs - 1, false, true));
    TEST_ASSERT_TRUE(k.step(1000 + kKickMs, false, true));
    TEST_ASSERT_FALSE(k.step(1000 + kKickMs + 10, false, true));
    TEST_ASSERT_TRUE(k.step(1000 + 2 * kKickMs, false, true));
}

void test_no_kick_while_wifi_is_down() {
    SyncKick k;
    k.step(0, false, true);
    TEST_ASSERT_FALSE(k.step(kKickMs * 3, false, false));
}

void test_wifi_back_without_a_current_sync_kicks_at_once() {
    SyncKick k;
    k.step(0, false, true);
    k.step(1000, false, false);
    TEST_ASSERT_TRUE(k.step(2000, false, true));
    TEST_ASSERT_FALSE(k.step(2000 + kKickMs - 1, false, true)); // the interval starts again there
}

void test_wifi_back_with_a_current_sync_does_not_kick() {
    SyncKick k;
    k.step(0, true, true);
    k.step(1000, true, false);
    TEST_ASSERT_FALSE(k.step(2000, true, true));
}

void test_an_empty_page_blames_the_internet_only_without_a_current_sync() {
    TEST_ASSERT_EQUAL_STRING("no internet", nodata::reason(false));
    TEST_ASSERT_EQUAL_STRING("will retry", nodata::reason(true));
}

// A restart keeps the clock (rig, 2026-11-18), so a valid time does not mean
// the internet is up. These key on a recent sync instead.
void test_no_sync_since_start_is_not_current_even_with_a_valid_clock() {
    TEST_ASSERT_FALSE(syncCurrent(60, 0, 0));
}

void test_a_sync_is_current_until_it_is_overdue() {
    TEST_ASSERT_TRUE(syncCurrent(100, 1, 50));
    TEST_ASSERT_TRUE(syncCurrent(50 + kSyncOverdueS - 1, 1, 50));
    TEST_ASSERT_FALSE(syncCurrent(50 + kSyncOverdueS, 1, 50));
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
    RUN_TEST(test_a_clock_never_synced_is_not_valid);
    RUN_TEST(test_no_kick_while_the_sync_is_current);
    RUN_TEST(test_no_current_sync_kicks_every_interval_not_every_pass);
    RUN_TEST(test_no_kick_while_wifi_is_down);
    RUN_TEST(test_wifi_back_without_a_current_sync_kicks_at_once);
    RUN_TEST(test_wifi_back_with_a_current_sync_does_not_kick);
    RUN_TEST(test_an_empty_page_blames_the_internet_only_without_a_current_sync);
    RUN_TEST(test_no_sync_since_start_is_not_current_even_with_a_valid_clock);
    RUN_TEST(test_a_sync_is_current_until_it_is_overdue);
    return UNITY_END();
}
