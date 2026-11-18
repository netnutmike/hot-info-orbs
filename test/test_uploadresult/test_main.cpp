// Host tests for UploadResult.h (issue #55):  pio test -e native -f test_uploadresult
#include "UploadResult.h"
#include <string>
#include <unity.h>

void setUp() {}
void tearDown() {}

using ota::Outcome;

static bool has(const std::string &page, const char *text) {
    return page.find(text) != std::string::npos;
}

// --- what happened ---

void test_no_bytes_is_no_file() {
    TEST_ASSERT_TRUE(ota::outcome(0, false) == Outcome::NoFile);
}

void test_no_bytes_is_no_file_even_with_an_error_set() {
    // The pre-#55 path: Update.end() on nothing sets an error. Still no file.
    TEST_ASSERT_TRUE(ota::outcome(0, true) == Outcome::NoFile);
}

void test_bytes_and_no_error_is_ok() {
    TEST_ASSERT_TRUE(ota::outcome(1942896, false) == Outcome::Ok);
}

void test_bytes_and_an_error_is_failed() {
    TEST_ASSERT_TRUE(ota::outcome(4096, true) == Outcome::Failed);
}

// --- restart ---

void test_no_file_does_not_restart() {
    TEST_ASSERT_FALSE(ota::restartAfter(Outcome::NoFile));
}

void test_ok_restarts() {
    TEST_ASSERT_TRUE(ota::restartAfter(Outcome::Ok));
}

void test_failed_restarts() {
    // Kept for now: the restart is what redraws the screens after a failed write.
    TEST_ASSERT_TRUE(ota::restartAfter(Outcome::Failed));
}

// --- the page ---

void test_every_page_is_html_with_a_settings_link() {
    for (Outcome o : {Outcome::NoFile, Outcome::Ok, Outcome::Failed}) {
        std::string p = ota::resultPage(o, "x");
        TEST_ASSERT_TRUE(has(p, "<!DOCTYPE html>"));
        TEST_ASSERT_TRUE(has(p, "href='/settings'"));
    }
}

void test_no_file_page_says_so_and_links_back_to_update() {
    std::string p = ota::resultPage(Outcome::NoFile, "");
    TEST_ASSERT_TRUE(has(p, "No file chosen"));
    TEST_ASSERT_TRUE(has(p, "href='/update'"));
    TEST_ASSERT_FALSE(has(p, "restarting"));
    TEST_ASSERT_FALSE(has(p, "<script"));
}

void test_ok_page_says_restarting_and_returns_to_settings_by_itself() {
    std::string p = ota::resultPage(Outcome::Ok, "");
    TEST_ASSERT_TRUE(has(p, "Update OK"));
    TEST_ASSERT_TRUE(has(p, "restarting"));
    TEST_ASSERT_TRUE(has(p, "<script"));
    TEST_ASSERT_TRUE(has(p, "'/settings'"));
}

void test_failed_page_gives_the_reason_and_a_try_again_link() {
    std::string p = ota::resultPage(Outcome::Failed, "Magic byte is wrong, not 0xE9");
    TEST_ASSERT_TRUE(has(p, "Update failed"));
    TEST_ASSERT_TRUE(has(p, "old firmware is kept"));
    TEST_ASSERT_TRUE(has(p, "Magic byte is wrong, not 0xE9"));
    TEST_ASSERT_TRUE(has(p, "href='/update'"));
    TEST_ASSERT_TRUE(has(p, "<script"));
}

void test_failed_reason_is_escaped() {
    std::string p = ota::resultPage(Outcome::Failed, "<b>&'\"");
    TEST_ASSERT_TRUE(has(p, "&lt;b&gt;&amp;&#39;&quot;"));
    TEST_ASSERT_FALSE(has(p, "<b>&'\""));
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_no_bytes_is_no_file);
    RUN_TEST(test_no_bytes_is_no_file_even_with_an_error_set);
    RUN_TEST(test_bytes_and_no_error_is_ok);
    RUN_TEST(test_bytes_and_an_error_is_failed);
    RUN_TEST(test_no_file_does_not_restart);
    RUN_TEST(test_ok_restarts);
    RUN_TEST(test_failed_restarts);
    RUN_TEST(test_every_page_is_html_with_a_settings_link);
    RUN_TEST(test_no_file_page_says_so_and_links_back_to_update);
    RUN_TEST(test_ok_page_says_restarting_and_returns_to_settings_by_itself);
    RUN_TEST(test_failed_page_gives_the_reason_and_a_try_again_link);
    RUN_TEST(test_failed_reason_is_escaped);
    return UNITY_END();
}
