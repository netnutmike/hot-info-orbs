"""Attribute every line on a ref to scott / yang / upstream by git blame.

Feeds the numbers in build.py (copy them over by hand, then rebuild).
usage: measure_blame.py REPO REF   e.g. ~/code/hot-info-orbs origin/main
Lines authored before 2026-01-01 count as upstream (upstream's last commit is
2025-02; this repo's own work starts 2026-08). -w -M -C so a reformat or a
file move does not re-attribute upstream lines.
"""
import subprocess, sys, collections, os
from concurrent.futures import ThreadPoolExecutor

repo, ref = sys.argv[1], sys.argv[2]
CUT = 1767225600  # 2026-01-01 UTC
BIG = 20000


def git(*a):
    return subprocess.run(['git', '-C', repo, *a], capture_output=True).stdout


def bucket(rel):
    p = rel.split('/')
    if rel == 'firmware/src/core/booteyes/eyeData.h':
        return '8 eye graphics data (third-party)'
    if rel.startswith('firmware/lib/'):
        return '7 vendored libs (firmware/lib)'
    if p[0] == 'firmware':
        return '1 firmware source'
    if p[0] == 'test':
        return '2 host tests'
    if rel.startswith('docs/assets/power/') and rel.endswith('.csv'):
        return '8b power-meter captures (data)'
    if p[0] == 'hardware' and rel.endswith('.py'):
        return '3 hardware scripts'
    if p[0] in ('tools', 'web-examples', '.github'):
        return '4 tools / CI / web examples'
    if p[0] == 'hardware':
        return '9 hardware CAD/fab files'
    if p[0] in ('docs', 'references') or rel.endswith('.md'):
        return '5 docs'
    if p[0] in ('images', 'fonts'):
        return '9b image/font assets'
    return '6 repo root / config'


def who(author, t):
    if t < CUT:
        return 'upstream'
    if author == 'scottdube':
        return 'scott'
    if author.startswith('Yangmin'):
        return 'yang'
    return 'other2026:' + author


files = git('ls-tree', '-r', '--name-only', '-z', ref).decode().split('\0')
files = [f for f in files if f]


def one(rel):
    blob = git('show', f'{ref}:{rel}')
    if b'\0' in blob[:8192]:
        # binary: attribute the file by the commit that added it
        out = git('log', '--diff-filter=A', '--format=%an%x09%at', '-1', ref, '--', rel).decode().strip()
        a, t = out.split('\t') if out else ('?', '0')
        return rel, None, who(a, int(t))
    n = len(blob.splitlines())
    if n > BIG:
        out = git('log', '--diff-filter=A', '--format=%an%x09%at', '-1', ref, '--', rel).decode().strip()
        a, t = out.split('\t')
        return rel, {who(a, int(t)): n}, None
    out = git('blame', '-w', '-M', '-C', '--line-porcelain', ref, '--', rel).decode('utf-8', 'replace')
    c = collections.Counter()
    author = None
    for line in out.split('\n'):
        if line.startswith('author '):
            author = line[7:]
        elif line.startswith('author-time '):
            t = int(line[12:])
        elif line.startswith('\t'):
            c[who(author, t)] += 1
    return rel, c, None


with ThreadPoolExecutor(8) as ex:
    res = list(ex.map(one, files))

lines = collections.defaultdict(collections.Counter)
nfiles = collections.defaultdict(collections.Counter)   # by majority owner
binfiles = collections.defaultdict(collections.Counter)
touched = collections.defaultdict(collections.Counter)  # files with any scott line / pure upstream
for rel, c, binwho in res:
    b = bucket(rel)
    if c is None:
        binfiles[b][binwho] += 1
        continue
    lines[b].update(c)
    tot = sum(c.values())
    if tot == 0:
        continue
    up = c.get('upstream', 0)
    if up == tot:
        touched[b]['all upstream'] += 1
    elif up == 0:
        touched[b]['all new'] += 1
    else:
        touched[b]['mixed'] += 1

owners = sorted({k for c in lines.values() for k in c} | {k for c in binfiles.values() for k in c})
print('LINES by blame')
print(f"{'bucket':36}" + ''.join(f'{o[:18]:>19}' for o in owners) + f"{'total':>9}")
for b in sorted(lines):
    c = lines[b]
    print(f'{b:36}' + ''.join(f'{c.get(o, 0):19}' for o in owners) + f'{sum(c.values()):9}')
print()
print('TEXT FILES: all upstream / mixed / all new (2026)')
for b in sorted(touched):
    c = touched[b]
    print(f"{b:36} upstream-only={c['all upstream']:4} mixed={c['mixed']:4} new={c['all new']:4}")
print()
print('BINARY FILES by adding commit')
for b in sorted(binfiles):
    print(f'{b:36}', dict(binfiles[b]))
