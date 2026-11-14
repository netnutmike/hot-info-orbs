#!/usr/bin/env python3
"""Host test for the "newer version" check on the settings and update pages.

The check is a script the browser runs (firmware/src/core/ota/UpdateCheck.h),
so `pio test -e native`, which compiles C++, cannot run it. This pulls the
script out of the header and runs its two pure functions in a JavaScript
engine: node where there is one (the CI runners), the jsc that ships with
macOS otherwise.

    python3 tools/test_update_check.py            run the cases
    python3 tools/test_update_check.py --page F   also write F, a page for a
                                                  by-eye check in a browser
                                                  against the live GitHub answer

Rejected: a copy of the compare in C++ under test/. It would test the copy.
"""

import json
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
HEADER = ROOT / "firmware" / "src" / "core" / "ota" / "UpdateCheck.h"
MAC_JSC = "/System/Library/Frameworks/JavaScriptCore.framework/Versions/Current/Helpers/jsc"

REL = "7d596023cc8f085c8ec8b66b4a037e2e48f00371"  # shape of a release's target_commitish

# (running version, running commit, newest tag, newest release commit) -> state
STATE_CASES = [
    (("1.7.0", "abc1234", "firmware-1.8.0", REL), "newer"),
    (("1.7.0", "7d59602", "firmware-1.8.0", REL), "newer"),
    (("1.9.0", "abc1234", "firmware-1.10.0", REL), "newer"),  # numbers, not text: 10 > 9
    (("1.10.0", "abc1234", "firmware-1.9.0", REL), "ahead"),
    (("2.0.0", "abc1234", "firmware-1.99.99", REL), "ahead"),
    (("1.6.1", "7d59602", "firmware-1.6.1", REL), "current"),  # the release file itself
    (("1.6.1", "7d596023cc", "firmware-1.6.1", REL), "current"),  # git may print a longer short hash
    (("1.6.1", "abc1234", "firmware-1.6.1", REL), "local"),  # same number, other commit
    (("1.6.1", "7d59602-dirty", "firmware-1.6.1", REL), "local"),
    (("1.6.1", "unknown", "firmware-1.6.1", REL), "local"),  # built outside git
    (("1.6.1", "", "firmware-1.6.1", REL), "local"),
    (("1.6.1", "abc1234", "firmware-1.6.1", "main"), "current"),  # release made from a branch: cannot tell
    (("1.6.1", "abc1234", "firmware-1.6.1", None), "current"),
    (("1.6.1", "abc1234", "v1.1-run1", REL), "unknown"),  # a board tag marked Latest by mistake
    (("1.6.1", "abc1234", "1.6.1", REL), "unknown"),
    (("1.6.1", "abc1234", "firmware-1.6", REL), "unknown"),
    (("1.6.1", "abc1234", None, None), "unknown"),  # an error reply has no tag_name
    (("dev", "abc1234", "firmware-1.6.1", REL), "unknown"),
]

# (state, newest version) -> the words on the page. Every state is told apart
# by its words or a glyph, never by color.
TEXT_CASES = [
    (("newer", "1.8.0"), "⬆ Version 1.8.0 is available."),
    (("current", "1.8.0"), "✔ This is the newest version."),
    (("local", "1.8.0"), "Local build, not a release file. Newest release: 1.8.0."),
    (("ahead", "1.6.1"), "This build is newer than the newest release, 1.6.1."),
    (("unknown", ""), "Could not check for a newer version."),
]

# newest tag -> the version shown (rebuilt from the numbers, so nothing GitHub
# sends reaches the page as text)
LATEST_CASES = [
    ("firmware-1.8.0", "1.8.0"),
    ("firmware-01.08.0", "1.8.0"),
    ("firmware-1.8.0<b>", ""),
    ("v1.1-run1", ""),
    (None, ""),
]


def script():
    if not HEADER.exists():
        sys.exit(f"FAIL: {HEADER.relative_to(ROOT)} does not exist")
    found = re.search(r"<script>\n(.*?)</script>", HEADER.read_text(encoding="utf-8"), re.S)
    if not found:
        sys.exit(f"FAIL: no <script> ... </script> in {HEADER.relative_to(ROOT)}")
    return found.group(1)


