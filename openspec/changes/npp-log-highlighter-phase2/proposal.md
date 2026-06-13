## Why

After highlighting log keywords in the editor (Phase 1), users still need to scroll manually to find all ERROR locations in a large log file. A fixed Overview Panel on the right side gives an instant bird's-eye view of where every highlighted match sits in the document, without scrolling.

## What Changes

- Add a narrow docked panel (right side) showing proportional colored marks for all matches where `showInPanel = true`
- Add `showInPanel` field to `LogTypeRule` and `StepTypeRule` in `config/LogPatterns.h` (default `false`; `[ ERROR ]` set to `true`)
- Add `config/OverviewConfig.h` for panel width and minimum mark height
- Double-clicking a mark navigates to that line (centered in viewport)
- A viewport indicator box shows the currently visible region of the document
- Panel updates automatically whenever `g_matches` is refreshed (Ctrl+Alt+Q or real-time)
- Extend `external/PluginInterface.h` with Docking API structs and notification codes

## Capabilities

### New Capabilities

- `overview-panel`: Docked right-side panel that paints proportional colored marks for each match where `showInPanel = true`, displays a viewport indicator box, and supports double-click navigation to jump to any match line

### Modified Capabilities

- `log-patterns-config`: Add `showInPanel` field to both `LogTypeRule` and `StepTypeRule` structs in `config/LogPatterns.h`

## Impact

- New files: `src/OverviewPanel.h`, `src/OverviewPanel.cpp`, `config/OverviewConfig.h`
- Modified files: `config/LogPatterns.h`, `src/Plugin.cpp`, `external/PluginInterface.h`
- No changes to `Parser.cpp`, `Highlighter.cpp`, or the DLL export interface
- Requires Notepad++ Docking API (`NPPM_DMMREGASDCKDLG`, `tTbData`) and `NPPN_READY` notification — both added to `external/PluginInterface.h`
