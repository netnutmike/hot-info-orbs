#ifndef STATIONPAGE_H
#define STATIONPAGE_H

#include "AmbientParse.h" // StationReading
#include "StationStatus.h"
#include "WeatherSource.h"
#include <memory>

// A personal weather station that gives "now" only (issue #51: every service
// but Tempest, whose reply also carries a forecast and stays a whole
// WeatherSource). docs/superpowers/specs/2026-11-18-weather-station-types-scope.md
class StationSource {
  public:
    virtual ~StationSource() = default;
    // Fills r; on failure sets st.error (and st.code when there was a reply)
    virtual bool read(StationReading &r, StationStatus &st) = 0;
};

// One station page: the station for the temperature and the readings on the
// conditions orb, and Visual Crossing for the rest (words, picture, today's
// high and low, sun times, three days), at the station's own coordinates
// when the service sends them, otherwise at the settings page's location.
// The station decides success: with no forecast the page still shows the
// temperature, and the settings page says why the rest is missing.
class StationPage : public WeatherSource {
  public:
    StationPage(int slot, StationSource *station, const std::string &label); // takes ownership
    bool fetch(WeatherDataModel &model) override;
    String label() override { return m_label; }
    String name() override { return "Weather " + m_label; }

  private:
    int m_slot;
    std::unique_ptr<StationSource> m_station;
    String m_label;
};

#endif
