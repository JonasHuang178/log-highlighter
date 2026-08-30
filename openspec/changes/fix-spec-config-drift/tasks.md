## 1. Config corrections

- [x] 1.1 Remove the `{ "Step123", MAKE_BGR(180, 230, 180), false }` row from `STEP_TYPE_RULES[]` in `log-highlighter/config/LogPatterns.h`, leaving `"Step"` as the only entry
- [x] 1.2 Remove the `OVERVIEW_BG_COLOR` definition and its comment block from `log-highlighter/config/OverviewConfig.h`
- [x] 1.3 Confirm no file under `log-highlighter/src/` references `OVERVIEW_BG_COLOR` or hard-codes a STEP rule count or indicator index (`grep -rn "OVERVIEW_BG_COLOR\|STEP_RULE_COUNT\|INDIC_BOOKMARK_BASE\|BOOKMARK_PATTERN_BASE" log-highlighter/src`)

## 2. Build verification

2.3 to 2.5 required a running Notepad++ and a human judging colors and positions;
a successful compile does not stand in for them. Verified in-editor by the author
against a 19-line sample covering every case, including the negatives (`Step `,
`Stepname`, `Step1init`, lowercase `start test`) and the one line the change
actually affects.

`Step1234 ` on line 9 is that line: before the change it matched both the `Step`
and `Step123` rules and was filled twice over an identical range; after, once. It
renders identically to the other Step lines, which is the outcome the redundancy
argument predicted. Highlighting, the Overview Panel and both other commands all
behaved as specified.

- [x] 2.1 Build Release x64 and confirm it compiles with no new warnings
- [x] 2.2 Verify the derived index bases shifted as expected: `INDIC_BOOKMARK_BASE` is 15 (was 16) and `BOOKMARK_PATTERN_BASE` shifted correspondingly, with no source change required
- [x] 2.3 Run Ctrl+Alt+Q on a log containing `Step1 `, `Step1234 ` and `Start test`; confirm Step lines are highlighted exactly as before and bookmark keywords still render in magenta
- [x] 2.4 Confirm the Overview Panel background is still `COLOR_BTNFACE` and visually continuous with the scrollbar
- [x] 2.5 Run Ctrl+Alt+W and one custom report to confirm the shifted indicator bases broke nothing

## 3. Spec updates

The delta files under `specs/` are applied to `openspec/specs/` by
`openspec archive`, which is documented as "Archive a completed change and update
main specs" and is how every prior change in this repo synced (`adafa41`,
`0493b02`, `b8fd849` — each archives and syncs in one commit). Applying them by
hand would double-apply. Only the post-archive verification is a task here.

- [ ] 3.1 After archiving, re-read `openspec/specs/log-patterns-config/spec.md` end to end and confirm no remaining text pins specific shipped keywords as normative
- [ ] 3.2 After archiving, confirm `openspec/specs/overview-panel/spec.md` carries "Panel background matches the adjacent scrollbar"
- [ ] 3.3 After archiving, confirm "Default rules" and "Default bookmark rules" are gone and that the case-sensitivity scenario survives under "BookmarkRule struct"

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
