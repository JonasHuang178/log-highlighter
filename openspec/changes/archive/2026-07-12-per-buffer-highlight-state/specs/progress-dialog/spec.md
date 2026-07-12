## MODIFIED Requirements

### Requirement: NPP window disabled during parse
`EnableWindow(g_nppData._nppHandle, FALSE)` SHALL be called immediately after the dialog is shown. `EnableWindow(TRUE)` SHALL be called before `DestroyWindow(hDlg)`. This prevents re-entrant `Ctrl+Alt+Q` presses and interaction with NPP menus while parsing is in progress.

A per-buffer `parseInProgress` flag (stored as a separate `static bool g_parseInProgress` that is reset after parse, not per-buffer) provides an additional re-entrancy guard independent of the window enable state.

#### Scenario: Re-entrant Ctrl+Alt+Q is blocked
- **WHEN** the user presses Ctrl+Alt+Q while a parse is already running
- **THEN** the second invocation returns immediately without starting a new parse

## REMOVED Requirements

### Requirement: Cancel support clears g_highlightActive
**Reason**: `g_highlightActive` is now per-buffer (`BufferState::highlightActive`). The cancel path sets `currentBuffer.highlightActive = false` for the buffer being parsed, not a global flag.
**Migration**: No behavioral change from the user's perspective — cancelling still clears highlights for the current buffer only.
