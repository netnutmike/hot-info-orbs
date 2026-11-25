#!/usr/bin/env python3
"""Build the HOT Info Orbs codebase one-pager (HTML, then PDF via headless Chrome).

Numbers are measured, not typed from memory: measure_blame.py (git blame
-w -M -C) on hot-info-orbs origin/main at aa3cd8f (2026-11-24). "New" = last
written since 2026-08-27 by a HOT author (Scott 19,468 + Yang 2); "upstream" =
lines last written before 2026 (brettdottech/info-orbs). Re-measure before
re-issuing: run measure_blame.py, copy the numbers into ROWS/ORBS/the text.
First issue 2026-11-14 at b03a0c0: 12,420 new + 7,156 upstream = 19,576.
"""
import os, subprocess, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
HTML = os.path.join(HERE, 'one-pager.html')
PDF = os.path.join(HERE, 'info-orbs-codebase-one-pager.pdf')
CHROME = '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome'

# (label, new, upstream, note)
ROWS = [
    ('Firmware source', 10571, 6005, ''),
    ('Hardware scripts', 3043, 0, ''),
    ('Host tests', 4621, 0, ''),
    ('Tools, CI, web examples', 1235, 1027, ''),
    ('Docs', 12674, 165, 'not code'),
]
NEW, UP = '#C96F00', '#2E63A8'   # validated pair: CVD dE 23.6, both >= 3:1 on white

X0, BARW, ROWH, BARH = 170, 300, 30, 14
scale = BARW / max(n + u for _, n, u, _ in ROWS)
svg = []
y = 4
for i, (label, n, u, note) in enumerate(ROWS):
    if note:
        y += 10
        svg.append(f'<line x1="0" x2="672" y1="{y - 2}" y2="{y - 2}" stroke="#D8DEE4" stroke-dasharray="2 3"/>')
        y += 6
    cy = y + ROWH / 2
    by = cy - BARH / 2
    svg.append(f'<text x="{X0 - 12}" y="{cy + 4}" text-anchor="end" class="lab">{label}</text>')
    wn = max(n * scale, 2)
    svg.append(f'<rect x="{X0}" y="{by}" width="{wn:.1f}" height="{BARH}" rx="3" fill="{NEW}"/>')
    x = X0 + wn
    if u:
        wu = max(u * scale, 2)
        svg.append(f'<rect x="{x + 2:.1f}" y="{by}" width="{wu:.1f}" height="{BARH}" rx="3" fill="{UP}"/>')
        x += 2 + wu
    txt = f'<tspan class="vb">{n:,}</tspan> new'
    txt += f' · <tspan class="vb">{u:,}</tspan> upstream' if u else ' · none inherited'
    if note:
        txt += f' <tspan class="note">({note})</tspan>'
    svg.append(f'<text x="{x + 10:.1f}" y="{cy + 4}" class="val">{txt}</text>')
    y += ROWH
svg_h = y + 4
chart = f'<svg viewBox="0 0 672 {svg_h}" width="672" height="{svg_h}" role="img" aria-label="Lines on main by area, new versus upstream">' + ''.join(svg) + '</svg>'

ORBS = [
    ('19,470', 'new lines', 'of project code'),
    ('73%', 'of the code', 'is new'),
    ('626', 'commits', 'in 13 weeks'),
    ('405', 'host tests', 'none inherited'),
    ('14', 'releases', '1.2.0 to 1.10.1'),
]
orbs = ''.join(f'<div class="orb"><b>{a}</b><span>{b}</span><i>{c}</i></div>' for a, b, c in ORBS)

