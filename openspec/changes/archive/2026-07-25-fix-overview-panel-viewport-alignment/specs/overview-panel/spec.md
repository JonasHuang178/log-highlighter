## MODIFIED Requirements

### Requirement: Drawing area aligned with scrollbar track
The panel SHALL inset its effective drawing area (marks and viewport box) by `GetSystemMetrics(SM_CYVSCROLL)` pixels at the top and bottom, so the proportional mapping region aligns with the scrollbar track between the arrow buttons. The background fill SHALL still cover the full panel strip.

#### Scenario: Viewport box aligns with scrollbar track
- **WHEN** the scrollbar arrow buttons are 17px tall
- **THEN** the viewport box's topmost position is y=17 (below the up-arrow zone) and its bottommost position is y=panelH-17 (above the down-arrow zone)

#### Scenario: Marks drawn within track region only
- **WHEN** a mark exists at line 0 of the document
- **THEN** the mark is drawn at y=arrowH (top of the track-aligned region), not y=0

#### Scenario: Short window — inset does not exceed panel height
- **WHEN** the panel height is less than 2 × arrowH
- **THEN** the effective track height is clamped to at least 1px, and all marks and viewport box are drawn within that minimal region

### Requirement: Click navigation uses track-aligned coordinates
Single-click navigation SHALL subtract the top inset from the click y-position and use the track height (not the full panel height) for the proportional line calculation. Clicks in the top or bottom inset zones SHALL clamp to the first or last line respectively.

#### Scenario: Click at top of track region
- **WHEN** the user clicks at y = arrowH (top of track region) on a 10,000-line file
- **THEN** the editor navigates to line 0

#### Scenario: Click in the top arrow zone
- **WHEN** the user clicks at y = 5 (within the top inset zone)
- **THEN** the click is clamped to line 0

#### Scenario: Click at bottom of track region
- **WHEN** the user clicks at y = panelH - arrowH - 1 (bottom of track region) on a 10,000-line file
- **THEN** the editor navigates to approximately line 9,999
