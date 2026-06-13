## 1. Extend PluginInterface.h with Docking API

- [x] 1.1 Add `tTbData` struct (hClient, pszName, dlgID, uMask, hIconTab, pszAddInfo, rcFloat, iPrevCont, pszModuleName)
- [x] 1.2 Add docking flags: `DWS_DF_CONT_RIGHT`, `DWS_ICONTAB`, `DWS_ADDINFO`
- [x] 1.3 Add `NPPM_DMMREGASDCKDLG` to `NppMsg` enum (cross-check value against Notepad++ Notepad_plus_msgs.h)
- [x] 1.4 Add `NPPN_READY` to `ScintillaNotification` enum (cross-check value against Notepad++ SDK)
- [x] 1.5 Add `SCN_UPDATEUI` is already present — verify it triggers on scroll events

## 2. Update config/LogPatterns.h

- [x] 2.1 Add `bool showInPanel` field to `LogTypeRule` struct
- [x] 2.2 Add `bool showInPanel` field to `StepTypeRule` struct
- [x] 2.3 Set `showInPanel = false` for all rules in `LOG_TYPE_RULES[]` except `[ ERROR ]`
- [x] 2.4 Set `showInPanel = true` for `[ ERROR ]` rule
- [x] 2.5 Set `showInPanel = false` for all rules in `STEP_TYPE_RULES[]`

## 3. Create config/OverviewConfig.h

- [x] 3.1 Define `OVERVIEW_PANEL_WIDTH` (default: 14) — panel width in pixels
- [x] 3.2 Define `OVERVIEW_MARK_MIN_H` (default: 3) — minimum mark height in pixels
- [x] 3.3 Define `OVERVIEW_VIEWPORT_COLOR` — viewport box color (e.g., `RGB(200, 200, 200)`)
- [x] 3.4 Add `OverviewConfig.h` to `log-highlighter.vcxproj` under config ItemGroup
- [x] 3.5 Add `OverviewConfig.h` to `log-highlighter.vcxproj.filters` under config filter

## 4. Implement OverviewPanel (src/OverviewPanel.h + OverviewPanel.cpp)

- [x] 4.1 Declare `OverviewPanel` class with: `Init(HWND hNpp, HINSTANCE hInst)`, `Update(HWND hSci, const std::vector<Match>&)`, `UpdateViewport()`, `Destroy()`
- [x] 4.2 Implement `Init()`: register window class, create panel HWND, call `NPPM_DMMREGASDCKDLG` with `DWS_DF_CONT_RIGHT`
- [x] 4.3 Implement `WndProc`: handle `WM_PAINT`, `WM_SIZE`, `WM_LBUTTONDBLCLK`, `WM_ERASEBKGND`
- [x] 4.4 Implement `OnPaint()`: double-buffered (off-screen DC + `BitBlt`), fill background, draw colored marks with merge logic, draw viewport box on top
- [x] 4.5 Mark position formula: `y = (matchLine / totalLines) * panelH`; height = `max(OVERVIEW_MARK_MIN_H, panelH / totalLines)`
- [x] 4.6 Mark merge logic: if next same-color mark's y is within 2px of previous mark's bottom, extend previous mark instead of drawing new one
- [x] 4.7 Viewport box: `boxTop = (firstVisible / totalLines) * panelH`; `boxBottom = ((firstVisible + visibleLines) / totalLines) * panelH`; draw as semi-transparent rect
- [x] 4.8 Implement `OnDblClick(y)`: compute `targetLine = (y / panelH) * totalLines`; call `SCI_GOTOLINE(targetLine)` then `SCI_SCROLLCARET`
- [x] 4.9 Implement `Update()`: store `hSci` and filtered marks, call `InvalidateRect`
- [x] 4.10 Implement `UpdateViewport()`: call `InvalidateRect` to trigger viewport box redraw
- [x] 4.11 Add `OverviewPanel.h` and `OverviewPanel.cpp` to `log-highlighter.vcxproj` and `.vcxproj.filters`

## 5. Integrate into Plugin.cpp

- [x] 5.1 Add `#include "OverviewPanel.h"` and declare `static OverviewPanel g_overviewPanel`
- [x] 5.2 In `beNotified()`, handle `NPPN_READY`: call `g_overviewPanel.Init(g_nppData._nppHandle, g_hInstance)`
- [x] 5.3 In `ParseLog()`: after `ApplyHighlights()`, call `g_overviewPanel.Update(hSci, BuildPanelMarks(...))`
- [x] 5.4 In `beNotified()` `SCN_MODIFIED` handler: after `ApplyHighlights()`, call `g_overviewPanel.Update(hSci, BuildPanelMarks(...))`
- [x] 5.5 In `beNotified()`, handle `SCN_UPDATEUI`: call `g_overviewPanel.UpdateViewport()`
- [x] 5.6 Store `HINSTANCE` from `DllMain` in a global for use in `Init()`

## 6. Verify

- [ ] 6.1 Build Release x64 — zero errors and warnings
- [ ] 6.2 Panel appears docked on right side on Notepad++ startup
- [ ] 6.3 Red marks appear for `[ ERROR ]` matches after Ctrl+Alt+Q
- [ ] 6.4 Viewport box moves correctly when scrolling the editor
- [ ] 6.5 Double-click on a mark navigates to the correct line, centered in viewport
- [ ] 6.6 Panel marks update in real time when typing `[ ERROR ]`
- [ ] 6.7 `[ DEBUG ]` produces no panel mark (showInPanel=false) but still colors in editor