def engine():
    node = shutil.which("node")
    if node:
        return [node]
    if Path(MAC_JSC).exists():
        return [MAC_JSC]
    sys.exit("FAIL: no JavaScript engine (need node, or macOS for its jsc)")


def run_js(js):
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "cases.js"
        path.write_text(js, encoding="utf-8")
        # utf-8 named: the answers carry glyphs, and Windows would decode them as cp1252
        done = subprocess.run(engine() + [str(path)], capture_output=True, encoding="utf-8")
    if done.returncode != 0:
        sys.exit(f"FAIL: the script did not run:\n{done.stdout}{done.stderr}")
    return json.loads(done.stdout.strip().splitlines()[-1])


def write_page(path):
    """One page per case of interest, each in its own frame so every copy of
    the script finds its own marker, all asking the live GitHub API."""
    runs = [
        ("settings", "s", "0.0.1", "abc1234", "an old release: newer is available, link to /update"),
        ("update", "u", "0.0.1", "abc1234", "an old release: newer is available"),
        ("settings", "s", "99.0.0", "abc1234", "ahead of the newest release"),
        ("settings", "s", "LATEST", "abc1234", "same number, other commit: local build, link to /update"),
        ("settings", "s", "LATEST", "COMMIT", "the release file: newest version"),
        ("settings", "s", "1.6.1", "abc1234", "repository that does not exist: could not check"),
    ]
    frames = []
    for i, (name, page, version, commit, what) in enumerate(runs):
        repo = "scottdube/hot-info-orbs" if i != len(runs) - 1 else "scottdube/no-such-repository"
        marker = f"<span id='upd' style='display:block' data-p='{page}' data-r='{repo}' data-v='{version}' data-c='{commit}'></span>"
        doc = f"<body style='font-family:sans-serif;margin:4px'>Running {version}, commit {commit}{marker}<script>{script()}</script></body>"
        frames.append((what, name, doc))
    out = [
        "<!DOCTYPE html><meta charset='utf-8'><title>update check by eye</title><body style='font-family:sans-serif'>",
        "<p>LATEST and COMMIT are filled in from GitHub below, then the frames load.</p>",
    ]
    for i, (what, name, _) in enumerate(frames):
        out.append(f"<p><b>{i + 1}. {name} page, {what}</b><br><iframe id='f{i}' style='width:95%;height:4em;border:1px solid #999'></iframe></p>")
    # "</" escaped: a "</script>" inside the list would end this script early
    out.append("<script>const docs=" + json.dumps([d for _, _, d in frames]).replace("</", "<\\/") + ";")
    out.append(
        "fetch('https://api.github.com/repos/scottdube/hot-info-orbs/releases/latest').then(r=>r.json()).then(j=>{"
        "const v=(j.tag_name||'').replace('firmware-',''),c=(j.target_commitish||'').slice(0,7);"
        "docs.forEach((d,i)=>{document.getElementById('f'+i).srcdoc=d.split('LATEST').join(v).split('COMMIT').join(c)})})</script>"
    )
    Path(path).write_text("\n".join(out), encoding="utf-8")
    print(f"wrote {path}")


def main():
    js = script()
    calls = (
        ["orbUpdateState(%s)" % ",".join(json.dumps(a) for a in args) for args, _ in STATE_CASES]
        + ["orbUpdateText(%s)" % ",".join(json.dumps(a) for a in args) for args, _ in TEXT_CASES]
        + ["orbUpdateLatest(%s)" % json.dumps(tag) for tag, _ in LATEST_CASES]
    )
    wanted = [w for _, w in STATE_CASES] + [w for _, w in TEXT_CASES] + [w for _, w in LATEST_CASES]
    js += "\nvar out=(typeof console!=='undefined'&&console.log)?console.log:print;\n"
    js += "out(JSON.stringify([" + ",".join(calls) + "]));\n"
    got = run_js(js)
    failed = 0
    for call, want, have in zip(calls, wanted, got):
        if want != have:
            failed += 1
            print(f"FAIL: {call}\n  want {want!a}\n  got  {have!a}")
    print(f"{len(calls) - failed} of {len(calls)} cases passed")
    if "--page" in sys.argv:
        write_page(sys.argv[sys.argv.index("--page") + 1])
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
