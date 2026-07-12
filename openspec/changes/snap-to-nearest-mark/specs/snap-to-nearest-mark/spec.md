## ADDED Requirements

### Requirement: Overview Panel click snaps to nearest mark
When the user clicks the Overview Panel and at least one mark exists in `m_marks`, the plugin SHALL compute the raw proportional line from the click Y position and then navigate to the mark whose `line` is closest to that raw line (minimum absolute difference). If two marks are equidistant, the first one in `m_marks` order (top-most) SHALL be selected.

If `m_marks` is empty, the plugin SHALL fall back to the existing proportional navigation (jump to raw line).

#### Scenario: Click near a single mark
- **WHEN** the user clicks the panel at a Y position whose raw line is 4980 and only one mark exists at line 5000
- **THEN** the editor navigates to line 5000

#### Scenario: Click between two marks — closer to upper
- **WHEN** the raw line is 4940 and marks exist at lines 4900 and 5000
- **THEN** the editor navigates to line 4900 (distance 40 < distance 60)

#### Scenario: Click between two marks — closer to lower
- **WHEN** the raw line is 4960 and marks exist at lines 4900 and 5000
- **THEN** the editor navigates to line 5000 (distance 40 < distance 60)

#### Scenario: No marks — proportional fallback
- **WHEN** `m_marks` is empty and the user clicks at 50% of the panel height on a 10,000-line file
- **THEN** the editor navigates to line 5000 (raw proportional)

#### Scenario: Dense cluster — click position guides selection
- **WHEN** marks exist at lines 4875, 4900, 4912, 5000, 5025, 5100 and the user clicks at a Y position whose raw line is 4892
- **THEN** the editor navigates to line 4900 (closest mark to 4892)
