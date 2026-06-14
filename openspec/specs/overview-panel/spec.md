## Requirements

### Requirement: Non-client-area panel
The plugin SHALL render the Overview Panel by subclassing the Scintilla HWND via `SetWindowSubclass` and intercepting Win32 non-client-area messages. No separate child HWND is created.

The panel occupies the rightmost `OVERVIEW_PANEL_WIDTH` (default 14) pixels of the Scintilla window rect, positioned **to the right of Scintilla's vertical scrollbar**. This is achieved by subtracting `OVERVIEW_PANEL_WIDTH` from the proposed right edge in `WM_NCCALCSIZE` *before* passing to `DefSubclassProc`, so the scrollbar and all Scintilla borders are laid out within the remaining narrower area.

#### Scenario: Panel appears after first Ctrl+Alt+Q
- **WHEN** the user presses Ctrl+Alt+Q for the first time
- **THEN** a `OVERVIEW_PANEL_WIDTH`-px strip appears on the right edge of the Scintilla window, to the right of the scrollbar

#### Scenario: Panel survives editor resize
- **WHEN** the user resizes the Notepad++ window
- **THEN** the panel redraws at correct proportional mark positions

---

### Requirement: Double-buffered painting
`WM_NCPAINT` SHALL:
1. Call `DefSubclassProc` first so Scintilla draws its own borders and scrollbars.
2. Acquire a window DC via `GetWindowDC`.
3. Create an off-screen compatible DC + bitmap of the panel dimensions.
4. Render all content (background, marks, viewport box) into the off-screen DC.
5. `BitBlt` the result to the window DC.
6. Release all GDI objects.

This prevents visible flicker during rapid scroll.

---

### Requirement: Panel background
The panel background SHALL be filled with `GetSysColor(COLOR_BTNFACE)`, matching the Windows scrollbar track color.

---

### Requirement: Proportional colored marks with adaptive height
The panel SHALL paint one colored mark per entry in the `PanelMark` list (marks where `showInPanel = true` in `LogPatterns.h`).

Mark color:
- `LOG_TYPE` rule → `rule.textColor`
- `STEP_TYPE` rule → `rule.bgColor`

Mark height (in pixels):
```
markH_base = max(OVERVIEW_MARK_MIN_H, panelH / totalLines)
adaptive   = panelH * 0.5 / numMarks        (only if numMarks > 0)
markH      = max(1, min(markH_base, adaptive))
```

This limits total mark coverage to approximately 50% of panel height when marks are dense.

Adjacent marks with the **same color** whose pixel ranges overlap (`nextY < currentMergeBot`) SHALL be merged into a single rectangle.

#### Scenario: Single ERROR in a large document
- **WHEN** the document has 1000 lines and one `[ ERROR ]` at line 500
- **THEN** one red mark appears at the vertical midpoint of the panel

#### Scenario: Dense ERROR cluster
- **WHEN** many `[ ERROR ]` matches exist across a large document
- **THEN** marks are scaled down so total colored area stays below ~50% of panel height

#### Scenario: showInPanel false rule excluded
- **WHEN** a `[ WARN ]` match exists and its `showInPanel` is `false`
- **THEN** no mark appears in the panel for that match

---

### Requirement: Viewport indicator box
The panel SHALL draw a filled + outlined rectangle indicating the currently visible region of the document.

- **Fill color**: `OVERVIEW_VIEWPORT_BG_COLOR` (default `CLR_NONE` = `GetSysColor(COLOR_SCROLLBAR)`)
- **Border color**: `OVERVIEW_VIEWPORT_COLOR` (default `RGB(130,130,130)`)
- **Minimum height**: 2 px (clamped if the viewport is very small relative to the document)
- **Update trigger**: every `SCN_UPDATEUI` notification

`m_visibleLines` is cached during `DrawPanel` (which runs inside `WM_NCPAINT` when Scintilla's layout is stable). The cached value is used during deferred navigation to avoid calling `SCI_LINESONSCREEN` from a `WM_TIMER` callback, where Scintilla layout may not yet be settled.

#### Scenario: Viewport box tracks scrolling
- **WHEN** the user scrolls the editor
- **THEN** the viewport box moves proportionally within the panel

---

### Requirement: Single-click navigation
A left mouse button click (`WM_NCLBUTTONDOWN` with `wParam == HTBORDER`) on the panel strip SHALL navigate the editor to the proportionally corresponding line.

`WM_NCLBUTTONDBLCLK` and `WM_NCLBUTTONUP` with `HTBORDER` SHALL return 0 to suppress the default system behavior for those messages (resizing, menu, etc.).

Navigation is deferred via `SetTimer(hSci, 0xAB42, 10ms)` so that `DoNavigation()` runs after all click-message processing is complete.

`DoNavigation()` SHALL:
1. Compute `targetFirst = clickedLine - visibleLines/2`, clamped to `[0, totalLines - visibleLines]`
2. Call `SCI_SETFIRSTVISIBLELINE` to center the view on the target line
3. Call `SCI_SETEMPTYSELECTION` with the byte position of the target line start (places caret, clears selection)
4. Call `SCI_SETFIRSTVISIBLELINE` again to correct any scroll drift caused by the caret move

#### Scenario: Click on a mark
- **WHEN** the user clicks on a colored mark in the panel
- **THEN** the editor scrolls so the corresponding line is centered, caret is at line start, no text is selected

#### Scenario: Click on empty panel area
- **WHEN** the user clicks on a panel area with no mark
- **THEN** the editor navigates to the proportionally corresponding line and centers it

---

### Requirement: Arrow cursor in panel strip
`WM_SETCURSOR` SHALL show `IDC_ARROW` when the cursor is inside the panel strip, overriding the `HTBORDER` default resize cursor.

---

### Requirement: Lifecycle
- `Init(hNpp, hSci, hInst)` — installs the subclass; triggers `SWP_FRAMECHANGED` to force `WM_NCCALCSIZE` immediately. Called on the first `ParseLog()` invocation.
- `Update(hSci, marks)` — stores the new mark list, caches `totalLines`, invalidates the NCA frame.
- `UpdateViewport()` — invalidates the NCA frame (moves viewport box). Called on every `SCN_UPDATEUI`.
- `Destroy()` — removes the subclass, triggers `SWP_FRAMECHANGED` to restore Scintilla's full client width.
- `WM_NCDESTROY` — if Scintilla is destroyed before `Destroy()` is called, the subclass proc cleans up automatically.

---

### Requirement: Configuration via OverviewConfig.h
Appearance constants are defined in `config/OverviewConfig.h` and require a rebuild to change:

| Constant | Default | Description |
|---|---|---|
| `OVERVIEW_PANEL_WIDTH` | `14` | Panel width in pixels |
| `OVERVIEW_MARK_MIN_H` | `1` | Minimum mark height in pixels |
| `OVERVIEW_BG_COLOR` | `RGB(60,60,60)` | Panel background (currently unused; background uses `COLOR_BTNFACE`) |
| `OVERVIEW_VIEWPORT_COLOR` | `RGB(130,130,130)` | Viewport box border color |
| `OVERVIEW_VIEWPORT_BG_COLOR` | `CLR_NONE` | Viewport box fill (`CLR_NONE` = system scrollbar color) |
