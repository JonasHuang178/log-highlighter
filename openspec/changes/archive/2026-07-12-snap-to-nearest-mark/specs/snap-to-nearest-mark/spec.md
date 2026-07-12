## ADDED Requirements

### Requirement: Overview Panel click snaps to nearest mark within radius
When the user clicks the Overview Panel, the plugin SHALL compute the raw proportional line from the click Y position and then find the mark in `m_marks` whose `line` is closest to that raw line (minimum absolute difference). If that closest mark is within `OVERVIEW_SNAP_RADIUS` lines of the raw line, the plugin SHALL navigate to that mark's line. If the closest mark is farther than `OVERVIEW_SNAP_RADIUS` lines, or if `m_marks` is empty, the plugin SHALL fall back to proportional navigation (jump to raw line).

If two marks are equidistant and both within radius, the first one in `m_marks` order (top-most) SHALL be selected.

`OVERVIEW_SNAP_RADIUS` is defined in `config/OverviewConfig.h` (unit: document lines). Default value: 50.

#### Scenario: Click within radius of a mark
- **WHEN** `OVERVIEW_SNAP_RADIUS = 50`, raw line is 4980, and a mark exists at line 5000 (distance 20)
- **THEN** the editor navigates to line 5000

#### Scenario: Click outside radius of all marks — proportional fallback
- **WHEN** `OVERVIEW_SNAP_RADIUS = 50`, raw line is 2000, and the nearest mark is at line 5000 (distance 3000)
- **THEN** the editor navigates to line 2000 (raw proportional)

#### Scenario: Click between two marks — both in radius, closer to upper
- **WHEN** `OVERVIEW_SNAP_RADIUS = 50`, raw line is 4940, and marks exist at lines 4900 and 5000
- **THEN** the editor navigates to line 4900 (distance 40 < distance 60)

#### Scenario: Click between two marks — both in radius, closer to lower
- **WHEN** `OVERVIEW_SNAP_RADIUS = 50`, raw line is 4960, and marks exist at lines 4900 and 5000
- **THEN** the editor navigates to line 5000 (distance 40 < distance 60)

#### Scenario: No marks — proportional fallback
- **WHEN** `m_marks` is empty and the user clicks at 50% of the panel height on a 10,000-line file
- **THEN** the editor navigates to line 5000 (raw proportional)

#### Scenario: Dense cluster — click position guides selection within radius
- **WHEN** `OVERVIEW_SNAP_RADIUS = 50`, marks exist at lines 4875, 4900, 4912, 5000, 5025, 5100, and raw line is 4892
- **THEN** the editor navigates to line 4900 (closest mark to 4892, within radius)

### Requirement: Snap radius configurable in OverviewConfig.h
`OVERVIEW_SNAP_RADIUS` SHALL be defined as a compile-time constant in `config/OverviewConfig.h`. The unit SHALL be document lines. The default value SHALL be 50.

#### Scenario: Radius set to 0 disables snap
- **WHEN** `OVERVIEW_SNAP_RADIUS = 0` and the user clicks anywhere on the panel
- **THEN** all clicks fall back to proportional navigation (no snap occurs)
