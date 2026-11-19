// Host tests for AmbientParse.h and the station-type table (issue #51):
//   pio test -e native -f test_ambient
// The fixture is made up (no real station, made-up MACs and coordinates): the
// repo is public. Reply shape: docs/AMBIENT-API.md.
#include "AmbientParse.h"
#include "SettingsValidation.h"
#include "StationTypes.h"
#include <fstream>
#include <sstream>
#include <unity.h>

void setUp() {}
void tearDown() {}

static bool parseFixture(const std::string &mac, bool metric, StationReading &r, std::string &err) {
    std::ifstream in("test/fixtures/ambient_sample.json");
    TEST_ASSERT_TRUE(in.good());
    std::stringstream ss;
    ss << in.rdbuf();
    JsonDocument filter;
    ambientFilter(filter);
    JsonDocument doc;
    TEST_ASSERT_FALSE(deserializeJson(doc, ss.str(), DeserializationOption::Filter(filter)));
    return ambientParse(doc, mac, metric, r, err);
}

// --- picking the station ---

void test_the_station_with_that_mac() {
    StationReading r;
    std::string err;
    TEST_ASSERT_TRUE(parseFixture("AA:BB:CC:DD:EE:01", false, r, err));
    TEST_ASSERT_EQUAL_STRING("Dock", r.name.c_str());
    TEST_ASSERT_EQUAL_FLOAT(32, r.temp);
}

void test_mac_match_ignores_case() {
    StationReading r;
    std::string err;
    TEST_ASSERT_TRUE(parseFixture("aa:bb:cc:dd:ee:01", false, r, err));
    TEST_ASSERT_EQUAL_STRING("Dock", r.name.c_str());
}

void test_a_mac_not_on_the_account_is_an_error() {
    StationReading r;
    std::string err;
    TEST_ASSERT_FALSE(parseFixture("00:00:00:00:00:99", false, r, err));
    TEST_ASSERT_EQUAL_STRING("station 00:00:00:00:00:99 is not on this Ambient account", err.c_str());
}

void test_an_empty_account_is_an_error() {
    JsonDocument doc;
    deserializeJson(doc, "[]");
    StationReading r;
    std::string err;
    TEST_ASSERT_FALSE(ambientParse(doc, "00:11:22:33:44:55", false, r, err));
    TEST_ASSERT_EQUAL_STRING("no stations on this Ambient account", err.c_str());
}

void test_a_reply_that_is_not_a_list_is_an_error() {
    JsonDocument doc;
    deserializeJson(doc, "{\"error\":\"apiKey-missing\"}");
    StationReading r;
    std::string err;
    TEST_ASSERT_FALSE(ambientParse(doc, "00:11:22:33:44:55", false, r, err));
    TEST_ASSERT_EQUAL_STRING("unexpected reply from Ambient", err.c_str());
}

void test_a_station_without_outdoor_temperature_is_an_error() {
    JsonDocument doc;
    deserializeJson(doc, "[{\"macAddress\":\"00:11:22:33:44:55\",\"lastData\":{\"tempinf\":70}}]");
    StationReading r;
    std::string err;
    TEST_ASSERT_FALSE(ambientParse(doc, "00:11:22:33:44:55", false, r, err));
    TEST_ASSERT_EQUAL_STRING("no outdoor temperature from this station", err.c_str());
}

// --- the readings ---

void test_us_units_as_sent() {
    StationReading r;
    std::string err;
    TEST_ASSERT_TRUE(parseFixture("00:11:22:33:44:55", false, r, err));
    TEST_ASSERT_EQUAL_FLOAT(66.9, r.temp);
    TEST_ASSERT_EQUAL_FLOAT(66.9, r.feels);
    TEST_ASSERT_EQUAL_FLOAT(30, r.humidity);
    TEST_ASSERT_EQUAL_FLOAT(0.9, r.windAvg);
    TEST_ASSERT_EQUAL_FLOAT(4, r.windGust);
    TEST_ASSERT_EQUAL_STRING("ENE", r.windDir.c_str());
}

void test_metric_converts() {
    StationReading r;
    std::string err;
    TEST_ASSERT_TRUE(parseFixture("AA:BB:CC:DD:EE:01", true, r, err));
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, r.temp);
    TEST_ASSERT_FLOAT_WITHIN(0.01, -2.0, r.feels);
    TEST_ASSERT_EQUAL_FLOAT(0, r.windAvg);
}

