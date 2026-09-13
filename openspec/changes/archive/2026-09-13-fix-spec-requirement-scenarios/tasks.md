## 1. Scanner spec

- [x] 1.1 Rewrite `Match struct` in the normative mood, correct the field table to list `LOG_TYPE`, `STEP_TYPE` and `BOOKMARK`, and record `BookmarkLinesFrom` as the consumer of the sort-order guarantee
- [x] 1.2 Give `Match struct` scenarios covering keyword-length, step-to-end-of-line, bookmark distinguishability and result ordering
- [x] 1.3 Rewrite `Progress callback` in the normative mood and state that a cancelled scan discards already-collected matches
- [x] 1.4 Give `Progress callback` scenarios covering the tick interval, cancellation, and the no-callback case
- [x] 1.5 Confirm every scenario traces to a claim the pre-existing requirement text already made, or is listed as an addition in `proposal.md`

## 2. Status bar timing spec

- [x] 2.1 Add a scenario to `Parse time displayed in Notepad++ status bar`
- [x] 2.2 Convert `Timing scope` to the normative mood and state that the interval SHALL NOT stop at the end of the scan
- [x] 2.3 Convert `Adaptive time format` to the normative mood, preserving the format table and all three existing scenarios unchanged
- [x] 2.4 Add a scenario to `Timing source` covering a mid-parse system clock adjustment
- [x] 2.5 Confirm no existing scenario was reworded or dropped

## 3. Code comment sync

- [x] 3.1 Correct the `Match` comment block in `log-highlighter/src/Parser.h` to list `BOOKMARK` alongside `LOG_TYPE` and `STEP_TYPE`
- [x] 3.2 Confirm no executable line changed and the plugin still builds Release x64

## 4. Validate and archive

- [x] 4.1 Run `openspec validate fix-spec-requirement-scenarios --strict` and confirm the change is valid
- [x] 4.2 Run `openspec archive fix-spec-requirement-scenarios`; a refusal here means a target requirement was missed — repair the delta rather than passing `--no-validate`
- [x] 4.3 Run `openspec validate --specs` immediately afterwards, as `spec-document-structure` requires. On this branch the result is 11 passed / 1 failed: `scanner` and `status-bar-timing` now pass, and the single remaining failure is `log-patterns-config`, which this change does not touch. This branch is cut from `main`, which does not yet carry the `MAKE_BGR macro` scenario — that repair is in `fix-spec-config-drift`, archived on the `chore/archive-fix-spec-config-drift` branch and not yet merged. 12 passed / 0 failed is reached when both land on `main`, and is to be confirmed there rather than claimed here
- [x] 4.4 Sweep `openspec/specs/` for delta headers leaked by the archive run (`grep -rn "^## \(ADDED\|MODIFIED\|REMOVED\|RENAMED\) Requirements" openspec/specs`) and confirm there are none

## 5. Close out the deferred work

- [x] 5.1 In `openspec/changes/archive/2026-08-30-fix-openspec-spec-headers/tasks.md`, check off 5.2 — this change is the `fix-spec-requirement-scenarios` it asked for
- [x] 5.2 Check off 5.1 and 5.3 of that change, recording that `fix-spec-config-drift` is no longer blocked: it archived on 2026-09-13, and not via the `--skip-specs` shortcut 5.3 warned against
