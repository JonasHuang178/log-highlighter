## Why

After highlighting log keywords in the editor (Phase 1), users still need to scroll manually to find all ERROR locations in a large log file. A fixed Overview Panel on the right side gives an instant bird's-eye view of where every highlighted match sits in the document, without scrolling. For very large files, parsing can also freeze the UI without feedback, so a progress dialog with cancel support is needed.

## What Changes

- Add a narrow panel (right side of Scintilla) showing proportional colored marks for all matches where `showInPanel = true`; mark colors match the rule's configured `textColor` / `bgColor`
- Panel implemented via Non-Client Area (NCA) painting on the Scintilla HWND using `SetWindowSubclass` — no separate docked window, no Notepad++ Docking API
- Add `showInPanel` field to `LogTypeRule` and `StepTypeRule` in `config/LogPatterns.h`
- Add `config/OverviewConfig.h` for panel width, minimum mark height, and viewport box colors
- Single-click on the panel navigates to the corresponding line, centered in the editor viewport, with caret placed at the target line
- A viewport indicator box (configurable fill + border color) shows the currently visible region
- Adaptive mark height prevents the panel from turning solid color when marks are dense
- Panel updates automatically whenever `g_matches` is refreshed (Ctrl+Alt+Q or real-time)
- Progress dialog shown during parsing: displays current / total lines, Cancel button clears all highlights and panel
- Parser restructured to line-by-line scanning with local buffer copy for safe UI-thread progress pumping
- Plugin display name changed from `LogHighlighter` to `log-highlighter`
- `src/Highlighter.cpp` / `src/Highlighter.h` renamed to `src/log-highlighter.cpp` / `src/log-highlighter.h`
- New files: `src/ProgressDialog.h`, `src/ProgressDialog.cpp`

## Capabilities

### New Capabilities

- `overview-panel`: NCA-based panel on the right side of Scintilla that paints proportional colored marks, displays a viewport indicator box, and supports single-click navigation to jump to any line
- `progress-dialog`: Modeless progress window shown during `ParseDocument`; updates every 500 lines; Cancel clears highlights and panel

### Modified Capabilities

- `log-patterns-config`: Add `showInPanel` field to both `LogTypeRule` and `StepTypeRule` structs
- `parser`: Restructured to line-by-line scan with `progressFn(cur, total)` callback and local buffer copy

## Impact

- New files: `src/OverviewPanel.h`, `src/OverviewPanel.cpp`, `config/OverviewConfig.h`, `src/ProgressDialog.h`, `src/ProgressDialog.cpp`
- Renamed: `src/Highlighter.cpp` → `src/log-highlighter.cpp`, `src/Highlighter.h` → `src/log-highlighter.h`
- Modified: `config/LogPatterns.h`, `src/Plugin.cpp`, `src/Parser.h`, `src/Parser.cpp`, `external/Scintilla.h`
- No Notepad++ Docking API required; no changes to `external/PluginInterface.h`
