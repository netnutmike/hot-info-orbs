"""Build the XIAO orb power report page from the committed PPK captures.

  python3 tools/power-report/build_report.py [out.html]

Reads docs/assets/power/xiao-2026-11-16/*.csv (10 kSa/s means, written by
tools/ppk), fills the /*DATA*/ slot in report_template.html, and writes
docs/power-report.html (committed) unless told otherwise.
The thermal pictures are inlined in the template. Published as a claude.ai
artifact on 2026-11-16; the findings themselves are in docs/POWER-MEASURED.md.
"""
import csv, json, os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
CAP = os.path.join(HERE, "..", "..", "docs", "assets", "power", "xiao-2026-11-16")


def load(name):
    with open(os.path.join(CAP, f"{name}.csv")) as f:
        return [float(r["mA"]) for r in csv.DictReader(f)]


data = {"inrush": []}
for name in ["inrush", "inrush_repeat1", "inrush_repeat2", "inrush_repeat3"]:
    v = load(name)
    # Skip the first 0.2 s: repeats 2 and 3 open with a few ms at ~360 mA
    # before the off level, cause not established. Power-on is at ~0.5 s.
    on = next(i for i, x in enumerate(v) if i > 2000 and x > 50)
    seg = v[on - 20:on + 30000]  # -2 ms .. 3 s at 10 kSa/s
    data["inrush"].append([round(max(seg[i:i + 20]), 1) for i in range(0, len(seg) - 19, 20)])  # 2 ms max
v = load("run_11.0min")  # on, ~11 min after boot, Stocks shown
data["steady"] = [round(sum(v[i:i + 100]) / 100, 1) for i in range(0, len(v) - 99, 100)]  # 10 ms means
data["steadyAvg"] = round(sum(v) / len(v), 2)

out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, "..", "..", "docs", "power-report.html")
with open(os.path.join(HERE, "report_template.html")) as f:
    tpl = f.read()
assert tpl.count("/*DATA*/") == 1
with open(out, "w") as f:
    f.write(tpl.replace("/*DATA*/", json.dumps(data, separators=(",", ":"))))
print(f"wrote {out}")