html = f'''<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<title>HOT Info Orbs: the codebase by the numbers</title>
<style>
@page {{ size: 8.5in 11in; margin: 0 }}
* {{ box-sizing: border-box; -webkit-print-color-adjust: exact; print-color-adjust: exact }}
:root {{ --paper:#F3F5F7; --surface:#FFFFFF; --ink:#15202B; --muted:#56636E; --rule:#D8DEE4;
  --accent:#C96F00; --mask:#1C5D54; --glass:#0A0E12; --rim:#2A333C; --orb-text:#E9EEF2; --orb-orange:#FF9200;
  --display:"Avenir Next","Helvetica Neue",Arial,sans-serif; --mono:"SF Mono",Menlo,Consolas,monospace }}
html,body {{ margin:0; padding:0 }}
body {{ width:8.5in; height:11in; background:var(--paper); color:var(--ink); font-family:var(--display);
  font-size:10.5pt; line-height:1.4; padding:0.5in 0.55in 0.4in; display:flex; flex-direction:column; gap:17px; overflow:hidden }}
.eyebrow {{ font-family:var(--mono); font-size:7.5pt; letter-spacing:.16em; text-transform:uppercase; color:var(--muted) }}
header {{ display:flex; justify-content:space-between; align-items:flex-start; gap:24px }}
.badge {{ flex:none; width:116px; height:116px; border-radius:20%; overflow:hidden; background:#000;
  box-shadow:0 0 0 5px var(--rim); margin:6px 6px 0 0 }}
.badge img {{ display:block; width:100%; height:100% }}
h1 {{ font-size:31pt; line-height:1.02; margin:6px 0 8px; font-weight:800; letter-spacing:-.01em }}
h1 em {{ font-style:normal; color:var(--accent) }}
.lede {{ font-size:11.5pt; color:var(--muted); max-width:5.6in; margin:0 }}
.orbs {{ display:flex; justify-content:space-between; padding:2px 6px }}
.orb {{ width:122px; height:122px; border-radius:50%; background:radial-gradient(circle at 35% 30%,#1a222b 0%,var(--glass) 62%);
  box-shadow:0 0 0 5px var(--rim); display:flex; flex-direction:column; align-items:center; justify-content:center;
  color:var(--orb-text); font-family:var(--mono); text-align:center; gap:1px }}
.orb b {{ font-size:19pt; color:var(--orb-orange); font-weight:700; line-height:1.1 }}
.orb span {{ font-size:7.6pt; letter-spacing:.04em }}
.orb i {{ font-style:normal; font-size:6.6pt; color:#8A97A3 }}
section.card {{ background:var(--surface); border:1px solid var(--rule); padding:14px 18px 10px }}
h2 {{ font-family:var(--mono); font-size:7.8pt; font-weight:600; letter-spacing:.14em; text-transform:uppercase;
  color:var(--accent); margin:0 0 8px; padding-bottom:5px; border-bottom:2px solid var(--accent) }}
.charthead {{ display:flex; justify-content:space-between; align-items:baseline; margin-bottom:2px }}
.charthead p {{ margin:0; font-weight:700; font-size:11.5pt }}
.legend {{ display:flex; gap:16px; font-size:8.5pt; color:var(--muted) }}
.legend i {{ display:inline-block; width:11px; height:11px; border-radius:3px; vertical-align:-1px; margin-right:5px }}
svg text {{ font-family:var(--display); fill:var(--ink) }}
svg .lab {{ font-size:9.5pt; font-weight:600 }}
svg .val {{ font-size:8.8pt; fill:var(--muted) }}
svg .vb {{ font-weight:700; fill:var(--ink); font-family:var(--mono); font-size:8.6pt }}
svg .note {{ font-style:italic }}
.cols {{ display:grid; grid-template-columns:repeat(3,1fr); gap:14px }}
.cols .card {{ padding:13px 15px 12px }}
.cols p {{ margin:0; font-size:9.6pt }}
.cols .big {{ font-size:15pt; font-weight:800; line-height:1.15; margin-bottom:5px }}
.cols .big small {{ font-size:9pt; font-weight:600; color:var(--muted) }}
.strip {{ display:flex; justify-content:space-between; gap:10px; border-top:1px solid var(--rule); border-bottom:1px solid var(--rule); padding:9px 2px }}
.strip div {{ font-size:8.4pt; color:var(--muted); line-height:1.3 }}
.strip b {{ display:block; font-family:var(--mono); font-size:11.5pt; color:var(--ink) }}
footer {{ margin-top:auto; font-size:7pt; line-height:1.45; color:var(--muted) }}
footer .url {{ font-family:var(--mono); color:var(--ink); font-size:7.6pt }}
</style></head><body>

<header>
  <div>
    <div class="eyebrow">HOT Info Orbs · codebase fact sheet · November 2026</div>
    <h1>Five round screens.<br><em>26,502 lines</em> behind them.</h1>
    <p class="lede">A desk widget on one XIAO ESP32-S3 or S3 Plus, rebuilt in the open on top of
    brettdottech/info-orbs. Here is what 13 weeks of club work looks like when you count it.</p>
  </div>
  <div class="badge"><img src="hot-color-logo.jpg" alt="Hands-On Tech"></div>
</header>

<div class="orbs">{orbs}</div>

<section class="card">
  <h2>Where the lines are</h2>
  <div class="charthead"><p>Lines on main, by who wrote them last</p>
    <div class="legend"><span><i style="background:{NEW}"></i>New since Aug 27, 2026</span><span><i style="background:{UP}"></i>Inherited from upstream</span></div></div>
  {chart}
</section>

<div class="cols">
  <section class="card"><h2>Built on good bones</h2>
    <p class="big">91% <small>still in service</small></p>
    <p>Of the 6,627 firmware lines inherited from upstream, 6,005 are still running. We added to
    the original; we did not bulldoze it.</p></section>
  <section class="card"><h2>Tested before flashed</h2>
    <p class="big">405 tests <small>in 18 suites</small></p>
    <p>4,621 lines of host tests run on a laptop before anything touches an orb. Upstream shipped
    none, so every one of them is new.</p></section>
  <section class="card"><h2>Shipped, not just pushed</h2>
    <p class="big">7.0 commits <small>a day</small></p>
    <p>626 commits and 54 merged pull requests between Aug 27 and Nov 24, cut into 14 firmware
    releases, 1.2.0 through 1.10.1.</p></section>
</div>

<div class="strip">
  <div><b>677</b>files in the repo</div>
  <div><b>125</b>firmware source files,<br>58 of them brand new</div>
  <div><b>7</b>widget modules</div>
  <div><b>12,674</b>lines of new docs</div>
  <div><b>91,037</b>lines of vendored libraries<br>(not ours, not counted)</div>
  <div><b>AGPL-3.0</b>open source,<br>same as upstream</div>
</div>

<footer>
  <b>How we counted.</b> git blame (ignoring whitespace-only changes and moved lines) on the main branch at commit aa3cd8f,
  November 24, 2026. Line counts include blank lines and comments. "New" means last written since August 27, 2026;
  "upstream" means last written before then, in brettdottech/info-orbs. "Project code" is firmware source, host tests,
  hardware scripts, tools and CI: 19,470 new + 7,032 upstream = 26,502. Not counted as project code: docs, vendored
  libraries, 13,362 lines of third-party eye graphics data, the power-meter CSV captures, and the KiCad, Gerber and 3D model files. Work on unmerged
  branches is not included. Commit authorship is as recorded by git. Past commit rates are no guarantee of future commits.
  Screens sold separately. &nbsp; <span class="url">github.com/scottdube/hot-info-orbs</span>
</footer>
</body></html>
'''
open(HTML, 'w').write(html)

with tempfile.TemporaryDirectory() as prof:
    r = subprocess.run([CHROME, '--headless=new', '--disable-gpu', f'--user-data-dir={prof}',
                        '--no-pdf-header-footer', f'--print-to-pdf={PDF}', 'file://' + HTML],
                       capture_output=True, text=True, timeout=120)
print('chrome exit', r.returncode, (r.stderr or '').strip().splitlines()[-1:] )
print(PDF, os.path.getsize(PDF) if os.path.exists(PDF) else 'MISSING')
