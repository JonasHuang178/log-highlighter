## Context

Lazy rendering was introduced by `per-buffer-highlight-state` (2026-07-12) as a per-buffer version of an earlier global scheme: apply indicators only for the visible viewport plus a 1 000-line lookahead, then extend coverage from `SCN_UPDATEUI` as the user scrolls. The motivation was that each `SCI_INDICATORFILLRANGE` triggers a synchronous `SCN_MODIFIED(CHANGEINDICATOR)` round-trip to Notepad++ (~120 µs), so filling 64 k matches took seconds.

That root cause was solved differently. `ApplyHighlights` now masks `SC_MOD_CHANGEINDICATOR` out of the mod-event mask for the duration of the bulk fill (`log-highlighter.cpp:97-98`), which removes the per-fill notification cost outright. Once that landed, deferring work bought nothing, and `progress-dialog-two-phase` completed the shift by keeping the dialog visible through the apply phase.

The result is that the lazy path was never wired up:

| Spec requirement | Reality |
|---|---|
| `SCN_UPDATEUI` calls `ApplyHighlightsInRange` when `needed > appliedByteEnd` | `Plugin.cpp:259-262` only calls `g_overviewPanel.UpdateViewport()` |
| `GetLookaheadByteEnd(hSci)` computes viewport + 1 000 lines | Function does not exist |
| `appliedByteEnd` tracks the lazy cursor per buffer | Written twice in `ParseLog()`, only ever set to `-1`; never read |
| First parse applies viewport only | `ParseLog()` calls `ApplyHighlights(hSci, buf.matches)` — all matches |

## Goals / Non-Goals

**Goals:**
- `specs/` describes only behavior that exists in the code
- Dead code removed: `ApplyHighlightsInRange`, `appliedByteEnd`, the worker-thread `ParseDocument` overload
- `README.md` matches the code
- Zero user-visible behavior change

**Non-Goals:**
- Optimizing the apply phase further — the mod-event mask already handles the cost, and the progress dialog covers the remaining wait
- Restoring real-time re-highlight (deliberately removed; Ctrl+Alt+Q is the sole trigger)
- Adding true background-thread parsing — the worker-thread overload is being deleted, not made live

## Decisions

### Decision: Delete the capability rather than mark it deferred

The spec set is the source of truth for what the plugin does. A requirement with no implementation and no intent to implement is worse than absent — it makes every other spec suspect. The delta therefore removes `lazy-rendering` in full; the file is deleted from `specs/` when the change is archived.

**Alternative**: keep the spec with a "not implemented" note. Rejected — `specs/` is not a backlog.

### Decision: Delete `ApplyHighlightsInRange`, keep `ApplyHighlights`

`ApplyHighlights` is the only rendering entry point that is actually called. `ApplyHighlightsInRange` is the sole consumer of the sorted-by-`byteOffset` invariant via `std::lower_bound`, so `<algorithm>` comes out of `log-highlighter.cpp` with it.

The scanner spec's guarantee that matches are sorted by `byteOffset` **stays** — it is still true (`ScanBuffer` scans left-to-right) and the Overview Panel mark list depends on the ordering for top-to-bottom painting and for the equidistant-snap tie-break rule in `snap-to-nearest-mark`.

### Decision: Delete the worker-thread `ParseDocument` overload

No caller, and the design has moved away from it: the UI-thread overload already copies the document into a local `std::vector<char>` before scanning, which is what made the buffer-taking overload useful in the first place. `ScanBuffer` remains internally thread-agnostic (`static`, no Win32 calls), so reintroducing a worker thread later means re-exposing one small wrapper, not rebuilding the scanner.

### Decision: Drop `ApplyHighlights`'s `repaintAfter` parameter

The parameter was a two-caller switch: `true` from `ParseLog()` (force an immediate `RedrawWindow`), `false` from the `SCN_MODIFIED` handler (let Scintilla repaint itself after the text change that triggered the parse). With `SCN_MODIFIED` gone, only the `true` path is reachable, so the flag is a permanently-taken branch plus a decision the reader has to re-derive at every call site.

`RedrawWindow` becomes unconditional and the signature drops to two arguments. If a future caller ever needs to suppress the repaint, adding the flag back is trivial — but adding it speculatively is what left this here.

### Decision: Remove `appliedByteEnd` from `BufferState`, keep `matches` and `highlightActive`

`matches` is read on `NPPN_BUFFERACTIVATED` to rebuild panel marks; `highlightActive` gates that lookup. Only `appliedByteEnd` is dead.

### Decision: README documents observed behavior, not aspiration

Three sections are corrected:
- "Real-time re-highlight" — deleted; Ctrl+Alt+Q is the only trigger, and switching tabs restores the panel from cached per-buffer matches without re-parsing
- "Lazy rendering" — deleted; all highlights are applied on every parse
- "Large files (≥ 5 000 lines)" — the progress dialog appears on **every** parse, has two phases, and runs on the UI thread with `PeekMessage` pumping (no `std::thread`); Cancel works during parsing only

## Risks / Trade-offs

- [Risk] Very large files with many matches block the UI thread for the whole apply phase with no cancel. Accepted — this is already today's behavior; the change only stops documenting otherwise. The two-phase dialog keeps the user informed.
- [Trade-off] If lazy rendering is ever wanted again it must be rewritten from scratch. Acceptable: the deleted implementation was ~30 lines of `lower_bound` plus a loop, and the missing half (`GetLookaheadByteEnd`, the `SCN_UPDATEUI` wiring) was never written anyway.
