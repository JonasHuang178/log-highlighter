## Context

The Overview Panel occupies a 14px strip in Scintilla's non-client area, immediately to the right of the vertical scrollbar. The panel and the scrollbar share the same vertical extent (the full Scintilla window height), but they use that space differently:

```
 Scrollbar          Panel (current)        Panel (fixed)
┌──────────┐       ┌──────────┐           ┌──────────┐
│  ▲ arrow │       │ marks +  │           │ background│  ← arrow zone (inset top)
├──────────┤       │ viewport │           ├──────────┤
│          │       │ box span │           │ marks +  │
│  track   │       │ the full │           │ viewport │  ← track-aligned zone
│          │       │ height   │           │ box here │
├──────────┤       │          │           ├──────────┤
│  ▼ arrow │       │          │           │ background│  ← arrow zone (inset bottom)
└──────────┘       └──────────┘           └──────────┘
```

The arrow-button height is `GetSystemMetrics(SM_CYVSCROLL)`, typically 17px on Windows 10/11 at 100% DPI. This is the same metric Scintilla uses to size its scrollbar arrows.

Currently `DrawPanel()` receives `panelH` = full strip height, and all y-coordinate math (marks, viewport box) maps `[0, panelH)` to `[line 0, totalLines)`. The fix introduces a top and bottom inset equal to the arrow-button height, so the effective drawing area maps `[insetTop, panelH - insetBot)` to `[line 0, totalLines)`.

## Goals / Non-Goals

**Goals:**
- Marks and viewport box are vertically aligned with the scrollbar track
- Click navigation uses the same inset region for y → line conversion
- Panel background still covers the full strip (no visual gaps)
- DPI-aware: uses `GetSystemMetrics(SM_CYVSCROLL)` which respects the system DPI setting

**Non-Goals:**
- Pixel-perfect alignment with the scrollbar thumb — the panel and scrollbar have different proportional models (panel maps to document lines; scrollbar maps to scroll range), so approximate alignment is the goal
- Changing the panel width or scrollbar appearance
- Reading the actual scrollbar layout — there is no public API to query the exact arrow rect; `SM_CYVSCROLL` is the standard approximation

## Decisions

### Decision: Inset in `DrawPanel()`, not in `GetPanelRectScreen()`

`GetPanelRectScreen()` returns the full NCA strip rect and is used by `OnNCHitTest`, `OnNCPaint`, and `OnNCLButtonDblClk`. Shrinking it would break hit-testing (clicks on the inset area would fall through to Scintilla) and leave unpainted gaps. Instead, the background fill covers the full rect, and only the mark/viewport drawing uses the inset sub-region.

### Decision: Use `GetSystemMetrics(SM_CYVSCROLL)` for arrow-button height

This is the standard Win32 metric for vertical scrollbar arrow-button height. It matches what Scintilla uses for its own scrollbar layout. It scales with system DPI settings, so the inset stays correct at 125%, 150%, etc.

**Alternative**: Hard-code 17px. Rejected — breaks on high-DPI displays.

### Decision: Apply the same inset to `OnNCLButtonDblClk`

The click handler computes `rawLine = (clickY / panelH) * totalLines`. With the inset, it must subtract the top inset from `clickY` and use the effective height instead of the full height. Clicks in the inset zones (above/below the track-aligned region) are clamped to the first/last line.

## Risks / Trade-offs

- [Risk] On systems with non-standard scrollbar themes (e.g. flat scrollbars with no visible arrows), the inset may not match. Accepted — `SM_CYVSCROLL` is the best available approximation, and the mismatch is cosmetic only.
- [Trade-off] The effective drawing height is reduced by ~34px (2 × 17px). On very short windows this reduces the space for marks. Accepted — the alignment benefit outweighs the minor height loss.
