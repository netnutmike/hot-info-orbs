#pragma once
// The second line on a page that has had no data since power-on (weather,
// stocks). Its first line says what is missing. std only, for the host tests.
//
// No time sync either means the internet itself is out (the time server and
// the data service are both on it). With the time in, the service is the one
// not answering, and the page tries again on its own turn. "No WiFi" needs no
// line here: the page's plate already says it (StalePolicy.h).

namespace nodata {

inline const char *reason(bool timeValid) {
    return timeValid ? "will retry" : "no internet";
}

} // namespace nodata
