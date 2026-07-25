## ADDED Requirements

### Requirement: NCA panel on the right side of Scintilla
The plugin SHALL steal 14px from the right edge of the Scintilla HWND via `WM_NCCALCSIZE` using `SetWindowSubclass`. The panel SHALL be painted via `WM_NCPAINT` using `GetWindowDC` and a double-buffered off-screen DC. The subclass SHALL be installed on the first `ParseLog()` call (lazy init) and removed on `WM_NCDESTROY`.

#### Scenario: Panel appears after first Ctrl+Alt+Q
- **WHEN** the user presses Ctrl+Alt+Q for the first time
- **THEN** a 14px panel appears on the right edge of the Scintilla window with colored marks

#### Scenario: Panel survives editor resize
- **WHEN** the user resizes the Notepad++ window
- **THEN** the panel redraws at correct proportional mark positions

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
A single click on the panel SHALL navigate the editor to the closest mark by line distance from the proportionally corresponding click position. If no marks exist, or the nearest mark is farther than `OVERVIEW_SNAP_RADIUS` lines away, the target line is the proportionally corresponding line. The target line SHALL be centered in the editor viewport. The caret SHALL be placed at the start of the target line with no selection.

Navigation is deferred via `SetTimer(10ms)` to run after all click messages finish. `SCI_SETFIRSTVISIBLELINE` centers the view; `SCI_SETEMPTYSELECTION` places the caret without creating a selection.

#### Scenario: Click on a mark
- **WHEN** the user clicks on a colored mark in the panel
- **THEN** the editor scrolls so the corresponding mark line is centered, caret is at line start, no text is selected

#### Scenario: Click on empty panel area with marks present
- **WHEN** the user clicks on a panel area with no mark directly under the cursor but marks exist within `OVERVIEW_SNAP_RADIUS` lines
- **THEN** the editor navigates to the nearest mark line and centers it

#### Scenario: Click on empty panel area with no marks
- **WHEN** the user clicks the panel and no marks exist
- **THEN** the editor navigates to the proportionally corresponding line and centers it

### Requirement: Panel follows active Scintilla view
When `Update()` is called with a Scintilla HWND different from the currently subclassed one (the user switched views), the panel SHALL migrate: remove the subclass from the old Scintilla, restore its full client area via `SWP_FRAMECHANGED`, install the subclass on the new Scintilla, and trigger `SWP_FRAMECHANGED` on it. The panel is a single resource that follows the active view.

#### Scenario: Switch from main view to second view
- **WHEN** the panel is subclassed on scintillaMainHandle and `Update()` is called with scintillaSecondHandle
- **THEN** the subclass is removed from scintillaMainHandle (its client area is restored to full width) and installed on scintillaSecondHandle (which gains the 14px panel strip)

#### Scenario: Single-view tab switch — no migration
- **WHEN** all tabs are in the same view and the user switches tabs
- **THEN** the Scintilla HWND is unchanged, no subclass migration occurs, and the panel redraws with the new buffer's marks (or clears if unparsed)

### Requirement: Synchronous repaint on Update
`Update()` SHALL use `RedrawWindow` with `RDW_UPDATENOW` (in addition to `RDW_FRAME | RDW_INVALIDATE`) to force synchronous repaint. This ensures the panel reflects the active buffer's marks immediately on tab switch, without waiting for the next deferred paint cycle.

`UpdateViewport()` (called on `SCN_UPDATEUI` during scroll) SHALL continue using asynchronous `InvalidateFrame()` to avoid unnecessary synchronous overhead on every scroll tick.

#### Scenario: Panel clears immediately on tab switch to unparsed buffer
- **WHEN** file A has been parsed and the user switches to unparsed file B
- **THEN** the panel clears (no marks) within the same message-processing pass — the user never sees A's marks on B's tab

#### Scenario: Panel restores immediately on tab switch to parsed buffer
- **WHEN** files A and B are both parsed, the user is on A, and switches to B
- **THEN** the panel immediately shows B's marks — no flicker or stale A marks visible

### Requirement: Panel updates with g_matches
The panel SHALL refresh its marks whenever `g_matches` is updated, without performing any additional document scanning.

#### Scenario: Refresh after Ctrl+Alt+Q
- **WHEN** the user presses Ctrl+Alt+Q
- **THEN** the panel redraws with marks reflecting the latest parse results

#### Scenario: Refresh after real-time edit
- **WHEN** the user types or pastes a keyword and real-time highlighting is active
- **THEN** the panel mark for the new keyword appears immediately
