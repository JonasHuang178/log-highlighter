## ADDED Requirements

### Requirement: Per-buffer state map
The plugin SHALL maintain a `std::unordered_map<LRESULT, BufferState>` named `g_bufferStates` where the key is the NPP buffer ID returned by `NPPM_GETCURRENTBUFFERID`.

`BufferState` SHALL contain:
- `std::vector<Match> matches` — parse results for this buffer
- `bool highlightActive = false` — whether this buffer has been parsed at least once

#### Scenario: New buffer default state
- **WHEN** Ctrl+Alt+Q is pressed on a buffer that has never been parsed
- **THEN** a new `BufferState` entry is created with `highlightActive = false`

#### Scenario: Parsed buffer retains state
- **WHEN** Ctrl+Alt+Q completes on buffer A, then the user switches to buffer B and back to A
- **THEN** `g_bufferStates[idA].matches` still contains A's parse results and `highlightActive` is still `true`

---

### Requirement: Tab switch restores Overview Panel
On `NPPN_BUFFERACTIVATED`, the plugin SHALL:
1. Look up the incoming buffer's `BufferState` in `g_bufferStates`
2. If `highlightActive = true`: call `g_overviewPanel.Update(hSci, BuildPanelMarks(hSci, state.matches))`
3. If `highlightActive = false` or no entry exists: call `g_overviewPanel.Update(hSci, {})` to clear the panel

No parsing, no `ClearAllHighlights`, no indicator changes shall occur during tab switch — Scintilla already preserves the correct indicator state per buffer.

#### Scenario: Switching to a parsed buffer
- **WHEN** the user switches to a tab that was previously parsed with Ctrl+Alt+Q
- **THEN** the Overview Panel immediately shows that buffer's marks without any re-parse

#### Scenario: Switching to an unparsed buffer
- **WHEN** the user switches to a tab that has never been parsed
- **THEN** the Overview Panel is cleared (no marks shown)

#### Scenario: Tab switch does not re-parse
- **WHEN** the user switches between tabs (any combination of parsed and unparsed)
- **THEN** no call to `ParseDocument` or `ApplyHighlights` is made

---

### Requirement: Buffer state removed on file close
On `NPPN_FILEBEFORECLOSE`, the plugin SHALL erase the buffer's entry from `g_bufferStates` using the buffer ID from `notification->nmhdr.idFrom`.

#### Scenario: Closed buffer is removed from map
- **WHEN** the user closes a tab that was previously parsed
- **THEN** its entry is removed from `g_bufferStates`, freeing the match list memory

---

### Requirement: Ctrl+Alt+Q is the sole parse trigger
Parsing SHALL only occur when the user explicitly invokes Parse Log (Ctrl+Alt+Q or Plugins menu). No automatic parse SHALL occur on file open, buffer switch, language change, or text modification.

#### Scenario: Opening a file does not auto-parse
- **WHEN** the user opens any file via File > Open or recent files list
- **THEN** no parse occurs and no highlights are applied

#### Scenario: Re-parse on same buffer replaces state
- **WHEN** the user presses Ctrl+Alt+Q on a buffer that was already parsed
- **THEN** the existing `BufferState` for that buffer is overwritten with new results
