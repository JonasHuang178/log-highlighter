## Why

The `lazy-rendering` capability is documented but not implemented. `ApplyHighlightsInRange` exists in `src/log-highlighter.cpp` with no call site anywhere in the project, and `GetLookaheadByteEnd` — which the spec requires — does not exist at all. The `SCN_UPDATEUI` handler in `Plugin.cpp` only refreshes the Overview Panel viewport box; it never extends indicator coverage. Since the two-phase progress dialog change, `ParseLog()` applies **all** highlights eagerly in Phase 2 and then resets `appliedByteEnd` to `-1`, so the lazy cursor is dead state.

Lazy rendering is no longer wanted. The progress dialog already gives the user feedback during the apply phase, which was the original reason for deferring work off the critical path. Keeping an unimplemented capability in `specs/` makes the spec set untrustworthy.

Two adjacent pieces of dead weight are removed in the same pass:

- The worker-thread `ParseDocument(const std::vector<char>&, int, progressFn)` overload has no caller. Parsing runs on the UI thread with `PeekMessage` pumping; no `std::thread` exists in the project.
- `README.md` describes three behaviors that are not true: real-time re-highlight on text change (removed by `per-buffer-highlight-state`), lazy rendering, and a 5 000-line threshold with background-thread parsing for the progress dialog.

## What Changes

- **Remove** the `lazy-rendering` capability entirely — spec, `ApplyHighlightsInRange` declaration and definition, and the `appliedByteEnd` field of `BufferState`
- **Remove** the worker-thread `ParseDocument` overload from `Parser.h` / `Parser.cpp`
- **Keep** `ApplyHighlights` (full apply) as the single rendering path — no behavior change for the user
- **Simplify** `ApplyHighlights` by dropping its `bool repaintAfter = true` parameter: the `false` branch existed only for the removed `SCN_MODIFIED` path, and every remaining call site uses the default
- **Update** `README.md` to describe what the code actually does

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `lazy-rendering`: capability removed in full
- `per-buffer-state`: `BufferState` drops `appliedByteEnd`
- `scanner`: one `ParseDocument` overload instead of two
- `status-bar-timing`: timing scope names `ApplyHighlights` (all matches) instead of `ApplyHighlightsInRange` (viewport)

## Impact

- `src/log-highlighter.h` / `src/log-highlighter.cpp` — delete `ApplyHighlightsInRange`; drop the `repaintAfter` parameter from `ApplyHighlights`; fix the stale `SCN_MODIFIED` comments on `InitStyles` / `ApplyHighlights`; drop the now-unused `<algorithm>` include
- `src/Parser.h` / `src/Parser.cpp` — delete the buffer overload
- `src/Plugin.cpp` — drop `appliedByteEnd` from `BufferState` and its two assignments
- `README.md` — rewrite the Usage / lazy-rendering / implementation-notes sections
- `openspec/specs/lazy-rendering/spec.md` — deleted at archive time
- No user-visible behavior change: highlights were already applied in full on every parse
