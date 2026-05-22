# Draft 3: ASAN + UBSAN CI job

## Goal

Add a CI job that builds with `-fsanitize=address,undefined` and runs the
test suite. Sanitizers catch uninitialized-pointer dereferences and other
runtime UB that compiler warnings and static analysis miss.

## Rationale

- The current `ubuntu2404` job runs valgrind, but valgrind catches
  memory-leak / use-after-free issues, not all uninitialized-variable
  access patterns. ASAN catches more, faster, with better diagnostics.
- UBSAN is cheap to add alongside ASAN and catches a complementary class
  (signed overflow, alignment, null-deref through reference, etc).
- This job stays as ASAN+UBSAN only - MSAN would require rebuilding all
  dependencies (libz, nanobind) under MSAN, which is overkill.

## Risks (note these in PR body)

- Sanitizers slow down test runs significantly (~2-3x). With one extra
  job in parallel this should still finish inside the macos15 window so
  no slowdown of overall CI.
- Sanitizers can flag existing latent bugs not introduced by the PR
  triggering the CI. If that happens, the maintainer can either fix the
  underlying issue or mark the job `continue-on-error: true` until fixed.
  Draft below starts WITHOUT continue-on-error - happy to add it on
  request.

## Files changed

- `.github/workflows/ci.yml` - add one new job

## Snippet to append to ci.yml

Insert as a new top-level job (suggested position: just after the
`ubuntu2404` job, before `docs`):

```yaml
  ubuntu2404_sanitizers:
    name: "Ubuntu 24.04 (ASAN+UBSAN)"
    runs-on: ubuntu-latest
    if: "!contains(github.event.head_commit.message, '[skip ci]')"
    env:
      CFLAGS: "-fsanitize=address,undefined -fno-omit-frame-pointer -O1 -g"
      CXXFLAGS: "-fsanitize=address,undefined -fno-omit-frame-pointer -O1 -g"
      LDFLAGS: "-fsanitize=address,undefined"
      ASAN_OPTIONS: "detect_leaks=0:abort_on_error=0:halt_on_error=0"
      UBSAN_OPTIONS: "print_stacktrace=1:halt_on_error=1"
    steps:
    - uses: actions/checkout@v4
    - name: apt-get
      run: |
        sudo apt-get update
        sudo apt-get install -y libz-dev g++ cmake
    - name: build
      run: |
        g++ --version
        cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DEXTRA_WARNINGS=ON
        cmake --build build -j2
    - name: run C++ tests
      run: |
        cmake --build build --target check -j2
        cd build && ctest --output-on-failure
```

Notes on the snippet:

- Debug build with -O1 (not -O0): -O0 makes ASAN extremely slow without
  catching meaningfully more.
- `detect_leaks=0`: ASAN's leak detector reports many false positives in
  C++ codebases that intentionally hold global state for the program's
  lifetime. Leak detection is best left to a separate dedicated job (or
  valgrind, which is already in `ubuntu2404`).
- `halt_on_error=0` for ASAN: lets all tests run and surface every
  finding, not just the first.
- `halt_on_error=1` for UBSAN: UBSAN findings are typically clear-cut.
- Skipped the Python test step intentionally: ASAN against a Python
  module requires `LD_PRELOAD=$(g++ -print-file-name=libasan.so)` and
  has historically been flaky for projects that build C++ via skbuild.
  Can be added in a follow-up if the maintainer asks.

## Local test before opening PR

```bash
cd /path/to/gemmi
rm -rf build
CXXFLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -O1 -g" \
LDFLAGS="-fsanitize=address,undefined" \
  cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DEXTRA_WARNINGS=ON
cmake --build build -j
ASAN_OPTIONS=detect_leaks=0:halt_on_error=0 \
UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 \
  cmake --build build --target check -j
```

If any sanitizer finding shows up locally, decide before opening the PR:
either fix the underlying issue (preferred, if small) or note it in the
PR body so the maintainer is not surprised.

## PR title

```
ci: add ASAN+UBSAN job
```

## PR body

```markdown
Follow-up to #429.

Adds a CI job that builds gemmi with `-fsanitize=address,undefined` and
runs the C++ test suite under the sanitizers. Catches uninitialized-
pointer dereferences (the failure mode that the #413-#422 regression
class produces at runtime) and other UB that valgrind and compiler
warnings miss.

Highlights:

- Debug build, `-O1`, frame pointers preserved.
- `detect_leaks=0` - leak detection is left to the existing valgrind job
  in `ubuntu2404`, and C++ globals tend to false-positive there.
- UBSAN `halt_on_error=1`, ASAN `halt_on_error=0` - lets the run surface
  multiple findings per build.
- C++ tests only - Python sanitizer integration via skbuild is a separate
  follow-up if desired.

I have not added `continue-on-error: true` here; if you would prefer the
job to be informational while you assess any baseline findings, that is
a one-line change.
```
