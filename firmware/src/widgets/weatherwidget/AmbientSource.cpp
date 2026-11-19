#include "AmbientSource.h"

#include "AmbientParse.h"
#include "Settings.h"
#include "SettingsValidation.h"
#include <HTTPClient.h>

// GET /v1/devices: every station on the account with its latest reading, a
// few KB, read through a filter. Limits: 1 request/s per API key, 3/s per
// application key; a page fetches every 10 minutes while it is on screen.
bool AmbientSource::read(StationReading &r, StationStatus &st) {
    const SettingsValues &s = Settings::get();
    const String url = String("https://api.ambientweather.net/v1/devices?applicationKey=") + sv::urlEncode(s.awapp).c_str() +
                       "&apiKey=" + sv::urlEncode(s.awkey).c_str();
    HTTPClient http;
    http.useHTTP10(true); // no chunked encoding, so getStream() is plain JSON
    http.begin(url);
    st.code = http.GET();
    bool ok = false;
    if (st.code == 200) {
        JsonDocument filter;
        ambientFilter(filter);
        JsonDocument doc;
        DeserializationError e = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
        std::string err;
        if (e) {
            st.error = String("JSON: ") + e.c_str();
        } else if (!ambientParse(doc, m_mac, s.wxmetric, r, err)) {
            st.error = err.c_str();
        } else {
            ok = true;
        }
    } else if (st.code == 401) {
        st.error = "HTTP 401: check both Ambient keys";
    } else if (st.code == 429) {
        st.error = "HTTP 429: too many requests to Ambient, it tries again later";
    } else {
        st.error = st.code > 0 ? String("HTTP ") + st.code : http.errorToString(st.code);
    }
    http.end();
    return ok;
}
