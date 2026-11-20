#ifndef VISUALCROSSINGSOURCE_H
#define VISUALCROSSINGSOURCE_H

#include "WeatherSource.h"

// Visual Crossing timeline API, units from Settings. The location is the one
// on the settings page, or (for a station page, issue #51) the station's own
// "lat,lon". The body moved from WeatherWidget::getWeatherData() unchanged.
class VisualCrossingSource : public WeatherSource {
  public:
    VisualCrossingSource(); // the settings page's location
    explicit VisualCrossingSource(const String &location);
    bool fetch(WeatherDataModel &model) override;
    // After a fetch: the short conditions ("Partly cloudy") for a station
    // page's text, and why a fetch failed
    const String &conditions() const { return m_conditions; }
    const String &lastError() const { return m_error; }

  private:
    String m_url;
    String m_conditions;
    String m_error;
};

#endif
