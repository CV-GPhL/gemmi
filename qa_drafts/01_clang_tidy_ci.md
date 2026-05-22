# Draft 1: clang-tidy informational CI job

## Goal

Add a clang-tidy job to CI that flags missing struct member initializers
(the #413-#422 regression class). Start informational - `continue-on-error:
true` so it never blocks a PR - and start with a single check
(`cppcoreguidelines-pro-type-member-init`). The maintainer can decide later
whether to make it required or to broaden the check list.

## Rationale

- Catches exactly the bug class that motivated this issue.
- `CMAKE_EXPORT_COMPILE_COMMANDS ON` is already set in CMakeLists.txt so the
  compile database is built for free.
- Single check keeps noise out of the log; baseline run on master surfaces
  ~15 unique warnings, all in pre-existing code (factory-filled fields like
  `mtz.hpp Column::parent`, `symmetry.hpp Op::rot/tran`). They are not real
  bugs but they are also not blocking with `continue-on-error: true`.
- If maintainer wants the existing warnings silenced, a `.clang-tidy`
  config can mark those specific structs with `// NOLINT` comments in a
  follow-up. Out of scope for this draft.

## Files changed

- `.github/workflows/ci.yml` - add one new job

## Snippet to append to ci.yml

Insert as a new top-level job (suggested position: just before the `docs`
job so it groups with the other build/test jobs):

```yaml
  clang_tidy:
    name: "clang-tidy (informational)"
    runs-on: ubuntu-latest
    if: "!contains(github.event.head_commit.message, '[skip ci]')"
    continue-on-error: true
    steps:
    - uses: actions/checkout@v4
    - name: apt-get
      run: |
        sudo apt-get update
        sudo apt-get install -y clang-tidy cmake g++ libz-dev
    - name: generate compile_commands.json
      run: |
        cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    - name: run clang-tidy
      run: |
        clang-tidy --version
        clang-tidy -p build \
          --checks='-*,cppcoreguidelines-pro-type-member-init' \
          --warnings-as-errors='*' \
          src/*.cpp prog/*.cpp 2>&1 | tee clang-tidy.log
    - uses: actions/upload-artifact@v4
      if: always()
      with:
        name: clang-tidy-log
        path: clang-tidy.log
        retention-days: 14
```

Notes on the snippet:

- `--warnings-as-errors='*'` makes the job's exit code reflect the warning
  count - combined with `continue-on-error: true` at the job level, this
  means: the job goes red in the matrix when there are warnings, but the
  overall workflow stays green. The maintainer can see the red mark and
  click through to the log without it blocking merges. If they prefer
  silent reporting instead, drop the `--warnings-as-errors`.
- Globbing `src/*.cpp prog/*.cpp` runs against the compiled translation
  units; clang-tidy uses the compile DB to pick up the right -I/-std flags.
- The check list is intentionally narrow. Broader categories
  (`cppcoreguidelines-*`, `clang-analyzer-*`) can be added later if
  desired - they would significantly increase the baseline noise.

## Local test before opening PR

```bash
cd /path/to/gemmi
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
clang-tidy -p build \
  --checks='-*,cppcoreguidelines-pro-type-member-init' \
  src/*.cpp prog/*.cpp 2>&1 | tee /tmp/clang-tidy.log
grep -c 'warning:' /tmp/clang-tidy.log
```

Expected on current master (with #427 and #428 merged): ~15 warnings, all
in pre-existing factory-filled structs. None of them are real regressions.

## PR title

```
ci: add informational clang-tidy job for member-init regression class
```

## PR body

```markdown
Follow-up to #429.

Adds a `clang_tidy` CI job that runs clang-tidy with a narrow check list
(`cppcoreguidelines-pro-type-member-init`) against the compile database
that the project already exports via `CMAKE_EXPORT_COMPILE_COMMANDS`.

The job is informational only:

- `continue-on-error: true` - never blocks a PR.
- Single check - keeps the log readable.
- Uploads the full log as a build artifact for inspection.

Running this on current master surfaces ~15 warnings, all in pre-existing
factory-filled structs (e.g. `mtz.hpp Column::parent`, `symmetry.hpp
Op::rot/tran`). None are regressions; they reflect intentional design.

The intent is that this job goes red the moment another #413-class
regression is introduced, without surfacing noise from the baseline. If
you prefer to suppress the existing warnings to make "red == new regression
only", I can submit a follow-up adding either targeted `// NOLINT` comments
or a `.clang-tidy` config that excludes those files.

If you would rather not see red on baseline noise, dropping
`--warnings-as-errors='*'` keeps the job green and just records the
warnings as an artifact - happy to flip that based on your preference.
```
