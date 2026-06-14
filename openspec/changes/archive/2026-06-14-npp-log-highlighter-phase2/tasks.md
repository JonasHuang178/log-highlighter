## 1. Update config/LogPatterns.h

- [x] 1.1 Add `bool showInPanel` field to `LogTypeRule` struct
- [x] 1.2 Add `bool showInPanel` field to `StepTypeRule` struct
- [x] 1.3 Set `showInPanel = true` for `[ ERROR ]` rule; `false` for all others

## 2. Create config/OverviewConfig.h

- [x] 2.1 Define `OVERVIEW_PANEL_WIDTH` (14px)
- [x] 2.2 Define `OVERVIEW_MARK_MIN_H` (1px — reduced from 3 to prevent solid-color panel)
- [x] 2.3 Define `OVERVIEW_VIEWPORT_COLOR` — viewport box border color
- [x] 2.4 Define `OVERVIEW_VIEWPORT_BG_COLOR` — viewport box fill color (`CLR_NONE` = system scrollbar track color)
- [x] 2.5 Add to `log-highlighter.vcxproj` and `.vcxproj.filters`

## 3. Add SCI_ constants to external/Scintilla.h

- [x] 3.1 `SCI_POSITIONFROMLINE`, `SCI_GETLINEENDPOSITION`
- [x] 3.2 `SCI_SETSELECTIONSTART`, `SCI_SETSELECTIONEND`, `SCI_SETEMPTYSELECTION`
- [x] 3.3 `SCI_SETFIRSTVISIBLELINE`, `SCI_LINESCROLL`

## 4. Implement OverviewPanel (NCA approach)

- [x] 4.1 Declare `OverviewPanel` class: `Init(HWND hNpp, HWND hSci, HINSTANCE)`, `Update(HWND, vector<PanelMark>)`, `UpdateViewport()`, `Destroy()`, `IsInitialized()`
- [x] 4.2 Implement `Init()`: `SetWindowSubclass(hSci)` + `SetWindowPos(SWP_FRAMECHANGED)` to trigger `WM_NCCALCSIZE`
- [x] 4.3 `WM_NCCALCSIZE`: subtract `OVERVIEW_PANEL_WIDTH` from right edge before calling `DefSubclassProc`
- [x] 4.4 `WM_NCPAINT`: double-buffered off-screen paint via `GetWindowDC`, `BitBlt` to panel strip
- [x] 4.5 `WM_NCHITTEST`: return `HTBORDER` when cursor is inside panel strip
- [x] 4.6 `WM_NCLBUTTONDOWN` (HTBORDER): store `m_pendingNavLine`, `SetTimer(10ms)`
- [x] 4.7 `WM_TIMER`: call `DoNavigation()` — `SCI_SETFIRSTVISIBLELINE` + `SCI_SETEMPTYSELECTION` + re-anchor scroll
- [x] 4.8 `WM_SETCURSOR`: show `IDC_ARROW` when hovering panel strip
- [x] 4.9 `WM_NCDESTROY`: `RemoveWindowSubclass`, clear `m_hSci`
- [x] 4.10 `DrawPanel()`: background fill, adaptive mark height, merge-on-overlap logic, viewport box with fill + border

## 5. Adaptive mark height and merge logic

- [x] 5.1 `markH_base = max(OVERVIEW_MARK_MIN_H, panelH / totalLines)`
- [x] 5.2 `adaptive = panelH * 0.5 / numMarks` — cap total mark coverage at 50% of panel height
- [x] 5.3 `markH = max(1, min(markH_base, adaptive))`
- [x] 5.4 Merge condition: `y < mergeBot` (truly overlapping only, no 2px tolerance)

## 6. Mark colors from rule configuration

- [x] 6.1 `BuildPanelMarks()`: LOG_TYPE marks use `rule.textColor`; STEP_TYPE use `rule.bgColor`
- [x] 6.2 No BGR↔RGB swap (`MAKE_BGR(r,g,b) == RGB(r,g,b)`)

## 7. Viewport indicator box

