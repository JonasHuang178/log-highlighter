## Why

After running Parse Log (Ctrl+Alt+Q) once, the plugin sets a global `g_highlightActive` flag that causes `SCN_MODIFIED` to auto-re-parse every subsequent file opened in NPP — including non-log files. The user wants Ctrl+Alt+Q to be the only trigger for parsing, while preserving highlight results per-buffer so switching tabs and back restores highlights instantly without re-parsing.

## What Changes

- **Remove** `SCN_MODIFIED` auto re-parse behavior entirely
- **Replace** global `g_matches`, `g_appliedByteEnd`, `g_highlightActive` with a per-buffer state map (keyed by NPP buffer ID)
- **Add** `NPPN_BUFFERACTIVATED` notification handler to restore Overview Panel state and lazy-rendering cursor when switching tabs
- **Keep** Ctrl+Alt+Q as the only trigger for parsing

## Capabilities

### New Capabilities

- `per-buffer-state`: Per-buffer highlight state storage and restoration on tab switch

### Modified Capabilities

- `lazy-rendering`: `g_appliedByteEnd` becomes per-buffer (stored in the state map; restored on `NPPN_BUFFERACTIVATED`)
- `progress-dialog`: `g_highlightActive` and `g_parseInProgress` guard logic moves to per-buffer context

## Impact

- `src/Plugin.cpp`: main change site — state map, `NPPN_BUFFERACTIVATED` handler, remove `SCN_MODIFIED` re-parse block
- `src/Parser.h / Parser.cpp`: no change needed
- `src/log-highlighter.h / .cpp`: no change needed
- `src/OverviewPanel.h / .cpp`: no change needed (Overview Panel already accepts marks via `Update()`)
- Scintilla indicators are already stored per-buffer by NPP — visual highlights survive tab switches for free
