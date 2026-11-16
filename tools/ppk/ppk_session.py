"""One PPK II session for the XIAO orb power test (2026-11-16).

Why one process: the PPK II drops its output when the program holding its
serial port exits, so separate capture runs each started from a cold orb
(run2 caught a boot; run3 read 0 mA). This process holds the port, so the
orb stays powered for the whole session.

  ppk_session.py <minutes> <outdir>

- power OFF, start sampling, power ON at 0.5 s, keep 3.5 s  -> inrush
- 10 s windows at +60 s and then every 5 min                 -> run_<min>
- a file named  capture_<tag>  in <outdir> triggers a 10 s window named <tag>
  at once (for dim / off states); the file is removed after
- a file named  stop  in <outdir> ends the session
Power goes OFF at the end. Reports append to <outdir>/session.log; each
window's 10 kSa/s trace (10-sample means) is saved as <tag>.csv.
"""
import os, sys, time, statistics, glob

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from ppk_common import open_clean  # noqa: E402

MV = 5000
minutes, outdir = float(sys.argv[1]), sys.argv[2]
os.makedirs(outdir, exist_ok=True)
LOG = os.path.join(outdir, "session.log")


def log(msg):
    line = f"{time.strftime('%H:%M:%S')} {msg}"
    print(line, flush=True)
    with open(LOG, "a") as f:
        f.write(line + "\n")


def window(p, secs, tag, power_on_at=None):
    chunks = []
    p.start_measuring()
    t0 = time.time()
    switched = power_on_at is None
    while time.time() - t0 < secs:
        if not switched and time.time() - t0 >= power_on_at:
            p.toggle_DUT_power("ON")
            switched = True
        d = p.ser.read(65536)  # blocking bulk read keeps up with 400 kB/s
        if d:
            chunks.append(d)
    p.stop_measuring()
    time.sleep(0.3)
    p.ser.reset_input_buffer()
    raw = b"".join(chunks)
    samples, _ = p.get_samples(raw)
    ma = [x / 1000.0 for x in samples]
    n10 = len(ma) // 10
    ma10 = [sum(ma[i * 10:i * 10 + 10]) / 10 for i in range(n10)]
    with open(os.path.join(outdir, f"{tag}.csv"), "w") as f:
        f.write("t_s,mA\n")
        for i, v in enumerate(ma10):
            f.write(f"{i / 10000:.4f},{v:.3f}\n")
    avg = statistics.fmean(ma)
    bins = [statistics.fmean(ma10[i:i + 1000]) for i in range(0, len(ma10) - 999, 1000)]
    log(f"[{tag}] {len(ma)} samples ({len(ma) / 100000:.2f} s of {secs} s) "
        f"avg {avg:.2f} mA = {avg * MV / 1e6:.3f} W | "
        f"max100k {max(ma):.1f} mA @ {ma.index(max(ma)) / 1e5:.4f} s | "
        f"max10k {max(ma10):.1f} mA | min10k {min(ma10):.1f} mA")
    log(f"[{tag}] 100 ms means: " + " ".join(f"{b:.0f}" for b in bins))


p = open_clean()
p.ser.timeout = 0.05
p.use_source_meter()
p.set_source_voltage(MV)
p.toggle_DUT_power("OFF")
time.sleep(2.0)
start = time.time()
log(f"session start, {minutes} min, 5.000 V source meter")
window(p, 3.5, "inrush", power_on_at=0.5)
next_run = start + 60
try:
    while time.time() - start < minutes * 60:
        if os.path.exists(os.path.join(outdir, "stop")):
            log("stop file seen")
            break
        for trig in glob.glob(os.path.join(outdir, "capture_*")):
            tag = os.path.basename(trig)[len("capture_"):]
            os.remove(trig)
            window(p, 10, tag)
        if time.time() >= next_run:
            window(p, 10, f"run_{(time.time() - start) / 60:04.1f}min")
            next_run += 300
        time.sleep(0.5)
finally:
    p.toggle_DUT_power("OFF")
    log("power OFF, session end")
