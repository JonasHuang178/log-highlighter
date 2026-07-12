## MODIFIED Requirements

### Requirement: Single-click navigation with caret placement
A single click on the panel SHALL navigate the editor to the closest mark by line distance from the proportionally corresponding click position. If no marks exist, the target line is the proportionally corresponding line. The target line SHALL be centered in the editor viewport. The caret SHALL be placed at the start of the target line with no selection.

Navigation is deferred via `SetTimer(10ms)` to run after all click messages finish. `SCI_SETFIRSTVISIBLELINE` centers the view; `SCI_SETEMPTYSELECTION` places the caret without creating a selection.

#### Scenario: Click on a mark
- **WHEN** the user clicks on a colored mark in the panel
- **THEN** the editor scrolls so the corresponding mark line is centered, caret is at line start, no text is selected

#### Scenario: Click on empty panel area with marks present
- **WHEN** the user clicks on a panel area with no mark directly under the cursor but marks exist elsewhere
- **THEN** the editor navigates to the nearest mark line and centers it

#### Scenario: Click on empty panel area with no marks
- **WHEN** the user clicks the panel and no marks exist
- **THEN** the editor navigates to the proportionally corresponding line and centers it
