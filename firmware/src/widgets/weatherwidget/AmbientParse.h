#pragma once
// Ambient Weather Network (ambientweather.net) parsing, issue #51. std +
// ArduinoJson only, so it is covered by `pio test -e native`
// (test/test_ambient). Reply shape and limits: docs/AMBIENT-API.md.
//
// GET /v1/devices gives every station on the account with its latest
// reading: current values only, no forecast. The page's forecast comes from
// a forecast service (StationPage.h).

#include "TempestParse.h" // stationSummary
#include <ArduinoJson.h>
#include <cctype>
#include <cmath>
#include <string>

// What a station gives for "now". NAN = the station sent no such reading.
struct StationReading {
    std::string name; // the station's own name on the service, "" if none
    std::string mac; // as the service sends it; with a blank MAC setting, the one it used
    std::string conditions; // filled from the forecast service, not the station
    float temp = NAN;
    float feels = NAN;
    float humidity = NAN;
    float windAvg = NAN;
    float windGust = NAN;
    std::string windDir; // "NNW", "" when unknown
    bool hasCoords = false;
    double lat = 0;
    double lon = 0;
};

// Degrees to one of 16 compass points; "" for NAN
inline std::string compass16(float deg) {
    static const char *pts[] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};
    if (std::isnan(deg)) {
        return "";
    }
    int i = (int)lroundf(fmodf(fmodf(deg, 360.0f) + 360.0f, 360.0f) / 22.5f) % 16;
    return pts[i];
}

inline bool ambientSameMac(const char *a, const std::string &b) {
    if (!a) {
        return false;
    }
    size_t i = 0;
    for (; a[i] && i < b.size(); i++) {
        if (toupper((unsigned char)a[i]) != toupper((unsigned char)b[i])) {
            return false;
        }
    }
    return !a[i] && i == b.size();
}

// Keeps what the page uses; [0] in a filter applies to every element
inline void ambientFilter(JsonDocument &filter) {
    JsonObject d = filter[0].to<JsonObject>();
    d["macAddress"] = true;
    d["info"]["name"] = true;
    d["info"]["coords"]["coords"]["lat"] = true;
    d["info"]["coords"]["coords"]["lon"] = true;
    JsonObject l = d["lastData"].to<JsonObject>();
    for (const char *k : {"tempf", "feelsLike", "humidity", "windspeedmph", "windgustmph", "winddir"}) {
        l[k] = true;
    }
}

// The station with this MAC (as sv::parseMac keeps it). Ambient sends US
// units; metric converts here, so the page needs no unit logic per service.
inline bool ambientParse(JsonDocument &doc, const std::string &mac, bool metric, StationReading &out, std::string &err) {
    JsonArray all = doc.as<JsonArray>();
    if (all.isNull()) {
        err = "unexpected reply from Ambient";
        return false;
    }
    if (all.size() == 0) {
        err = "no stations on this Ambient account";
        return false;
    }
    JsonObject dev;
    if (mac.empty()) {
        // Blank: the account's only station. With several, name them so the
        // owner can copy the right MAC from the settings page.
        if (all.size() > 1) {
            err = "this Ambient account has " + std::to_string(all.size()) + " stations:";
            const char *sep = " ";
            for (JsonObject d : all) {
                err += sep + std::string(d["info"]["name"] | "(no name)") + " " + std::string(d["macAddress"] | "?");
                sep = ", ";
            }
            err += ". Enter the MAC of the one for this page";
            return false;
        }
        dev = all[0];
    } else {
        for (JsonObject d : all) {
            if (ambientSameMac(d["macAddress"], mac)) {
                dev = d;
                break;
            }
        }
    }
    if (dev.isNull()) {
        err = "station " + mac + " is not on this Ambient account";
        return false;
    }
    JsonObject last = dev["lastData"];
    if (last.isNull() || !last["tempf"].is<float>()) {
        err = "no outdoor temperature from this station";
        return false;
    }
    auto temp = [metric](float f) { return std::isnan(f) || !metric ? f : (f - 32.0f) * 5.0f / 9.0f; };
    auto speed = [metric](float mph) { return std::isnan(mph) || !metric ? mph : mph * 1.609344f; };
    StationReading r;
    r.name = dev["info"]["name"] | "";
    r.mac = dev["macAddress"] | "";
    r.temp = temp(last["tempf"].as<float>());
    r.feels = temp(last["feelsLike"] | NAN);
    r.humidity = last["humidity"] | NAN;
    r.windAvg = speed(last["windspeedmph"] | NAN);
    r.windGust = speed(last["windgustmph"] | NAN);
    r.windDir = compass16(last["winddir"] | NAN);
    JsonObject c = dev["info"]["coords"]["coords"];
    if (c["lat"].is<double>() && c["lon"].is<double>()) {
        r.hasCoords = true;
        r.lat = c["lat"].as<double>();
        r.lon = c["lon"].as<double>();
    }
    out = r;
    return true;
}

// The reason in a refused request's body, {"error":"..."}; "" when there is
// none (docs/AMBIENT-API.md: two API keys are refused with a clear one)
inline std::string ambientErrorText(const std::string &body) {
    JsonDocument doc;
    if (deserializeJson(doc, body) || !doc["error"].is<const char *>()) {
        return "";
    }
    return doc["error"].as<const char *>();
}