- [x] 7.1 Fill with `OVERVIEW_VIEWPORT_BG_COLOR` (`CLR_NONE` → `GetSysColor(COLOR_SCROLLBAR)`)
- [x] 7.2 Border with `OVERVIEW_VIEWPORT_COLOR`
- [x] 7.3 Cache `SCI_LINESONSCREEN` result as `m_visibleLines` during `WM_NCPAINT`; use cached value in `DoNavigation()`

## 8. Parser: line-by-line with progress callback

- [x] 8.1 Change `progressFn` signature to `std::function<bool(int cur, int total)>`
- [x] 8.2 Copy Scintilla buffer to `std::vector<char>` before scanning (safe during `PeekMessage`)
- [x] 8.3 Implement `find_range(hay, hayEnd, needle, len)` for bounded keyword search
- [x] 8.4 Scan line-by-line: LOG_TYPE (all occurrences per line) + STEP_TYPE (prefix+digits+space|EOL)
- [x] 8.5 Call `progressFn(lineNo, totalLines)` every 500 lines and on final line; return `{}` on cancel

## 9. Progress dialog (ProgressDialog.h / ProgressDialog.cpp)

- [x] 9.1 `CreateProgressDialog(hParent, hInst)`: modeless `WS_POPUP | WS_CAPTION | WS_SYSMENU` window, fixed size via `WM_GETMINMAXINFO`
- [x] 9.2 Static text label + Cancel button; `DlgState` in `GWLP_USERDATA`
- [x] 9.3 `WM_COMMAND` (Cancel / IDCANCEL) + `WM_CLOSE` → set `cancelled = true`
- [x] 9.4 `SetProgressLine(hDlg, cur, total)`: update label to `"Processing: X / Y lines"`
- [x] 9.5 `IsProgressCancelled(hDlg)`: read `DlgState::cancelled`

## 10. Integrate into Plugin.cpp

- [x] 10.1 `g_parseInProgress` re-entrancy guard in `ParseLog()`
- [x] 10.2 `EnableWindow(hNpp, FALSE)` during parse; restore + `SetForegroundWindow` after
- [x] 10.3 Pass progress lambda to `ParseDocument()`; lambda calls `SetProgressLine`, `PeekMessage` loop, checks `IsProgressCancelled`
- [x] 10.4 On cancel: `ClearAllHighlights`, `g_overviewPanel.Update({})`, `g_highlightActive = false`
- [x] 10.5 On success: `ClearAllHighlights` → `ApplyHighlights` (with `RDW_UPDATENOW`) → lazy `Init` → `Update`
- [x] 10.6 `SCN_MODIFIED` handler guards `g_parseInProgress` (indirectly via `g_highlightActive`)

## 11. Rename and branding

- [x] 11.1 Rename `src/Highlighter.cpp` → `src/log-highlighter.cpp`, `src/Highlighter.h` → `src/log-highlighter.h`
- [x] 11.2 Update all `#include "Highlighter.h"` references
- [x] 11.3 Update `.vcxproj` and `.vcxproj.filters` entries
- [x] 11.4 `getName()` returns `"log-highlighter"`
- [x] 11.5 `PLUGIN_NAME` in `config/AboutInfo.h` → `"log-highlighter"`
- [x] 11.6 Progress dialog title → `"log-highlighter"`

## 12. Verify

- [x] 12.1 Build Release x64 — zero errors and warnings
- [x] 12.2 Panel appears on right side of Scintilla after first Ctrl+Alt+Q
- [x] 12.3 Red marks appear for `[ ERROR ]` matches; colors match rule config
- [x] 12.4 Viewport box moves correctly when scrolling the editor
- [x] 12.5 Single-click on a mark navigates to the correct line, centered in viewport, caret placed
- [x] 12.6 Panel marks update in real time when typing `[ ERROR ]`
- [x] 12.7 `[ DEBUG ]` produces no panel mark (showInPanel=false) but still colors in editor
- [x] 12.8 Progress dialog shows and updates on every parse
- [x] 12.9 Cancel clears highlights and panel, leaves document in original state
- [x] 12.10 Dense [ERROR] marks do not produce solid-color panel (adaptive height)
