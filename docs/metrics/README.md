# The codebase, by the numbers

A one-page fact sheet of the project's code: lines new versus inherited from
brettdottech/info-orbs, commits, tests, releases. Last measured on main at
aa3cd8f (firmware 1.10.1), 24 November 2026.

- `info-orbs-codebase-one-pager.pdf`: the sheet, letter size.
- Online copy: https://claude.ai/artifact/2pVRXizMRGzHFTzCt4QERy
- `one-pager.html`: the same page as HTML (`hot-color-logo.jpg` beside it).
- `build.py`: writes the HTML and prints it to PDF with headless Chrome. The
  numbers are typed into `ROWS`, `ORBS` and the text, from the measurement.
- `measure_blame.py`: the measurement. `git blame -w -M -C` on every file of a
  ref, lines bucketed by area and by who wrote them last; lines last written
  before 2026 count as upstream. Run it, then copy the numbers into
  `build.py` and rebuild:

```
python3 measure_blame.py .. origin/main
python3 build.py
```

Commits, merged PRs, releases, test cases (`RUN_TEST(` calls under `test/`)
and file counts come from `git` and `gh`; the commit message of each re-issue
records them. The same numbers are on slide 7 of the Design History deck and
slide 4 of the October 27 club deck in `docs/presentation/`.
