## Requirements

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

---

### Requirement: Per-buffer scan caches
`BufferState` SHALL hold, in addition to the existing match list, a bookmark line cache used by
Next Bookmark and a rendered report cache used by Custom Report commands.

#### Scenario: Independent population
- **WHEN** any one of Parse Log, Next Bookmark or Custom Report runs
- **THEN** only the cache belonging to that command is required to be populated for it to succeed

---

### Requirement: Stale flag driven by document modification
`BufferState` SHALL carry a `stale` flag. The plugin SHALL handle `SCN_MODIFIED` for
`SC_MOD_INSERTTEXT` and `SC_MOD_DELETETEXT` and set `stale` on **every tracked buffer**, not only
the active one. Handling this notification SHALL NOT trigger any scan, parse, highlight or
report, and SHALL NOT create state entries for buffers that have none.

#### Scenario: User types in the document
- **WHEN** text is inserted or deleted in a buffer
- **THEN** every tracked buffer's `stale` flag is set
- **AND** no parse, scan or report is started

#### Scenario: Modification reaches a buffer that is not active
- **WHEN** a document is modified without being the active buffer, as with Replace All in All
  Opened Documents
- **THEN** that buffer's caches are still invalidated
- **AND** no command serves a result built before the modification

#### Scenario: Unrelated modifications
- **WHEN** a modification occurs that is neither an insertion nor a deletion of text
- **THEN** no `stale` flag is set

#### Scenario: Buffer with no cached state
- **WHEN** a modification occurs and some open buffer has no `BufferState` entry
- **THEN** no entry is created for it
- **AND** it scans fresh on first use

---

### Requirement: Stale invalidates every cached scan
When `stale` is set, the bookmark line cache and the report cache SHALL be treated as invalid.
The next invocation of a command SHALL rescan before using its cache, and SHALL clear `stale` for
that buffer once its cache is repopulated. Clearing SHALL apply only to the buffer being used, so
that other buffers remain invalidated until they are next used.

#### Scenario: Report after an edit
- **WHEN** the document is edited after a report was displayed
- **AND** the user invokes the same report again
- **THEN** the document is rescanned and the report describes the current content

#### Scenario: Navigation after an edit
- **WHEN** the document is edited after Next Bookmark has cached bookmark lines
- **AND** the user presses Ctrl+Alt+W
- **THEN** the document is rescanned and navigation targets current line positions

#### Scenario: Another buffer used later
- **WHEN** one buffer has rescanned and cleared its own flag after an edit
- **AND** the user switches to a different buffer and invokes a command
- **THEN** that buffer also rescans rather than serving its pre-edit cache

#### Scenario: Highlights after an edit
- **WHEN** the document is edited after Parse Log has run
- **THEN** existing highlights remain on screen and are not refreshed automatically
- **AND** the next Parse Log rescans rather than reusing the cached match list

---

### Requirement: Cache lifetime follows the buffer
All caches and the `stale` flag SHALL be released when the buffer is closed, via the existing
`NPPN_FILEBEFORECLOSE` handler.

#### Scenario: Tab closed
- **WHEN** a buffer with populated caches is closed
- **THEN** all of its cached state is released

#### Scenario: Tab switch
- **WHEN** the user switches between tabs
- **THEN** each buffer retains its own caches and stale flag independently
