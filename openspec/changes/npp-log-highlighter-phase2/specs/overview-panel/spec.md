## ADDED Requirements

### Requirement: Docked panel on the right side
The plugin SHALL register a docked panel on the right side of Notepad++ using
`NPPM_DMMREGASDCKDLG` with `DWS_DF_CONT_RIGHT`. Registration SHALL occur in
response to the `NPPN_READY` notification.

#### Scenario: Panel appears on first launch
- **WHEN** Notepad++ starts and loads the plugin
- **THEN** a narrow panel appears docked on the right side of the Notepad++ window

#### Scenario: Panel survives resize
- **WHEN** the user resizes the Notepad++ window
- **THEN** the panel height adjusts and marks are redrawn at correct proportional positions

### Requirement: Proportional colored marks
The panel SHALL paint one colored mark per match where `showInPanel = true`.
Mark vertical position SHALL be proportional to the match line number within
the total document line count. Mark height SHALL be `max(OVERVIEW_MARK_MIN_H, panelH / totalLines)`.
Consecutive marks of the same color within 2px SHALL be merged into a single taller mark.

#### Scenario: Single ERROR in document
- **WHEN** the document has 1000 lines and one `[ ERROR ]` at line 500
- **THEN** one red mark appears at the vertical midpoint of the panel

#### Scenario: Dense ERROR cluster
- **WHEN** multiple `[ ERROR ]` matches fall within 2px of each other on the panel
- **THEN** they are merged into one continuous red mark

#### Scenario: showInPanel false rule excluded
- **WHEN** a `[ DEBUG ]` match exists and its `showInPanel` is `false`
- **THEN** no mark appears in the panel for that match

### Requirement: Viewport indicator box
The panel SHALL draw a semi-transparent rectangle indicating the currently
visible region of the document. Position and height SHALL be computed from
`SCI_GETFIRSTVISIBLELINE` and `SCI_LINESONSCREEN`. The box SHALL update on
every `SCN_UPDATEUI` notification.

#### Scenario: Viewport box tracks scrolling
- **WHEN** the user scrolls the editor
- **THEN** the viewport box moves proportionally within the panel

#### Scenario: Viewport box at document start
- **WHEN** the editor is scrolled to the top of the document
- **THEN** the viewport box appears at the top of the panel

### Requirement: Double-click navigation
A double-click on the panel SHALL navigate the editor to the line corresponding
to the clicked vertical position. The target line SHALL be centered in the
editor viewport using `SCI_GOTOLINE` followed by `SCI_SCROLLCARET`.

#### Scenario: Double-click on a mark
- **WHEN** the user double-clicks on a colored mark in the panel
- **THEN** the editor scrolls so that the corresponding line is visible and centered

#### Scenario: Double-click on empty panel area
- **WHEN** the user double-clicks on a panel area with no mark
- **THEN** the editor navigates to the proportionally corresponding line and centers it

### Requirement: Panel updates with g_matches
The panel SHALL refresh its marks whenever `g_matches` is updated, without
performing any additional document scanning.

#### Scenario: Refresh after Ctrl+Alt+Q
- **WHEN** the user presses Ctrl+Alt+Q
- **THEN** the panel redraws with marks reflecting the latest parse results

#### Scenario: Refresh after real-time edit
- **WHEN** the user types or pastes a keyword and real-time highlighting is active
- **THEN** the panel mark for the new keyword appears immediately
