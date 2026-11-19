# Ambient Weather Network API (issue #51)

**From the published docs, 2026-11-18; not yet measured against a real
station.** Replace this line with the measurements (reply size, timing, TLS
from the XIAO) once a key pair has been tried on the rig.

Source: the API blueprint in `github.com/ambient-weather/api-docs`
(`apiary.apib`). The docs site `ambientweather.docs.apiary.io` failed to load
on 2026-11-18; the raw blueprint on GitHub is the same text.

Keys, MACs and coordinates are **never committed**: this repo is public, and a
station's coordinates are a home address. They go in each orb's own settings.
`test/fixtures/ambient_sample.json` is made up.

## The call

`GET https://api.ambientweather.net/v1/devices?applicationKey=…&apiKey=…`

- **ambientweather.net**, not .com (the .com site sells the hardware).
- **Two keys**, both made by the station owner at ambientweather.net >
  Account > API Keys: the **API Key** opens that account's stations; the
  **Application Key** names the program asking. Each orb owner makes their
  own pair.
- **The reply** is a JSON array, one object per station on the account:
  `macAddress`, `info` (`name`, `location`, and sometimes
  `coords.coords.lat`/`lon`), and `lastData`, the newest reading. The docs'
  example is about 1 KB per station, so it is read in one go through an
  ArduinoJson filter, not streamed like Tempest's ~96 KB.
- **Rate limit:** 1 request/s per API key, 3/s per Application Key; HTTP 429
  when over. A page fetches every 10 minutes while it is on screen.

## Fields the firmware reads (`AmbientParse.h`)

| Field | Unit | Use |
|---|---|---|
| `lastData.tempf` | °F | the temperature on orb 4 (required: without it the fetch fails) |
| `lastData.feelsLike` | °F | "Feels like" on orb 2 |
| `lastData.humidity` | % | "Humidity" on orb 2 |
| `lastData.windspeedmph`, `windgustmph` | mph | "Wind ENE 1-4 mph" on orb 2 |
| `lastData.winddir` | degrees | compass point for the wind |
| `info.coords.coords.lat`/`lon` | degrees | where Visual Crossing forecasts; when absent, the settings page's location |

Metric pages convert inside the parser (°C, km/h), so the rest of the
firmware sees one unit per setting, as with Tempest.

## Errors the settings page shows

| Cause | Shown |
|---|---|
| wrong or missing key | `HTTP 401: check both Ambient keys` |
| over the rate limit | `HTTP 429: too many requests to Ambient, it tries again later` |
| MAC not on the account | `station … is not on this Ambient account` |
| account has no stations | `no stations on this Ambient account` |
| no outdoor temperature in `lastData` | `no outdoor temperature from this station` |

## Not used yet

- `/v1/devices/{mac}` (history, up to 288 readings) and the realtime
  Socket.io API (`rt2.ambientweather.net`): one reading every 10 minutes is
  all a page needs.
- Rain, pressure and indoor readings: in `lastData`, not on any orb yet.
