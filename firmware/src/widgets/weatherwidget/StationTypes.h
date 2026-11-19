#ifndef STATIONTYPES_H
#define STATIONTYPES_H

// The kinds of personal weather station a station slot can be (issue #51,
// docs/superpowers/specs/2026-11-18-weather-station-types-scope.md). The
// settings page builds the type picker and the station's ID field from this
// table; main.cpp builds the page from the type. No Arduino types, so the
// host tests check it (test/test_ambient).
//
// To add a type: a row here (append only: the index is what is saved), its
// source file with a host-tested parser, its account keys in Settings, and
// its case in main.cpp and the settings page's save.

#include <cstddef>

#include <cstdint>
#include <string>

namespace stations {

enum class Type { Tempest = 0, Ambient = 1 };

struct TypeInfo {
    const char *key; // stable name, for logs and the page's form values
    const char *name; // shown in the picker
    const char *idLabel; // what the slot's ID field holds
    const char *idHint; // where the owner finds it
    const char *keysHint; // the account keys it needs, under API keys
};

constexpr TypeInfo kTypes[] = {
    {"tempest", "Tempest (WeatherFlow)", "Station ID", "The number in the station's tempestwx.com address.",
     "a Tempest token"},
    {"ambient", "Ambient Weather", "Station MAC", "On ambientweather.net under Devices, like 00:0E:C6:AB:12:34.",
     "an Ambient Application Key and API Key"},
};
constexpr int kTypeCount = sizeof(kTypes) / sizeof(kTypes[0]);

// A saved index, or a form value; anything unknown is Tempest, which is what
// every slot was before types existed
inline Type typeFrom(int index) {
    return index >= 0 && index < kTypeCount ? static_cast<Type>(index) : Type::Tempest;
}

inline const TypeInfo &info(Type t) {
    return kTypes[static_cast<int>(t)];
}

// A slot has a page when the ID for its own type is set. The other type's ID
// is kept (switching back restores it) but does not count.
inline bool slotUsed(Type t, uint32_t tempestId, const std::string &mac) {
    return t == Type::Ambient ? !mac.empty() : tempestId != 0;
}

} // namespace stations

#endif
