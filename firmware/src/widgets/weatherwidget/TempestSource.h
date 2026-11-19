#ifndef TEMPESTSOURCE_H
#define TEMPESTSOURCE_H

#include "config_helper.h"
// Compiled into every build since 2026-11-09: the token is a setting
// (Settings::tsttok, pasted on the page), so a downloaded .bin can use it.

#include "StationStatus.h"
#include "WeatherSource.h"
#include <string>

// WeatherFlow Tempest better_forecast for one station. The reply is ~96 KB, so
// it is streamed through a filter rather than read into a String (written for
// a module without PSRAM; still right on the XIAO - a 96 KB String fragments
// the heap for nothing). docs/TEMPEST-API.md
class TempestSource : public WeatherSource {
  public:
    TempestSource(int slot, uint32_t stationId, const std::string &label);
    bool fetch(WeatherDataModel &model) override;
    String label() override { return m_label; }
    String name() override { return "Weather " + m_label; }

  private:
    int m_slot;
    uint32_t m_station;
    String m_label;
};

#endif
