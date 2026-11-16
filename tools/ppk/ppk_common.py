"""Open a PPK II cleanly. PPK_PORT overrides the port; otherwise the first of the
pair the PPK enumerates (on macOS the lower-numbered one answered)."""
import os, time
from ppk2_api.ppk2_api import PPK2_API, PPK2_Command
PORT = os.environ.get("PPK_PORT") or sorted(PPK2_API.list_devices())[0]
def quiesce(p):
    for _ in range(20):
        p.stop_measuring(); time.sleep(0.3); p.ser.reset_input_buffer()
        time.sleep(0.3)
        if p.ser.in_waiting == 0:
            return
    raise SystemExit("PPK will not stop streaming")
def open_clean():
    p = PPK2_API(PORT, timeout=1, write_timeout=1, exclusive=True)
    for attempt in range(4):
        quiesce(p)
        p._write_serial((PPK2_Command.GET_META_DATA,))
        raw = b""; t0 = time.time()
        while time.time() - t0 < 3 and b"END" not in raw:
            time.sleep(0.05); raw += p.ser.read(p.ser.in_waiting)
        txt = raw.decode("utf-8", errors="ignore")
        if "Calibrated" in txt and "END" in txt:
            p._parse_metadata(txt[txt.index("Calibrated"):])
            return p
    raise SystemExit("no clean metadata after 4 tries")
