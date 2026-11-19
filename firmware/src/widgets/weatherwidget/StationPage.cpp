#include "StationPage.h"

#include "Settings.h"
#include "VisualCrossingSource.h"

StationPage::StationPage(int slot, StationSource *station, const std::string &label)
    : m_slot(slot), m_station(station), m_label(label.c_str()) {}

bool StationPage::fetch(WeatherDataModel &model) {
    StationStatus st;
    st.attempted = true;
    const uint32_t start = millis();
    const SettingsValues &s = Settings::get();
    StationReading r;
    st.ok = m_station->read(r, st);
    if (st.ok) {
        if (s.wxkey.empty()) {
            st.note = "No forecast: paste a Visual Crossing key under API keys";
        } else {
            const String where = r.hasCoords ? String(r.lat, 4) + "," + String(r.lon, 4) : String(s.wxloc.c_str());
            VisualCrossingSource forecast(where);
            if (forecast.fetch(model)) {
                r.conditions = forecast.conditions().c_str();
            } else {
                st.note = "Forecast failed (" + forecast.lastError() + "); the rest of the page is from the last good fetch";
            }
        }
        // After the forecast, which also sets these from its own idea of "now"
        model.setCityName(m_label);
        model.setCurrentTemperature(r.temp);
        model.setCurrentText(stationSummary(r, s.wxmetric ? "kph" : "mph").c_str());
    }
    st.ms = millis() - start;
    st.at = millis();
    st.freeHeap = ESP.getFreeHeap();
    stationStatus(m_slot) = st;
    if (!st.ok) {
        Serial.printf("Station %s: %s\n", m_label.c_str(), st.error.c_str());
    }
    return st.ok;
}
