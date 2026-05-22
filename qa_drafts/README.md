# QA infrastructure drafts for upstream Gemmi

Drafts staged for issue project-gemmi/gemmi#429. Each draft is a small,
self-contained, ready-to-apply change. Drafts are deliberately ordered by
likelihood-of-acceptance so we can submit them one by one and wait for
feedback between each.

When the maintainer responds on #429, pick the items they greenlight and
turn each draft into its own PR (one PR per draft, not bundled - per the
"small understandable PRs" preference stated on the issue).

## Drafts

| # | Title                                  | File                          | New job? | Risk |
|---|----------------------------------------|-------------------------------|----------|------|
| 1 | clang-tidy informational job           | 01_clang_tidy_ci.md           | yes      | low  |
| 2 | EXTRA_WARNINGS on remaining Linux/mac  | 02_extra_warnings_expansion.md| no       | low  |
| 3 | ASAN+UBSAN job                         | 03_asan_ubsan_job.md          | yes      | med  |
| 4 | CONTRIBUTING.md                        | 04_contributing_md.md         | no       | med  |

## Workflow

For each draft, when ready to PR:

1. Branch off `master` of CV-GPhL/gemmi fork: `git checkout master && git pull
   upstream master && git checkout -b qa/<short-name>`
2. Apply the snippets in the draft file (the draft file shows the exact
   chunk to paste / lines to edit).
3. Test locally where applicable (drafts 2 + 3 have a local test command).
4. Commit, push to fork, open PR against project-gemmi/gemmi:master with
   the body from the draft.
5. Wait for review before starting the next one.
