## Context

The Overview Panel click handler (`OnNCLButtonDblClk`) computes the target line as:
```
line = (clickY / panelH) * totalLines
```
This gives a raw proportional line regardless of where marks are. On large files the panel height is fixed (~800px) while `totalLines` can reach 10,000+, so each pixel spans ~12 lines. A 1px mark is nearly unclickable.

The fix is to snap the raw line to the nearest mark in `m_marks` (already stored in `OverviewPanel`), using `std::min_element` with a custom comparator.

## Goals / Non-Goals

**Goals:**
- Single click on Overview Panel jumps to the closest mark by line distance from the raw proportional click position
- Clicking in a dense cluster of marks respects click position (upper click → upper mark, lower click → lower mark)
- If no marks exist, behavior is unchanged (proportional jump)

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

### Decision: Fallback to raw line when no marks
Preserves the original behavior for unparsed buffers or files with no panel-visible rules.

## Risks / Trade-offs

- [Risk] Dense cluster of marks at the same pixel position → `min_element` picks the first one encountered (stable, deterministic). User clicking the same pixel always lands on the same mark. → Acceptable; strategy 2 (click position guides selection) handles separation at the pixel level naturally.
- [Trade-off] Clicking far from any mark still snaps to the nearest mark, which may be far from the click. → Accepted by design; the user's goal in clicking the panel is always to reach a mark.
