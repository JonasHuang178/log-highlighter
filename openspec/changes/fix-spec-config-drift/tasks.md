## 1. Config corrections

- [x] 1.1 Remove the `{ "Step123", MAKE_BGR(180, 230, 180), false }` row from `STEP_TYPE_RULES[]` in `log-highlighter/config/LogPatterns.h`, leaving `"Step"` as the only entry
- [x] 1.2 Remove the `OVERVIEW_BG_COLOR` definition and its comment block from `log-highlighter/config/OverviewConfig.h`
- [x] 1.3 Confirm no file under `log-highlighter/src/` references `OVERVIEW_BG_COLOR` or hard-codes a STEP rule count or indicator index (`grep -rn "OVERVIEW_BG_COLOR\|STEP_RULE_COUNT\|INDIC_BOOKMARK_BASE\|BOOKMARK_PATTERN_BASE" log-highlighter/src`)

## 2. Build verification

- [x] 2.1 Build Release x64 and confirm it compiles with no new warnings
- [x] 2.2 Verify the derived index bases shifted as expected: `INDIC_BOOKMARK_BASE` is 15 (was 16) and `BOOKMARK_PATTERN_BASE` shifted correspondingly, with no source change required
- [ ] 2.3 Run Ctrl+Alt+Q on a log containing `Step1 `, `Step1234 ` and `Start test`; confirm Step lines are highlighted exactly as before and bookmark keywords still render in magenta
- [ ] 2.4 Confirm the Overview Panel background is still `COLOR_BTNFACE` and visually continuous with the scrollbar
- [ ] 2.5 Run Ctrl+Alt+W and one custom report to confirm the shifted indicator bases broke nothing

## 3. Spec updates

- [ ] 3.1 Apply the `log-patterns-config` delta: add "Shipped rule table invariants", "No redundant STEP_TYPE prefix" and "LogPatterns.h structural inventory"
- [ ] 3.2 Apply the `log-patterns-config` MODIFIED requirements: "Rebuild-only customization" (constant list becomes an exact set, with scenarios) and "BookmarkRule struct" (absorbs the generalized case-sensitivity scenario)
- [ ] 3.3 Apply the `log-patterns-config` REMOVED requirements: delete "Default rules" and "Default bookmark rules", verifying no scenario is lost that was not re-attached in 3.2
- [ ] 3.4 Apply the `overview-panel` delta: add "Panel background matches the adjacent scrollbar"
- [ ] 3.5 Re-read `openspec/specs/log-patterns-config/spec.md` end to end and confirm no remaining text pins specific shipped keywords as normative

## 4. Documentation

- [x] 4.1 Remove the `Step123` row from the Step Type table in `README.md` (line ~30) and the redundancy blockquote below it (lines ~32-35)
- [x] 4.2 Remove the `OVERVIEW_BG_COLOR` paragraph from `README.md` (lines ~454-456)
- [x] 4.3 Confirm the README's Overview Panel appearance table still lists exactly the 7 constants that remain in `OverviewConfig.h`
- [x] 4.4 Search the README for any other reference to `Step123` or `OVERVIEW_BG_COLOR` and remove it

## 5. Consistency check

- [x] 5.1 Diff `config/OverviewConfig.h` against the constant inventory in the updated spec; confirm they match exactly with no extras on either side
- [x] 5.2 Diff `config/LogPatterns.h` against the structural inventory in the updated spec; confirm exactly `MAKE_BGR`, three structs and three tables
- [x] 5.3 Verify the shipped rule tables satisfy the invariants: all three non-empty, at least one rule with `showInPanel = true`, no STEP prefix equal to another prefix plus digits
- [x] 5.4 Confirm the deferred `static_assert` follow-up is recorded in `proposal.md` and not silently implemented in this change
