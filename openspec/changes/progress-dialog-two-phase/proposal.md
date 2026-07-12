## Why

The progress dialog disappears before rendering is complete. `DestroyWindow(hDlg)` is called immediately after `ParseDocument()` finishes, but `ApplyHighlights()` (the slow part — one `SCI_INDICATORFILLRANGE` per match) runs afterwards with no visible feedback. On large files with many matches the user sees the dialog vanish and then waits several seconds with no indication of what is happening before highlights appear.

## What Changes

- The progress dialog stays open through both phases of work: **Phase 1 – Parsing** and **Phase 2 – Applying highlights**
- During Phase 2 the dialog label changes to `"Applying highlights…"` with an indeterminate progress style (no line counter, since `SCI_INDICATORFILLRANGE` has no per-call callback)
- `DestroyWindow` is deferred until after `ApplyHighlights()` completes
- Cancel during Phase 1 (parsing) still works as before

## Capabilities

### New Capabilities

### Modified Capabilities
- `progress-dialog`: Requirement changes — dialog now covers both parse and apply phases; label updates to reflect current phase; `DestroyWindow` deferred to after apply completes

## Impact

- `src/Plugin.cpp` — `ParseLog()`: move `DestroyWindow` call after `ApplyHighlights()`; add `SetProgressApplying(hDlg)` call between the two phases
- `src/ProgressDialog.h` / `src/ProgressDialog.cpp` — add `SetProgressApplying()` function that changes label text and switches progress bar to marquee style
