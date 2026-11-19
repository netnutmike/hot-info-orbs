# Weather stations besides Tempest: scope (issue #51)

**Agreed 2026-11-18** (decisions at the end). The goal is two things:
first, add **Ambient Weather Network** (Yang has a station and will send a
key pair for testing); second, make each later service a small, separate
addition.

## The finding that shapes everything: a station only gives "now"

A weather page fills all five orbs (`WeatherDataModel`):

| Orb | Shows | Where it comes from |
|---|---|---|
| 1 | time, date, weekday, label | clock + settings |
| 2 | the conditions in words | **forecast** |
| 3 | weather picture, sunrise/sunset, moon | **forecast** (icon, sun times); the moon is computed |
| 4 | temperature now, today's high and low | **station** (now); **forecast** (high/low) |
| 5 | the next three days | **forecast** |

Tempest hides this split because its `better_forecast` reply carries both
the station's reading and a forecast. The other station services return
**current readings only**: no words, no picture, no highs, no days. Checked
against each service's API docs on 2026-11-18:

| Service | Endpoint | Credentials | Forecast? | Rate limit |
|---|---|---|---|---|
| Ambient Weather Network (ambientweather**.net**) | `api.ambientweather.net/v1/devices` (each device carries `lastData`) | Application Key + API Key, both made on the site | no | 1 request/s per API key, 3/s per app key; 429 when over |
| Ecowitt | `api.ecowitt.net/api/v3/device/real_time` | application_key + api_key + the station's MAC | no | not published |
| Davis WeatherLink v2 | `api.weatherlink.com/v2/current/{station-id}` | `api-key` in the URL, **secret in an `X-Api-Secret` header** | no | per key, numbers not published |
| Weather Underground (PWS) | `api.weather.com/v2/pws/observations/current` | free key, only for owners who upload | a 5-day forecast on the same key | old sources say 500/day and 10/min; current limit not confirmed |

A page refreshes every 10 minutes while it is on screen, so even the
tightest of these limits is far away.

So a non-Tempest station page = **station for "now" + a forecast service for
the rest**, joined on one page. The station's own coordinates (Ambient
`info.coords`, WeatherLink `/stations`, WU `lat`/`lon`) say where to
forecast, so the user does not type a location twice.

## Proposed shape: two small interfaces instead of one big one

Today `WeatherSource::fetch(model)` fills the whole model. Split it:

- **`StationSource`**: fills "now": temperature, and humidity, wind and rain
  where the service has them (the page doesn't show those yet, but the
  model should carry them). It also reports the station's coordinates.
- **`ForecastSource`**: fills the conditions in words, the picture, today's
  high and low, sun times and three days, for a latitude/longitude.
- **`StationPage`** (a `WeatherSource`): one station plus one forecast,
  fetched in turn. If the forecast fails, the temperature still shows; the
  stale-data marks cover the parts that are old.

`WeatherWidget` doesn't change: it still draws one `WeatherDataModel`.

Current sources fit without rework:
- `TempestSource` stays a whole `WeatherSource`, since its single reply
  already fills everything.
- `VisualCrossingSource` gets a `ForecastSource` face as well.

### Adding the next service: one file, one table row

Each station type is a **descriptor** in one table:

```
{ id: "ambient", name: "Ambient Weather",
  fields: [ {key:"app", label:"Application Key", secret:true},
            {key:"api", label:"API Key", secret:true},
            {key:"mac", label:"Station MAC (blank = first)", secret:false} ],
  make: [](const StationFields&) -> StationSource* { ... } }
```

The settings page builds a station's form from its type's `fields`, and
`main.cpp` builds the page with `make`. So adding Ecowitt is:
- `EcowittSource.cpp`, with a host-tested parser beside it (as
  `TempestParse.h` is today);
- one table row;
- a line in the user guide.

The settings page, settings storage and `main.cpp` are not touched.

Forecast services use the same kind of table. That leaves room for a
**no-key forecast** later: Open-Meteo is free for non-commercial use, needs
no key, and asks for CC-BY attribution. With it, a station owner would need
only the station's own keys.

## Settings

- Each station slot gets a `type` and that type's fields. Secrets are
  write-only on the page, as the tokens are today.
- **Saved settings carry over:** a slot with `tstn1`/`tlbl1` set reads as
  type `tempest`. The shared Tempest token stays as it is.
- **Not waiting for #50:** keep the two slots, each with a type. #50 later
  turns the fixed slots into an Add/Remove list without changing the type
  model.
- The forecast service for a station page: Visual Crossing when its key is
  set. (Open-Meteo, if it is added, would be the default with no key.)

## Things to get right

1. **Keys stay out of the repo.** Yang's Ambient key pair is for testing
   only. It goes into the rig's settings page, never into `secrets.h`,
   tests or fixtures. Recorded test replies are scrubbed of the MAC and
   coordinates before they are committed.
2. **Ambient is .net, not .com** (the .com site sells the hardware).
3. **Memory:** the Ambient `/devices` reply is small (one object per
   station), so it fits in an ArduinoJson document with a filter. Tempest
   needed streaming (~96 KB); measure each new service's reply size before
   choosing.
4. **TLS:** all four services are HTTPS; check that each connects from the
   XIAO with the client the existing sources use.
5. **Units:** Ambient sends °F (`tempf`); the page's units setting must
   apply to every type, so each parser converts to one unit inside.
6. **An Ambient account with several stations:** pick by MAC, with blank
   meaning the first.

## Done when (first step: Ambient only)

- [ ] `StationSource` / `ForecastSource` / `StationPage`, with the type
      table, and Tempest and Visual Crossing still working as before.
- [ ] `AmbientSource`, with a host-tested parser.
- [ ] The settings page: a type picker on each station slot, with the form
      built from the table; old Tempest settings carry over untouched.
- [ ] On the rig, with Yang's key: a page that shows the station's
      temperature plus the forecast; a wrong key shows its error on the
      settings page, as Tempest's does today.
- [ ] Docs: the user guide (Ambient setup, where to make the two keys);
      `docs/AMBIENT-API.md` beside `TEMPEST-API.md`.
- [ ] Ecowitt, WeatherLink and WU each become a follow-up issue against this
      model, opened only when someone with that station wants it.

## Decisions (2026-11-18)

- **Ambient first.** It is the one with a real station and keys on hand.
- **Not waiting for #50.** Types go on the two existing slots now; #50
  turns them into a list later.
- **Forecast for station pages: Visual Crossing for the first step.** It
  is already in the firmware and keyed. A no-key forecast (Open-Meteo) is
  a follow-up through the same forecast table, not part of this step.
