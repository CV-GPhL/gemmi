# Draft 4: CONTRIBUTING.md

## Goal

Add a CONTRIBUTING.md that documents the existing-but-undocumented
conventions for contributing to gemmi, plus a short PR-review checklist
focused on the failure mode behind the #413-#422 series (mass refactors
that quietly remove member initializers).

## Rationale and risk

This is the most controversial of the four drafts. CONTRIBUTING.md is
often viewed as bureaucracy. Two ways this can be done badly:

1. Over-prescribing process - turns contributors off. Avoid this by
   keeping the file short and concrete.
2. Codifying things the maintainer disagrees with - puts words in their
   mouth. Avoid this by sticking to things that are already true today
   (the build instructions match README, the test commands match
   `make check`, etc.) and by phrasing the review checklist as "the
   reviewer SHOULD" not "contributors MUST".

The draft below is short (under 100 lines) and the only really new
content is the review-checklist paragraph. Everything else is a pointer
to existing docs.

## File to add

Path: `CONTRIBUTING.md` at the repo root.

Content below.

---

```markdown
# Contributing to GEMMI

Thanks for your interest in contributing.

## Reporting bugs

File a [GitHub issue](https://github.com/project-gemmi/gemmi/issues).
For crashes or wrong output, include:

- a minimal input file (or a link to one)
- the exact gemmi command or Python snippet that reproduces it
- the gemmi version (`gemmi --version`) and how it was built (pip wheel,
  CMake from source, distro package)
- the expected vs actual output

## Building from source

See the [Installation
docs](https://gemmi.readthedocs.io/en/latest/install.html). In short:

```bash
git clone https://github.com/project-gemmi/gemmi.git
cd gemmi
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

For Python bindings:

```bash
pip install nanobind
cmake -B build -S . -DUSE_PYTHON=ON
cmake --build build -j
```

## Running tests

```bash
cmake --build build --target check -j
cd build && ctest --output-on-failure
python3 -m unittest discover -v -s tests/
```

CI runs the same commands plus a doctest pass over the Sphinx docs.

## Submitting changes

- Open a PR against `master`. One topic per PR; small, reviewable
  changes are preferred over large bundled ones.
- Include a test if the change is a bugfix or new feature - look at
  `tests/` for examples.
- Run `clang-format` only on lines you touched. Do not reformat
  unrelated code.
- For changes to public C++ headers (under `include/gemmi/`),
  double-check that you have not removed default member initializers
  unless you are also providing those initializers via a constructor.
  Several headers rely on aggregate default-construction and a missing
  `= nullptr` / `= 0` / `{}` can cause silent data corruption downstream.

## Reviewers' notes

The following are things to look out for when reviewing a PR, especially
a large or refactoring PR:

- Member-variable initializer changes in `include/gemmi/*.hpp`. Removing
  `= nullptr`, `= 0`, `{NAN, ...}` etc. from a struct member is almost
  always wrong even if the surrounding change is "just documentation".
  CI runs an informational `clang-tidy` job with
  `cppcoreguidelines-pro-type-member-init` - any new warning there is
  worth a closer look.
- Changes in the mmCIF read/write path (`include/gemmi/cif.hpp`,
  `to_cif.hpp`, `to_mmcif.hpp`) - the writer uses `number_or_qmark` to
  convert NaN to "?", so any uninitialized double that flows into it
  will produce garbage in the output CIF.
- Format-string and printf-family changes - gemmi uses both `std::format`
  and legacy printf in places.

## License

By contributing, you agree your contribution will be licensed under
MPLv2 and (at the user's option) LGPLv3, matching the project license.
```

---

## PR title

```
docs: add CONTRIBUTING.md with build/test pointers and reviewer notes
```

## PR body

```markdown
Follow-up to #429.

Adds a short CONTRIBUTING.md. Most of the content is pointers to
existing documentation (build, test, license) - the only genuinely new
content is the "Reviewers' notes" section, which calls out the
member-initializer-removal failure mode behind the #413-#422 series and
the mmCIF NaN-handling rule that makes it dangerous.

Kept short and concrete on purpose. If you would rather not have a
CONTRIBUTING.md at all, or want the reviewer-notes section moved into a
separate `docs/contributing.rst` page, happy to adjust - this is the
most opinionated of the four follow-ups from #429 and the easiest to
drop.
```
