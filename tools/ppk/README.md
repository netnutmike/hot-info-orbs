# PPK II capture scripts

Used for the XIAO measurement in `docs/POWER-MEASURED.md` (2026-11-16).
Needs `pip install ppk2-api pyserial`. Close nRF Connect first: only one
program can hold the PPK's port.

- `ppk_session.py <minutes> <outdir>` powers the orb at 5.000 V and captures
  the inrush. It then takes 10 s windows at +60 s and every 5 min. Touch
  `<outdir>/capture_<tag>` for an extra window, or `<outdir>/stop` to end.
  The output goes OFF when it exits.
- `ppk_inrush_repeat.py <n> <off_secs> <outdir>` runs n cold power-ons.

Keep one process for the whole test. The PPK drops its output when the
program holding its port exits, so separate runs each start from a cold orb.
`docs/TRAPS.md`, "PPK II from a script", has the rest.
