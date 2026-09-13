## 1. Repair the delta headers

- [x] 1.1 In `openspec/specs/overview-panel/spec.md`, replace the line-1 `## ADDED Requirements` header with `## Requirements`
- [x] 1.2 Do the same in `openspec/specs/progress-dialog/spec.md`
- [x] 1.3 Do the same in `openspec/specs/snap-to-nearest-mark/spec.md`
- [x] 1.4 Sweep all of `openspec/specs/` for any remaining delta header (`grep -rn "^## \(ADDED\|MODIFIED\|REMOVED\|RENAMED\)" openspec/specs`) and confirm there are none
- [x] 1.5 Confirm by diff that only line 1 changed in each of the three files and no line inside any `### Requirement:` block was touched

## 2. Add Purpose sections

- [x] 2.1 Add a `## Purpose` section above `## Requirements` in all 11 specs under `openspec/specs/`, each one or two sentences saying what the capability covers and why it is separate
- [x] 2.2 Confirm no Purpose restates mechanism, constants or keyword lists — that content belongs in the requirements or the README
- [x] 2.3 Confirm by diff that every edit is purely additive: no existing line removed or reworded in any of the 11 files

## 3. Validate the repair

- [x] 3.1 Run `openspec validate --specs` and confirm 8 passed, 3 failed — and that the 3 failures are exactly `log-patterns-config`, `scanner` and `status-bar-timing`, failing only on missing scenarios and missing SHALL/MUST (the content defects deferred by Decision 6). Any other failure, or any other spec failing, means this change broke something
- [x] 3.2 Confirm the nine previously hidden `overview-panel` requirements are now visible to the tooling
- [x] 3.3 Confirm no file under `log-highlighter/` was modified by this change

## 4. Archive and verify the generated capability spec

- [x] 4.1 Run `openspec archive fix-openspec-spec-headers` without `--skip-specs`, so the new `spec-document-structure` capability is written into `openspec/specs/`
- [x] 4.2 Run `openspec validate --specs` immediately afterwards — this is the loop the new capability requires, and design Decision 5 predicts it may fail here
- [x] 4.3 If archive wrote `openspec/specs/spec-document-structure/spec.md` with a line-1 delta header or without a `## Purpose`, repair it the same way as tasks 1 and 2, then re-validate. If archive normalized it, record that the prediction was wrong and do nothing
- [x] 4.4 Confirm `openspec validate --specs` reports 9 passed, 3 failed — the 8 from task 3.1 plus the newly created `spec-document-structure`, with the same 3 content-defect failures still outstanding

## 5. Hand off the remainder

- [x] 5.1 Confirm `fix-spec-config-drift` is still blocked and record why: `log-patterns-config` is one of its target specs and remains invalid, so `openspec archive` will keep refusing until the follow-up change lands

  **Resolved 2026-09-13, and the prediction was half right.** The archive did
  refuse, exactly as described, and wrote nothing. But it refused over a single
  requirement — `MAKE_BGR macro`, one of the three scenario-less requirements
  this change deferred — so the blockage was narrower than "wait for the whole
  follow-up". Adding one scenario to that requirement cleared it, and
  `fix-spec-config-drift` archived on its own, ahead of
  `fix-spec-requirement-scenarios`. The deferred list shrank from three specs to
  two as a side effect.

- [x] 5.2 Propose `fix-spec-requirement-scenarios` covering the 7 requirements listed in this change's Deferred section, so the work is captured rather than left in a conversation

  Proposed and archived 2026-09-13 as
  `archive/2026-09-13-fix-spec-requirement-scenarios`, covering the six
  requirements that remained after 5.1 cleared `MAKE_BGR macro`.

- [x] 5.3 Do not attempt to archive `fix-spec-config-drift` with `--skip-specs` as a shortcut — that would silently discard its deltas, which are the entire point of that change

  Honoured. The archive ran without `--skip-specs` and applied both deltas
  (`log-patterns-config` +3/~2/-2, `overview-panel` +1).
