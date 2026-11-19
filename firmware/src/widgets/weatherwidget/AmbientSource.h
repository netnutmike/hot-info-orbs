#ifndef AMBIENTSOURCE_H
#define AMBIENTSOURCE_H

#include "StationPage.h"
#include <string>

// Ambient Weather Network (ambientweather.NET, not .com) for one station on
// the account, picked by MAC. Keys from Settings (awapp, awkey).
// docs/AMBIENT-API.md
class AmbientSource : public StationSource {
  public:
    explicit AmbientSource(const std::string &mac) : m_mac(mac) {}
    bool read(StationReading &r, StationStatus &st) override;

  private:
    std::string m_mac;
};

#endif
