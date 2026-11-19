# Ambient Weather Network API (issue #51)

**From the published docs, 2026-11-18; the reply measured from a laptop
2026-11-19** against one real station (a club member's, with their keys).
Not yet measured: timing and TLS from the XIAO itself.

**Measured 2026-11-19 (one station on the account):** HTTP 200 in 0.28 s,
**1,563 bytes**; `lastData` has 30 fields; `info` carries `name` (the
station's Weather Underground ID, `K…-aws`), `location` and `coords`. The
firmware's parser (`AmbientParse.h`) read it correctly in both US and metric
units, and found the station's coordinates.

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

## Measured 2026-11-19: two API keys are not a pair

The first test keys from a station owner were **two API Keys** (both made on
the account page as "API Key"). Ambient refused them either way round with
HTTP 401 and a body that names the problem:

`{"error":"You are using an apiKey as your applicationKey. Please contact support@ambientweather.com to get an applicationKey"}`

So the server can tell the two kinds apart, and an owner needs one of each.
Where an owner gets an Application Key today is **not confirmed**: the
blueprint says the account page; this error text and some third-party docs
say Ambient support. The firmware shows the body's `error` text on the
settings page, not just the code.

The owner also sent the station's Weather Underground ID
(`K…-aws`), not its MAC: owners know their station by that ID or its name,
not by MAC. The MAC is in the `/devices` reply itself (`macAddress`), so a
working key pair is enough to find it.

The second pair (a real Application Key from the same account, 2026-11-19)
worked at once: no waiting for the key to be activated.

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
| MAC blank, several stations | `this Ambient account has N stations: name MAC, …` (so the owner can copy one) |
| account has no stations | `no stations on this Ambient account` |
| no outdoor temperature in `lastData` | `no outdoor temperature from this station` |

## Not used yet

- `/v1/devices/{mac}` (history, up to 288 readings) and the realtime
  Socket.io API (`rt2.ambientweather.net`): one reading every 10 minutes is
  all a page needs.
- Rain, pressure and indoor readings: in `lastData`, not on any orb yet.
