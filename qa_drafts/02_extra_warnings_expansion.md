# Draft 2: enable EXTRA_WARNINGS on remaining Linux/Mac CI jobs

## Goal

Turn on `EXTRA_WARNINGS=ON` for the four Linux/macOS CI jobs that currently
don't set it: `macos13`, `ubuntu2404arm`, `ubuntu2404`, `almalinux`.
Windows (MSVC) is intentionally left alone - the flag adds GCC/Clang-only
flags via the existing `if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU" AND
EXTRA_WARNINGS)` guard in CMakeLists.txt, so it's a no-op on MSVC anyway,
but adding it there would be misleading.

## Rationale

- These jobs already build the same code as the EXTRA_WARNINGS jobs - any
  warning surfaced is real on the same source.
- Existing EXTRA_WARNINGS jobs (macos15, ubuntu2204, ubuntu2204_clang8)
  build clean today, so expansion should not introduce new failures.
- Costs nothing - just an extra cmake flag per job.

## Files changed

- `.github/workflows/ci.yml` - 4 small edits

## Edits

All four edits add `-DEXTRA_WARNINGS=ON` (or `=1`) to the existing cmake
invocation in each job. The exact lines and the after-state:

### Edit 1 - macos13 (line ~52)

**Before:**
```yaml
        cmake $GITHUB_WORKSPACE -DCMAKE_BUILD_TYPE=MinSizeRel -DBUILD_SHARED_LIBS=OFF -DCMAKE_CXX_STANDARD=14
```

**After:**
```yaml
        cmake $GITHUB_WORKSPACE -DCMAKE_BUILD_TYPE=MinSizeRel -DBUILD_SHARED_LIBS=OFF -DCMAKE_CXX_STANDARD=14 -DEXTRA_WARNINGS=ON
```

### Edit 2 - ubuntu2404arm (line ~109)

**Before:**
```yaml
        export SKBUILD_CMAKE_ARGS='-DFETCH_ZLIB_NG=ON;-DCMAKE_BUILD_TYPE=None'
```

**After:**
```yaml
        export SKBUILD_CMAKE_ARGS='-DFETCH_ZLIB_NG=ON;-DCMAKE_BUILD_TYPE=None;-DEXTRA_WARNINGS=ON'
```

### Edit 3 - ubuntu2404 (line ~207)

**Before:**
```yaml
        cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo -DUSE_PYTHON=1 -DPython_EXECUTABLE=python3 -DBUILD_SHARED_LIBS=ON -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-O2 -g1 -DNDEBUG"
```

**After:**
```yaml
        cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo -DUSE_PYTHON=1 -DPython_EXECUTABLE=python3 -DBUILD_SHARED_LIBS=ON -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-O2 -g1 -DNDEBUG" -DEXTRA_WARNINGS=ON
```

### Edit 4 - almalinux (line ~262)

**Before:**
```yaml
        cmake . -DUSE_PYTHON=1 -DPython_EXECUTABLE=/usr/bin/python3.9 -DFETCH_ZLIB_NG=ON
```

**After:**
```yaml
        cmake . -DUSE_PYTHON=1 -DPython_EXECUTABLE=/usr/bin/python3.9 -DFETCH_ZLIB_NG=ON -DEXTRA_WARNINGS=ON
```

## Local test before opening PR

A clean build with EXTRA_WARNINGS on a recent GCC and a recent Clang:

```bash
cd /path/to/gemmi
rm -rf build && cmake -B build -S . -DCMAKE_BUILD_TYPE=RelWithDebInfo -DEXTRA_WARNINGS=ON
cmake --build build -j 2>&1 | tee /tmp/build.log
grep -E 'warning:|error:' /tmp/build.log | head -50
```

The existing CI jobs already build clean with these flags so this should
be a no-op locally. If something does surface, it is most likely a
compiler-version difference; either pin the build to a known-clean
compiler or send a follow-up fix.

## PR title

```
ci: enable EXTRA_WARNINGS on remaining Linux and macOS jobs
```

## PR body

```markdown
Follow-up to #429.

Enables `EXTRA_WARNINGS=ON` on the four CI jobs that don't currently set
it (`macos13`, `ubuntu2404arm`, `ubuntu2404`, `almalinux`). The flag is
already used on `macos15`, `ubuntu2204`, and `ubuntu2204_clang8`, and the
underlying CMakeLists.txt block is guarded on
`CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU"` so this only affects
GCC/Clang builds. Windows (MSVC) is intentionally not changed.

These jobs already compile the same source as the EXTRA_WARNINGS-enabled
jobs, so this expansion should not introduce new failures. The benefit is
better cross-toolchain coverage of the existing warning set.

No CMakeLists.txt change. No new warning flags added in this PR - keeping
the existing flag list (`-Wall -Wextra -Wpedantic -Wformat=2
-Wredundant-decls -Wfloat-conversion -Wdisabled-optimization -Wshadow`)
as-is, since adding flags is a separate decision.
```
