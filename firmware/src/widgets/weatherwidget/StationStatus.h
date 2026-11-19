#ifndef STATIONSTATUS_H
#define STATIONSTATUS_H

#include <Arduino.h>

// A station slot's last fetch, for the settings page. An orb in service has
// no serial cable, so this is how a failing station is seen without walking
// over to the orbs. Shared by every station type (issue #51).
struct StationStatus {
    bool attempted = false;
    bool ok = false;
    int code = 0; // HTTP code, or HTTPClient's negative error
    uint32_t ms = 0; // the whole fetch, connect to parse
    uint32_t at = 0; // millis() when it finished. Not wall-clock time: the
                     // start-up fetch runs before the clock has synced
    uint32_t freeHeap = 0; // just after the fetch
    String error; // when !ok
    String note; // when ok but part of the page is missing (the forecast)
};

constexpr int kStationSlots = 2;
StationStatus &stationStatus(int slot);

#endif
