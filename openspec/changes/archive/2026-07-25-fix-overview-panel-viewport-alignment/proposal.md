## Why

The Overview Panel's drawable area spans the full Scintilla window height — from the top edge of the window to the bottom edge. But the vertical scrollbar next to it has arrow buttons at the top and bottom (each ~17px, `SM_CYVSCROLL`), and its track (the draggable region) only occupies the space between those arrows.

This misalignment causes the viewport indicator box (green rectangle) to extend into the arrow-button zones:

- The top of the viewport box can sit above the scrollbar track, overlapping the up-arrow area
- The bottom of the viewport box can sit below the scrollbar track, overlapping the down-arrow area

The result is a visual mismatch: the panel's proportional mapping (line → y pixel) doesn't match the scrollbar track's proportional mapping, and the viewport box doesn't visually correspond to where the scrollbar thumb is.

## What Changes

- The panel's effective drawing area SHALL be inset by the scrollbar arrow-button height at the top and bottom, so marks and the viewport box are drawn only within the region that corresponds to the scrollbar track
- Click-to-navigate SHALL use the same inset region for its y → line conversion
- The background fill continues to cover the full panel strip (no gaps above/below the inset region)

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `overview-panel`: Panel drawing area, viewport box, marks, and click navigation are all constrained to align with the scrollbar track (between the arrow buttons)

## Impact

- `src/OverviewPanel.cpp` — `DrawPanel()`: compute inset from `GetSystemMetrics(SM_CYVSCROLL)`, offset all mark y-positions and viewport box by the top inset, reduce effective `panelH` by top + bottom inset
- `src/OverviewPanel.cpp` — `OnNCLButtonDblClk()`: subtract the top inset from click y before computing the proportional line
- No changes to `OverviewPanel.h`, `Plugin.cpp`, or `OverviewConfig.h`
- User-visible change: marks and viewport box now align with the scrollbar track
