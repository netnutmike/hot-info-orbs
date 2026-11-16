"""Repeat the cold power-on inrush N times in one PPK session (2026-11-16).

  ppk_inrush_repeat.py <n> <off_secs> <outdir>

Each cycle: output OFF for <off_secs>, then a 3.5 s window with power ON at
0.5 s, then OFF. Ends with the output OFF. Appends to <outdir>/session.log.
"""
import os, sys, time, statistics

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from ppk_common import open_clean  # noqa: E402

MV = 5000
n, off_secs, outdir = int(sys.argv[1]), float(sys.argv[2]), sys.argv[3]
LOG = os.path.join(outdir, "session.log")


def log(msg):
    line = f"{time.strftime('%H:%M:%S')} {msg}"
    print(line, flush=True)
    with open(LOG, "a") as f:
        f.write(line + "\n")


def inrush(p, tag):
    chunks = []
    p.start_measuring()
    t0 = time.time()
    on = False
    while time.time() - t0 < 3.5:
        if not on and time.time() - t0 >= 0.5:
            p.toggle_DUT_power("ON")
            on = True
        d = p.ser.read(65536)
        if d:
            chunks.append(d)
    p.stop_measuring()
    time.sleep(0.3)
    p.ser.reset_input_buffer()
    samples, _ = p.get_samples(b"".join(chunks))
    ma = [x / 1000.0 for x in samples]
    n10 = len(ma) // 10
    ma10 = [sum(ma[i * 10:i * 10 + 10]) / 10 for i in range(n10)]
    with open(os.path.join(outdir, f"{tag}.csv"), "w") as f:
        f.write("t_s,mA\n")
        for i, v in enumerate(ma10):
            f.write(f"{i / 10000:.4f},{v:.3f}\n")
    k = ma.index(max(ma))
    bins = [statistics.fmean(ma10[i:i + 1000]) for i in range(0, len(ma10) - 999, 1000)]
    log(f"[{tag}] {len(ma)} samples ({len(ma) / 1e5:.2f} s of 3.5 s) "
        f"max100k {max(ma):.1f} mA @ {k / 1e5:.4f} s | max10k {max(ma10):.1f} mA | "
        f"samples >500 mA: {sum(1 for x in ma if x > 500)}")
    log(f"[{tag}] 100 ms means: " + " ".join(f"{b:.0f}" for b in bins))


p = open_clean()
p.ser.timeout = 0.05
p.use_source_meter()
p.set_source_voltage(MV)
try:
    for i in range(1, n + 1):
        p.toggle_DUT_power("OFF")
        log(f"inrush repeat {i}/{n}: output OFF, waiting {off_secs:.0f} s")
        time.sleep(off_secs)
        inrush(p, f"inrush_repeat{i}")
finally:
    p.toggle_DUT_power("OFF")
    log("inrush repeats done, output OFF")
