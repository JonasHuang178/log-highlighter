## Context

The Overview Panel click handler (`OnNCLButtonDblClk`) computes the target line as:
```
line = (clickY / panelH) * totalLines
```
This gives a raw proportional line regardless of where marks are. On large files the panel height is fixed (~800px) while `totalLines` can reach 10,000+, so each pixel spans ~12 lines. A 1px mark is nearly unclickable.

The fix is to snap the raw line to the nearest mark in `m_marks` (already stored in `OverviewPanel`), using `std::min_element` with a custom comparator.

## Goals / Non-Goals

**Goals:**
- Single click on Overview Panel snaps to the nearest mark within `OVERVIEW_SNAP_RADIUS` lines of the raw proportional click position
- Clicking in a dense cluster of marks respects click position (upper click → upper mark, lower click → lower mark)
- If no marks exist, or all marks are outside the snap radius, behavior falls back to proportional jump
- Snap radius is configurable via `OVERVIEW_SNAP_RADIUS` in `OverviewConfig.h` (unit: lines)

**Non-Goals:**
- Changing visual mark appearance or hit-area size
- Multi-mark cycling on repeated clicks in the same area
- Any change to `DoNavigation()` — snap logic lives in `OnNCLButtonDblClk`, navigation stays unchanged

## Decisions

### Decision: Snap in `OnNCLButtonDblClk`, not `DoNavigation`
Snap at the point where `m_pendingNavLine` is set, so `DoNavigation` stays a pure "jump to line N" helper with no mark awareness. Keeps concerns separated.

**Alternative considered**: Snap inside `DoNavigation` — rejected because `DoNavigation` has no access to `m_marks`, and adding it would couple navigation to mark state unnecessarily.

### Decision: `std::min_element` over sorted binary search
`m_marks` can have at most a few hundred entries in practice (only `showInPanel=true` rules). Linear scan is negligible. A binary search (`std::lower_bound`) is more complex for no measurable gain.

### Decision: Fallback to raw line when no marks or all outside radius
Preserves the original behavior for unparsed buffers, files with no panel-visible rules, and clicks far from any mark.

### Decision: Snap radius unit = lines, not pixels
Line distance is directly meaningful to the user ("within 50 lines of a mark"). Pixel distance on the panel varies with file size (1px = 12 lines on a 10k-line file), making a pixel-based constant unpredictable. A line-based constant `OVERVIEW_SNAP_RADIUS = 50` has consistent semantics regardless of file size.

### Decision: Place `OVERVIEW_SNAP_RADIUS` in `OverviewConfig.h`
Consistent with existing panel configuration constants (`OVERVIEW_PANEL_WIDTH`, `OVERVIEW_MARK_MIN_H`, etc.). Rebuild-to-configure pattern already established.

## Risks / Trade-offs

- [Risk] Dense cluster of marks at the same pixel position → `min_element` picks the first one encountered (stable, deterministic). User clicking the same pixel always lands on the same mark. → Acceptable; click position guides selection naturally.
- [Trade-off] With a finite radius, clicks far from all marks fall back to proportional jump and land on no mark. → Intended behavior; the radius lets the user choose how aggressively snap applies.
