## Context

`ParseLog()` in `Plugin.cpp` runs in two sequential phases:
1. **Parse** — `ParseDocument()` scans the document and calls a progress callback every 500 lines. The callback pumps messages so the dialog stays responsive. Fast (< 1s).
2. **Apply** — `ApplyHighlights()` calls `SCI_INDICATORFILLRANGE` once per match. No callback, no message pumping. Slow (several seconds on large files).

Currently `DestroyWindow(hDlg)` fires between Phase 1 and Phase 2, so the dialog disappears before the slow work begins. The user sees the dialog vanish, then waits with no feedback until highlights appear.

The `ProgressDialog` is a custom `WNDCLASS` popup with a `STATIC` text child (`IDC_TEXT`) and a Cancel button. There is no progress bar control currently.

## Goals / Non-Goals

**Goals:**
- Dialog remains visible through both Phase 1 (parsing) and Phase 2 (applying)
- During Phase 2 the label changes to `"Applying highlights…"` to indicate a different phase is running
- Cancel button is hidden during Phase 2 (apply cannot be interrupted)
- `DestroyWindow` is called only after `ApplyHighlights()` returns

**Non-Goals:**
- Per-match progress during Phase 2 (no callback in `SCI_INDICATORFILLRANGE`)
- Marquee progress bar (adds `comctl32` dependency complexity; text label is sufficient)
- Cancel support during Phase 2

## Decisions

### Decision: Add `SetProgressApplying(HWND)` to ProgressDialog API
A single new function sets the label to `"Applying highlights…"` and hides the Cancel button (`ShowWindow(hCancel, SW_HIDE)`). Keeps the API change minimal — one new header declaration, one new implementation function.

**Alternative**: Repurpose `SetProgressLine` with a special sentinel value — rejected, confusing API.

### Decision: Hide Cancel button during Phase 2, do not disable it
Hiding is cleaner visually. The button cannot be clicked when hidden, so no extra guard needed in the WM_COMMAND handler.

### Decision: Move `DestroyWindow` after `ApplyHighlights` in the non-cancelled path
Minimal change to `Plugin.cpp`. The cancelled path already destroys before returning, no change needed there.

## Risks / Trade-offs

- [Risk] Phase 2 blocks the UI thread (no PeekMessage pumping) — so the dialog label update won't repaint until the OS gets a chance. Mitigated by calling `UpdateWindow(hDlg)` after `SetProgressApplying()` to force an immediate repaint before `ApplyHighlights` starts.
- [Trade-off] No per-match progress bar during Phase 2. The user sees a static "Applying…" label for the duration. Acceptable — the label at least communicates that work is still happening, which is the core problem being solved.