void test_metric_wind_in_kmh() {
    StationReading r;
    std::string err;
    TEST_ASSERT_TRUE(parseFixture("00:11:22:33:44:55", true, r, err));
    TEST_ASSERT_FLOAT_WITHIN(0.01, 6.437, r.windGust);
}

void test_missing_readings_stay_missing() {
    JsonDocument doc;
    deserializeJson(doc, "[{\"macAddress\":\"00:11:22:33:44:55\",\"lastData\":{\"tempf\":50}}]");
    StationReading r;
    std::string err;
    TEST_ASSERT_TRUE(ambientParse(doc, "00:11:22:33:44:55", false, r, err));
    TEST_ASSERT_TRUE(std::isnan(r.feels));
    TEST_ASSERT_TRUE(std::isnan(r.humidity));
    TEST_ASSERT_TRUE(std::isnan(r.windAvg));
    TEST_ASSERT_EQUAL_STRING("", r.windDir.c_str());
}

void test_coordinates_when_the_station_has_them() {
    StationReading r;
    std::string err;
    TEST_ASSERT_TRUE(parseFixture("AA:BB:CC:DD:EE:01", false, r, err));
    TEST_ASSERT_TRUE(r.hasCoords);
    TEST_ASSERT_EQUAL_FLOAT(40.5, r.lat);
    TEST_ASSERT_EQUAL_FLOAT(-105.25, r.lon);
}

void test_no_coordinates_when_the_station_has_none() {
    StationReading r;
    std::string err;
    TEST_ASSERT_TRUE(parseFixture("00:11:22:33:44:55", false, r, err));
    TEST_ASSERT_FALSE(r.hasCoords);
}

void test_summary_reads_like_a_tempest_page() {
    StationReading r;
    std::string err;
    TEST_ASSERT_TRUE(parseFixture("00:11:22:33:44:55", false, r, err));
    r.conditions = "Partly cloudy";
    TEST_ASSERT_EQUAL_STRING("Partly cloudy\nFeels like 67\nHumidity 30%\nWind ENE 1-4 mph", stationSummary(r, "mph").c_str());
}

void test_compass_points() {
    TEST_ASSERT_EQUAL_STRING("N", compass16(0).c_str());
    TEST_ASSERT_EQUAL_STRING("N", compass16(359).c_str());
    TEST_ASSERT_EQUAL_STRING("NNE", compass16(22.5).c_str());
    TEST_ASSERT_EQUAL_STRING("E", compass16(90).c_str());
    TEST_ASSERT_EQUAL_STRING("W", compass16(270).c_str());
    TEST_ASSERT_EQUAL_STRING("", compass16(NAN).c_str());
}

// --- the station MAC on the settings page ---

void test_mac_blank_is_unused() {
    std::string out = "x", err;
    TEST_ASSERT_TRUE(sv::parseMac("  ", out, err));
    TEST_ASSERT_EQUAL_STRING("", out.c_str());
}

void test_mac_normalized() {
    std::string out, err;
    TEST_ASSERT_TRUE(sv::parseMac(" aa-bb-cc-dd-ee-01 ", out, err));
    TEST_ASSERT_EQUAL_STRING("AA:BB:CC:DD:EE:01", out.c_str());
    TEST_ASSERT_TRUE(sv::parseMac("aabbccddee01", out, err));
    TEST_ASSERT_EQUAL_STRING("AA:BB:CC:DD:EE:01", out.c_str());
}

void test_mac_rejects_wrong_length_or_letters() {
    std::string out, err;
    TEST_ASSERT_FALSE(sv::parseMac("AA:BB:CC:DD:EE", out, err));
    TEST_ASSERT_FALSE(sv::parseMac("AA:BB:CC:DD:EE:GG", out, err));
    TEST_ASSERT_EQUAL_STRING("A MAC is 12 hex digits, like 00:0E:C6:AB:12:34 (shown on ambientweather.net under Devices)", err.c_str());
}

