## ADDED Requirements

### Requirement: Progress dialog during parsing
A modeless progress window SHALL be shown on every `ParseLog()` invocation. The window SHALL display the current and total line count and SHALL include a Cancel button. The window size SHALL be fixed (non-resizable) via `WM_GETMINMAXINFO`.

The Notepad++ main window SHALL be disabled (`EnableWindow(FALSE)`) for the duration of parsing to prevent re-entrant calls and menu interactions. A `g_parseInProgress` flag provides an additional re-entrancy guard.

#### Scenario: Progress updates during parse
- **WHEN** `ParseDocument` processes every 500 lines
- **THEN** the progress window label updates to `"Processing: X / Y lines"`

#### Scenario: Parse completes normally
- **WHEN** parsing finishes without cancellation
- **THEN** the progress window is destroyed, the NPP window is re-enabled, and highlights + panel are applied

### Requirement: Cancel support
Clicking Cancel (or closing the progress window via the X button) SHALL set a cancellation flag. `ParseDocument` SHALL return an empty vector on the next 500-line callback check. After cancellation:
- All highlights SHALL be cleared (`ClearAllHighlights`)
- The Overview Panel SHALL be cleared (no marks)
- `g_highlightActive` SHALL be set to `false`
- The document SHALL remain in its original unmodified state

#### Scenario: User cancels mid-parse
- **WHEN** the user clicks Cancel while the progress dialog is visible
- **THEN** parsing stops, all highlights are cleared, the panel shows no marks, and the editor is unchanged

### Requirement: Parser line-by-line with safe buffer copy
`ParseDocument` SHALL copy the Scintilla document buffer to a local `std::vector<char>` before scanning. This ensures the pointer remains valid even if the user edits the document during the `PeekMessage` loop inside the progress callback. The `progressFn(cur, total)` callback SHALL be called every 500 lines and on the final line.

#### Scenario: Safe buffer during PeekMessage
- **WHEN** the progress callback pumps messages and the user attempts to edit the document
- **THEN** the scan continues safely on the local copy; the result is slightly stale but no crash occurs
