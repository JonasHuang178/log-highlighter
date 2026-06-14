## ADDED Requirements

### Requirement: NCA panel on the right side of Scintilla
The plugin SHALL steal 14px from the right edge of the Scintilla HWND via `WM_NCCALCSIZE` using `SetWindowSubclass`. The panel SHALL be painted via `WM_NCPAINT` using `GetWindowDC` and a double-buffered off-screen DC. The subclass SHALL be installed on the first `ParseLog()` call (lazy init) and removed on `WM_NCDESTROY`.

#### Scenario: Panel appears after first Ctrl+Alt+Q
- **WHEN** the user presses Ctrl+Alt+Q for the first time
- **THEN** a 14px panel appears on the right edge of the Scintilla window with colored marks

#### Scenario: Panel survives editor resize
- **WHEN** the user resizes the Notepad++ window
- **THEN** the panel redraws at correct proportional mark positions

### Requirement: Proportional colored marks with adaptive height
The panel SHALL paint one colored mark per match where `showInPanel = true`. Mark color SHALL come from the rule's `textColor` (LOG_TYPE) or `bgColor` (STEP_TYPE). Mark height SHALL be adaptive:
```
markH_base = max(1, panelH / totalLines)
adaptive   = panelH * 0.5 / numMarks
markH      = max(1, min(markH_base, adaptive))
```
Marks that truly overlap (next mark's `y < current mergeBot`) SHALL be merged. Total mark coverage SHALL not exceed 50% of panel height when marks are dense.

#### Scenario: Single ERROR in document
- **WHEN** the document has 1000 lines and one `[ ERROR ]` at line 500
- **THEN** one red mark appears at the vertical midpoint of the panel

#### Scenario: Dense ERROR cluster
- **WHEN** many `[ ERROR ]` matches exist across a large document
- **THEN** marks are scaled down so the panel does not turn solid red; individual mark positions remain distinguishable

#### Scenario: showInPanel false rule excluded
- **WHEN** a `[ DEBUG ]` match exists and its `showInPanel` is `false`
- **THEN** no mark appears in the panel for that match

### Requirement: Viewport indicator box
The panel SHALL draw a filled rectangle with a border indicating the currently visible region. Fill color: `OVERVIEW_VIEWPORT_BG_COLOR` (`CLR_NONE` = `GetSysColor(COLOR_SCROLLBAR)`). Border color: `OVERVIEW_VIEWPORT_COLOR`. The box SHALL update on every `SCN_UPDATEUI` notification.

#### Scenario: Viewport box tracks scrolling
- **WHEN** the user scrolls the editor
- **THEN** the viewport box moves proportionally within the panel

### Requirement: Single-click navigation with caret placement
A single click on the panel SHALL navigate the editor to the proportionally corresponding line. The target line SHALL be centered in the editor viewport. The caret SHALL be placed at the start of the target line with no selection.

Navigation is deferred via `SetTimer(10ms)` to run after all click messages finish. `SCI_SETFIRSTVISIBLELINE` centers the view; `SCI_SETEMPTYSELECTION` places the caret without creating a selection.

#### Scenario: Click on a mark
- **WHEN** the user clicks on a colored mark in the panel
- **THEN** the editor scrolls so the corresponding line is centered, caret is at line start, no text is selected

#### Scenario: Click on empty panel area
- **WHEN** the user clicks on a panel area with no mark
- **THEN** the editor navigates to the proportionally corresponding line and centers it

### Requirement: Panel updates with g_matches
The panel SHALL refresh its marks whenever `g_matches` is updated, without performing any additional document scanning.

#### Scenario: Refresh after Ctrl+Alt+Q
- **WHEN** the user presses Ctrl+Alt+Q
- **THEN** the panel redraws with marks reflecting the latest parse results

#### Scenario: Refresh after real-time edit
- **WHEN** the user types or pastes a keyword and real-time highlighting is active
- **THEN** the panel mark for the new keyword appears immediately