void test_ambient_key_hint() {
    TEST_ASSERT_NOT_NULL(strstr(sv::ambientKeyHint("", "", false).c_str(), "Optional"));
    TEST_ASSERT_NOT_NULL(strstr(sv::ambientKeyHint("", "", true).c_str(), "No key yet"));
    TEST_ASSERT_NOT_NULL(strstr(sv::ambientKeyHint("k", "k", true).c_str(), "A key is stored"));
    TEST_ASSERT_NULL(strstr(sv::ambientKeyHint("secretA", "secretB", true).c_str(), "secret"));
}

// --- the station-type table ---

void test_type_table_keeps_tempest_at_zero() {
    // Saved settings from before #51 have no type: 0 must stay Tempest
    TEST_ASSERT_EQUAL_STRING("tempest", stations::kTypes[0].key);
    TEST_ASSERT_TRUE(stations::Type::Tempest == stations::typeFrom(0));
}

void test_type_from_an_unknown_index_is_tempest() {
    TEST_ASSERT_TRUE(stations::Type::Tempest == stations::typeFrom(-1));
    TEST_ASSERT_TRUE(stations::Type::Tempest == stations::typeFrom(99));
    TEST_ASSERT_TRUE(stations::Type::Ambient == stations::typeFrom(1));
}

void test_every_type_names_its_id_field() {
    for (const auto &t : stations::kTypes) {
        TEST_ASSERT_NOT_NULL(t.key);
        TEST_ASSERT_TRUE(strlen(t.name) > 0);
        TEST_ASSERT_TRUE(strlen(t.idLabel) > 0);
    }
}

// A slot is used by the ID of the type it is set to; the other type's ID,
// left over from before a switch, does not count
void test_slot_used_follows_its_type(void) {
    TEST_ASSERT_TRUE(stations::slotUsed(stations::Type::Tempest, 12345, ""));
    TEST_ASSERT_FALSE(stations::slotUsed(stations::Type::Tempest, 0, "AA:BB:CC:DD:EE:01"));
    TEST_ASSERT_TRUE(stations::slotUsed(stations::Type::Ambient, 0, "AA:BB:CC:DD:EE:01"));
    TEST_ASSERT_FALSE(stations::slotUsed(stations::Type::Ambient, 12345, ""));
}

// A refused request carries the reason in its body; that, not just the code,
// goes on the settings page (measured reply, 2026-11-19)
void test_error_body_text(void) {
    TEST_ASSERT_EQUAL_STRING("You are using an apiKey as your applicationKey. Please contact support@ambientweather.com to get an applicationKey",
                             ambientErrorText("{\"error\":\"You are using an apiKey as your applicationKey. Please contact support@ambientweather.com to get an applicationKey\"}").c_str());
    TEST_ASSERT_EQUAL_STRING("", ambientErrorText("<html>Bad gateway</html>").c_str());
    TEST_ASSERT_EQUAL_STRING("", ambientErrorText("{\"other\":1}").c_str());
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_the_station_with_that_mac);
    RUN_TEST(test_mac_match_ignores_case);
    RUN_TEST(test_a_mac_not_on_the_account_is_an_error);
    RUN_TEST(test_an_empty_account_is_an_error);
    RUN_TEST(test_a_reply_that_is_not_a_list_is_an_error);
    RUN_TEST(test_a_station_without_outdoor_temperature_is_an_error);
    RUN_TEST(test_us_units_as_sent);
    RUN_TEST(test_metric_converts);
    RUN_TEST(test_metric_wind_in_kmh);
    RUN_TEST(test_missing_readings_stay_missing);
    RUN_TEST(test_coordinates_when_the_station_has_them);
    RUN_TEST(test_no_coordinates_when_the_station_has_none);
    RUN_TEST(test_summary_reads_like_a_tempest_page);
    RUN_TEST(test_compass_points);
    RUN_TEST(test_mac_blank_is_unused);
    RUN_TEST(test_mac_normalized);
    RUN_TEST(test_mac_rejects_wrong_length_or_letters);
    RUN_TEST(test_ambient_key_hint);
    RUN_TEST(test_type_table_keeps_tempest_at_zero);
    RUN_TEST(test_type_from_an_unknown_index_is_tempest);
    RUN_TEST(test_every_type_names_its_id_field);
    RUN_TEST(test_slot_used_follows_its_type);
    RUN_TEST(test_error_body_text);
    return UNITY_END();
}
