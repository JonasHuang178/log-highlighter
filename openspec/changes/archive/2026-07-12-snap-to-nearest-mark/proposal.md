## Why

When a log file has many lines (e.g. 10,000+), marks in the Overview Panel shrink to 1px or merge together, making it nearly impossible to click precisely on a specific mark. The current click handler jumps to the proportionally corresponding line, ignoring where marks actually are.

## What Changes

- Overview Panel click now snaps to the nearest mark (by line distance from the raw proportional click position) instead of jumping to the raw proportional line
- If no marks exist, click behavior is unchanged (proportional jump)

## Capabilities

### New Capabilities
- `snap-to-nearest-mark`: Click on the Overview Panel snaps to the closest mark (by `|mark.line - rawLine|`), enabling precise navigation even when marks are 1px tall or clustered

### Modified Capabilities
- `overview-panel`: Click navigation requirement changes — target line is now the nearest mark line, not the raw proportional line

## Impact

- `src/OverviewPanel.cpp`: `OnNCLButtonDblClk` — change line computation from raw proportion to nearest-mark snap
- No API or data structure changes; `m_marks` already holds the mark list needed for the search
